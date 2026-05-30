// WIFI STUFF
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include "nds.h"
#include <dswifi9.h>
#include <wfc.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/select.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <../lib/rTOC2/toc2.h>
#include "globalDefs.h"
#include "commonObjs.h"
#include "menudefs.h"
#include "wificonnect.h"
#ifdef WIFIDEBUG
#include "../lib/rTOC2/wifidebug.h"
#define WPRINT(format, args...)  rWifiDebug::rPrint("Template: %s :" format,__func__, ##args)
#else
#define WPRINT(format, args...)
#endif
#include "debug.h"

namespace {

static const int kConnectFailureTimeoutFrames = 15 * 60;
static const char kConnectivityTestHost[] = "example.com";
static const int kConnectivityTestPort = 80;
static const int kConnectivityTestTimeoutSec = 3;

struct WifiSelection {
    WlanBssDesc bss;
};

static const WlanBssScanFilter kScanFilter = {
    UINT_MAX,
    { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff },
    0,
    { 0 }
};

WlanBssAuthType authMaskToType(unsigned mask)
{
    return mask ? (WlanBssAuthType)(31 - __builtin_clz(mask)) : WlanBssAuthType_Open;
}

bool isOpenAuth(WlanBssAuthType type)
{
    return type == WlanBssAuthType_Open;
}

bool isWepAuth(WlanBssAuthType type)
{
    return type == WlanBssAuthType_WEP_40 ||
           type == WlanBssAuthType_WEP_104 ||
           type == WlanBssAuthType_WEP_128;
}

bool isWpaAuth(WlanBssAuthType type)
{
    return type == WlanBssAuthType_WPA_PSK_TKIP ||
           type == WlanBssAuthType_WPA2_PSK_TKIP ||
           type == WlanBssAuthType_WPA_PSK_AES ||
           type == WlanBssAuthType_WPA2_PSK_AES;
}

const char *authLabel(WlanBssAuthType type)
{
    switch (type)
    {
        case WlanBssAuthType_Open:          return "open";
        case WlanBssAuthType_WEP_40:
        case WlanBssAuthType_WEP_104:
        case WlanBssAuthType_WEP_128:       return "wep";
        case WlanBssAuthType_WPA_PSK_TKIP:  return "wpa";
        case WlanBssAuthType_WPA2_PSK_TKIP: return "wpa2";
        case WlanBssAuthType_WPA_PSK_AES:   return "wpa";
        case WlanBssAuthType_WPA2_PSK_AES:  return "wpa2";
        default:                            return "secure";
    }
}

bool parseHexNibble(char c, unsigned char &value)
{
    if (c >= '0' && c <= '9')
    {
        value = (unsigned char)(c - '0');
        return true;
    }
    if (c >= 'a' && c <= 'f')
    {
        value = (unsigned char)(c - 'a' + 10);
        return true;
    }
    if (c >= 'A' && c <= 'F')
    {
        value = (unsigned char)(c - 'A' + 10);
        return true;
    }
    return false;
}

bool parseHexBytes(const char *text, unsigned expectedBytes, unsigned char *out)
{
    const size_t len = strlen(text);
    if (len != expectedBytes * 2)
        return false;

    for (unsigned i = 0; i < expectedBytes; ++i)
    {
        unsigned char hi = 0;
        unsigned char lo = 0;
        if (!parseHexNibble(text[i * 2], hi) || !parseHexNibble(text[i * 2 + 1], lo))
            return false;
        out[i] = (unsigned char)((hi << 4) | lo);
    }
    return true;
}

bool prepareAuthData(WlanBssAuthType &type, const char *ssid, unsigned ssidLen, const char *key, WlanAuthData &out)
{
    const size_t len = strlen(key);
    memset(&out, 0, sizeof(out));

    if (isOpenAuth(type))
        return true;

    if (isWepAuth(type))
    {
        if (len == WLAN_WEP_40_LEN)
        {
            type = WlanBssAuthType_WEP_40;
            memcpy(out.wep_key, key, len);
            return true;
        }
        if (len == WLAN_WEP_104_LEN)
        {
            type = WlanBssAuthType_WEP_104;
            memcpy(out.wep_key, key, len);
            return true;
        }
        if (len == WLAN_WEP_128_LEN)
        {
            type = WlanBssAuthType_WEP_128;
            memcpy(out.wep_key, key, len);
            return true;
        }
        if (parseHexBytes(key, WLAN_WEP_40_LEN, out.wep_key))
        {
            type = WlanBssAuthType_WEP_40;
            return true;
        }
        if (parseHexBytes(key, WLAN_WEP_104_LEN, out.wep_key))
        {
            type = WlanBssAuthType_WEP_104;
            return true;
        }
        if (parseHexBytes(key, WLAN_WEP_128_LEN, out.wep_key))
        {
            type = WlanBssAuthType_WEP_128;
            return true;
        }
        return false;
    }

    if (isWpaAuth(type))
    {
        if (len >= 8 && len <= 63)
            return wfcDeriveWpaKey(&out, ssid, ssidLen, key, (unsigned)len);

        if (parseHexBytes(key, WLAN_WPA_PSK_LEN, out.wpa_psk))
            return true;

        return false;
    }

    return false;
}

bool canReachNetworkHost(const char *hostName, int port)
{
    struct hostent *host = gethostbyname(hostName);
    if (!host)
        return false;

    const int sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock < 0)
        return false;

    bool ok = false;
    const int flags = fcntl(sock, F_GETFL, 0);
    if (flags >= 0)
        fcntl(sock, F_SETFL, flags | O_NONBLOCK);

    sockaddr_in server;
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = *((unsigned long *)host->h_addr_list[0]);
    server.sin_port = htons((unsigned short)port);

    if (connect(sock, (struct sockaddr*)&server, sizeof(server)) == 0)
    {
        ok = true;
    }
    else if ((errno == EINPROGRESS) || (errno == EWOULDBLOCK))
    {
        fd_set writeSet;
        FD_ZERO(&writeSet);
        FD_SET(sock, &writeSet);

        timeval timeout;
        timeout.tv_sec = kConnectivityTestTimeoutSec;
        timeout.tv_usec = 0;

        const int ready = select(sock + 1, NULL, &writeSet, NULL, &timeout);
        if ((ready > 0) && FD_ISSET(sock, &writeSet))
        {
            int socketError = 0;
            socklen_t optLen = sizeof(socketError);
            if ((getsockopt(sock, SOL_SOCKET, SO_ERROR, &socketError, &optLen) == 0) && (socketError == 0))
                ok = true;
        }
    }

    close(sock);
    return ok;
}

void resetScanEntries()
{
    if (!activeListBox)
        return;

    for (int i = 0; i < activeListBox->rGetTotal(); ++i)
    {
        WifiSelection *selection = (WifiSelection*)activeListBox->rGetData(i);
        if (selection)
            delete selection;
    }
}

}

extern "C" {

int wifi_init[] = {0, 0};

void sgIP_dbgprint(char * txt, ...)
{
}

void initWifi()
{
    if (wifi_init[0] == 1)
        return;

    if (Wifi_InitDefault(INIT_ONLY))
    {
        wifi_init[0] = 1;
        wifi_init[1] = 0;
    }
}

int autoConnect()
{
    if (wifi_init[0] == 0 && !Wifi_InitDefault(INIT_ONLY))
        return -1;
    wifi_init[0] = 1;

    if (!wfcBeginAutoConnect())
        return -1;

    int polls = 0;
    bool sawProgress = false;
    while (pmMainLoop())
    {
        switch (wfcGetStatus())
        {
            case WfcStatus_Scanning:
            case WfcStatus_Connecting:
            case WfcStatus_AcquiringIP:
                sawProgress = true;
                polls = 0;
                break;
            case WfcStatus_Connected:
                wifi_init[1] = 1;
                return 1;
            case WfcStatus_Disconnected:
            default:
                if (sawProgress || polls++ >= kConnectFailureTimeoutFrames)
                {
                    wifi_init[1] = 0;
                    return -1;
                }
                break;
        }
        swiWaitForVBlank();
    }

    wifi_init[1] = 0;
    return -1;
}

}

void rWifiConnect::rInitWifi()
{
    if (wifi_init[0] != 0)
        Wifi_DisconnectAP();
    isConnecting = false;
    connectPolls = 0;
    sawConnectionProgress = false;
    wifi_init[0] = 0;
    wifi_init[1] = 0;
}

int rWifiConnect::rConnect()
{
    if (!isConnecting)
    {
        bool wifiReady = (wifi_init[0] != 0);
        bool started = false;

        if (!wifiReady)
        {
            if (useFirmwareSettings || !selectedApValid)
            {
                started = Wifi_InitDefault(WFC_CONNECT);
                if (started)
                {
                    wifi_init[0] = 1;
                    wifi_init[1] = 0;
                    wifiReady = true;
                }
            }
            else
            {
                wifiReady = Wifi_InitDefault(INIT_ONLY);
                if (wifiReady)
                    wifi_init[0] = 1;
            }
        }

        if (!started)
        {
            if (!wifiReady)
                return ASSOCSTATUS_CANNOTCONNECT;

            rResetConnectState(false);
            started = (useFirmwareSettings || !selectedApValid) ?
                wfcBeginAutoConnect() :
                wfcBeginConnect(&selectedAp, isOpenAuth(selectedAp.auth_type) ? NULL : &authData);
        }

        if (!started)
            return ASSOCSTATUS_CANNOTCONNECT;
        isConnecting = true;
        connectPolls = 0;
        sawConnectionProgress = false;
    }

    const int status = rTranslateStatus();
    if (status == ASSOCSTATUS_ASSOCIATED)
    {
        wifi_init[1] = 1;
        isConnecting = false;
        return status;
    }

    if (status == ASSOCSTATUS_CANNOTCONNECT)
    {
        wifi_init[1] = 0;
        rResetConnectState(true);
        return status;
    }

    return status;
}

bool rWifiConnect::rTestConnection()
{
    return canReachNetworkHost(kConnectivityTestHost, kConnectivityTestPort);
}

void rWifiConnect::rResetConnectState(bool disconnectAp)
{
    isConnecting = false;
    connectPolls = 0;
    sawConnectionProgress = false;
    if (disconnectAp && wifi_init[0] != 0)
        Wifi_DisconnectAP();
}

int rWifiConnect::rTranslateStatus()
{
    switch (wfcGetStatus())
    {
        case WfcStatus_Scanning:
            sawConnectionProgress = true;
            connectPolls = 0;
            return ASSOCSTATUS_SEARCHING;
        case WfcStatus_Connecting:
            sawConnectionProgress = true;
            connectPolls = 0;
            return ASSOCSTATUS_ASSOCIATING;
        case WfcStatus_AcquiringIP:
            sawConnectionProgress = true;
            connectPolls = 0;
            return ASSOCSTATUS_ACQUIRINGDHCP;
        case WfcStatus_Connected:
            return ASSOCSTATUS_ASSOCIATED;
        case WfcStatus_Disconnected:
        default:
            if (sawConnectionProgress || connectPolls++ >= kConnectFailureTimeoutFrames)
                return ASSOCSTATUS_CANNOTCONNECT;
            return ASSOCSTATUS_SEARCHING;
    }
}

int rWifiConnect::rCheckNetSetupStep(int stat, unsigned short int** fnt, void* extra)
{
    WPRINT("on step %d with stat %d", setupStep, stat);

    switch (setupStep)
    {
        case NET_STEP_AP_MENU:
        {
            switch (stat)
            {
                case D_CANCEL:
                    setupStep = NET_STEP_CANCEL;
                    return -1;
                case NETWORK_BROWSE_AP:
                    useFirmwareSettings = false;
                    setupStep = NET_STEP_AP_BROWSE;
                    rDoAPBrowseList(fnt);
                    break;
                case NETWORK_USE_FIRMWARE:
                    useFirmwareSettings = true;
                    selectedApValid = false;
                    setupStep = NET_STEP_COMPLETE;
                    rDoSetupCompleteDialog(fnt);
                    break;
                default:
                    break;
            }
        } break;

        case NET_STEP_AP_BROWSE:
        {
            if (stat == D_CANCEL)
            {
                setupStep = NET_STEP_AP_MENU;
                rDoAPSetupMenu(fnt);
            }
            else if (extra)
            {
                rSaveSettings(extra);
                rCleanUp();
                setupStep = NET_STEP_AP_CONFIRM;
                rDoAPSelectConfirmDialog(fnt);
            }
        } break;

        case NET_STEP_AP_CONFIRM:
        {
            if (stat == D_CANCEL)
            {
                setupStep = NET_STEP_AP_BROWSE;
                rDoAPBrowseList(fnt);
            }
            else
            {
                pendingAuthType = authMaskToType(selectedAp.auth_mask);
                selectedAp.auth_type = pendingAuthType;

                if (isOpenAuth(pendingAuthType))
                {
                    setupStep = NET_STEP_COMPLETE;
                    rDoSetupCompleteDialog(fnt);
                }
                else
                {
                    setupStep = NET_STEP_WEP_KEY;
                    rDoEnterWepKey(fnt);
                }
            }
        } break;

        case NET_STEP_WEP_KEY:
        {
            if (stat == D_CANCEL)
            {
                setupStep = NET_STEP_AP_CONFIRM;
                rDoAPSelectConfirmDialog(fnt);
                break;
            }

            if (!rVerifyWepKey((char*)extra))
            {
                setupStep = NET_STEP_WEP_INVALID;
                rDoWepInvalidDialog(fnt);
            }
            else
            {
                rSaveSettings(extra);
                setupStep = NET_STEP_COMPLETE;
                rDoSetupCompleteDialog(fnt);
            }
        } break;

        case NET_STEP_WEP_INVALID:
        {
            setupStep = NET_STEP_WEP_KEY;
            rDoEnterWepKey(fnt);
        } break;

        case NET_STEP_COMPLETE:
        {
            setupComplete = true;
            isConnecting = false;
            setupStep = NET_STEP_NOTRUNNING;
            return -1;
        } break;

        default:
            break;
    }

    return 0;
}

bool rWifiConnect::rVerifyChannel(char *channel)
{
    return channel && channel[0] != '\0';
}

bool rWifiConnect::rVerifyWepKey(char *key)
{
    WlanBssAuthType type = pendingAuthType;
    WlanAuthData data;

    if (!key)
        return false;

    return prepareAuthData(type, selectedAp.ssid, selectedAp.ssid_len, key, data);
}

bool rWifiConnect::rVerifyIP(char *IP)
{
    return IP && IP[0] != '\0';
}

void rWifiConnect::rSaveSettings(void *data)
{
    switch (setupStep)
    {
        case NET_STEP_AP_BROWSE:
        {
            WifiSelection *selection = (WifiSelection*)data;
            if (!selection)
                break;

            memcpy(&selectedAp, &selection->bss, sizeof(selectedAp));
            selectedApValid = true;
        } break;

        case NET_STEP_WEP_KEY:
        {
            char *key = (char*)data;
            WlanBssAuthType type = pendingAuthType;
            if (!prepareAuthData(type, selectedAp.ssid, selectedAp.ssid_len, key, authData))
                break;

            pendingAuthType = type;
            selectedAp.auth_type = type;
        } break;

        default:
            break;
    }
}

void rWifiConnect::rCleanUp()
{
    if (setupStep == NET_STEP_AP_BROWSE)
        resetScanEntries();
}

void rWifiConnect::rDoAPSetupMenu(unsigned short int** fnt)
{
    if (activeMenu)
        delete activeMenu;

    rResetConnectState(true);
    setupComplete = false;
    setupStep = NET_STEP_AP_MENU;
    selectedApValid = false;
    useFirmwareSettings = true;
    pendingAuthType = WlanBssAuthType_Open;
    memset(&selectedAp, 0, sizeof(selectedAp));
    memset(&authData, 0, sizeof(authData));

    activeMenu = new rMenu(NetworkSetupText[0], NetworkSetupText[1], 10, 10, MENU_NETWORK_SETUP, (uint16**)fnt);
    activeMenu->rAddOption(NetworkSetupText[2], NETWORK_BROWSE_AP);
    activeMenu->rAddOption(NetworkSetupText[4], NETWORK_USE_FIRMWARE);
    activeMenu->rAddOption(NetworkSetupText[5], D_CANCEL);
}

void rWifiConnect::rDoAPBrowseList(unsigned short int **fnt)
{
    if (activeListBox)
        delete activeListBox;

    if (wifi_init[0] == 0)
        initWifi();

    scanStarted = false;
    activeListBox = new rListBox(191, 10, LISTBOX_BROWSE_AP, fnt);
    activeListBox->rSetPopulateFunction(rWifiConnect::rScanForAP, 10);
}

void rWifiConnect::rDoAPSelectConfirmDialog(unsigned short int **fnt)
{
    char msg[200];
    char ssid[WLAN_MAX_SSID_LEN + 1];

    memcpy(ssid, selectedAp.ssid, selectedAp.ssid_len);
    ssid[(int)selectedAp.ssid_len] = '\0';
    sprintf(msg, "%s %s?", NetworkSetupText[6], selectedAp.ssid_len ? ssid : "<hidden>");

    if (activeDialog)
        delete activeDialog;

    activeDialog = new rDialog(msg, OKCancelText[0], OKCancelText[1], 10, 10, DIALOG_CHOSE_AP, (uint16**)fnt);
}

void rWifiConnect::rDoQueryWep(unsigned short int **fnt)
{
    rDoEnterWepKey(fnt);
}

void rWifiConnect::rDoAskWepMode(unsigned short int **fnt)
{
    rDoEnterWepKey(fnt);
}

void rWifiConnect::rDoEnterWepKey(unsigned short int **fnt)
{
    if (activeEditBox)
        delete activeEditBox;

    activeEditBox = new rEditBox(NetworkSetupText[13], OKCancelText[0], OKCancelText[1], 10, 10, 100, EDITBOX_INPUT_WEP, fnt);
}

void rWifiConnect::rDoEnterDNSPrimary(unsigned short int** fnt)
{
    rDoSetupCompleteDialog(fnt);
}

void rWifiConnect::rDoEnterDNSSecondary(unsigned short int** fnt)
{
    rDoSetupCompleteDialog(fnt);
}

void rWifiConnect::rDoAPManualEntry(unsigned short int **fnt)
{
    rDoAPSetupMenu(fnt);
}

void rWifiConnect::rDoChannelInvalidDialog(unsigned short int ** fnt)
{
    rDoAPSetupMenu(fnt);
}

void rWifiConnect::rDoWepInvalidDialog(unsigned short int **fnt)
{
    if (activeDialog)
        delete activeDialog;

    activeDialog = new rDialog(NetworkSetupText[14], OKCancelText[0], NULL, 10, 10, DIALOG_INVALID_WEP, (uint16**)fnt);
}

void rWifiConnect::rDoIPInvalidDialog(unsigned short int **fnt)
{
    rDoAPSetupMenu(fnt);
}

void rWifiConnect::rDoNetmaskInvalidDialog(unsigned short int **fnt)
{
    rDoAPSetupMenu(fnt);
}

void rWifiConnect::rDoGatewayInvalidDialog(unsigned short int **fnt)
{
    rDoAPSetupMenu(fnt);
}

void rWifiConnect::rDoDNSInvalidDialog(unsigned short int **fnt, int dns)
{
    currentDNS = dns;
    rDoAPSetupMenu(fnt);
}

void rWifiConnect::rDoEnterChannel(unsigned short int ** fnt)
{
    rDoAPSetupMenu(fnt);
}

void rWifiConnect::rDoAskIPMode(unsigned short int** fnt)
{
    rDoSetupCompleteDialog(fnt);
}

void rWifiConnect::rDoEnterUserIP(unsigned short int** fnt)
{
    rDoSetupCompleteDialog(fnt);
}

void rWifiConnect::rDoEnterUserNetmask(unsigned short int** fnt)
{
    rDoSetupCompleteDialog(fnt);
}

void rWifiConnect::rDoEnterUserGateway(unsigned short int** fnt)
{
    rDoSetupCompleteDialog(fnt);
}

void rWifiConnect::rDoSetupCompleteDialog(unsigned short int** fnt)
{
    if (activeDialog)
        delete activeDialog;

    activeDialog = new rDialog(NetworkSetupText[33], OKCancelText[0], NULL, 10, 10, DIALOG_SETUP_COMPLETE, (uint16**)fnt);
}

void rWifiConnect::rScanForAP(void *ptr)
{
    rTextDisplay *list = (rTextDisplay*)ptr;
    unsigned count = 0;
    WlanBssDesc *apList = NULL;
    int currentPos = activeListBox->rGetSelection();
    int currentLine = activeListBox->rGetCurrentLine();

    if (!scanStarted)
    {
        scanStarted = wfcBeginScan(&kScanFilter);
        if (!scanStarted)
            return;
    }

    apList = wfcGetScanBssList(&count);
    if (!apList)
        return;

    resetScanEntries();
    activeListBox->rDeleteAll();

    unsigned short **fnt = list->getFont();
    char nameString[200];

    if (count > D_CANCEL - 1)
        count = D_CANCEL - 1;

    for (unsigned i = 0; i < count; ++i)
    {
        WifiSelection *selection = new WifiSelection();
        const WlanBssAuthType type = authMaskToType(apList[i].auth_mask);
        const unsigned strength = wlanCalcSignalStrength(apList[i].rssi) * 33;
        char ssid[WLAN_MAX_SSID_LEN + 1];
        int len = 0;

        memcpy(&selection->bss, &apList[i], sizeof(selection->bss));

        if (apList[i].ssid_len > 0)
        {
            memcpy(ssid, apList[i].ssid, apList[i].ssid_len);
            ssid[(int)apList[i].ssid_len] = '\0';
        }
        else
        {
            strcpy(ssid, "<hidden>");
        }

        sprintf(nameString, "%u%%  (%s)  %s", strength, authLabel(type), ssid);

        for (int pos = 0; pos < (int)strlen(nameString); ++pos)
        {
            len += fnt[nameString[pos] - 32][0];
            if (len >= activeListBox->rGetWidth())
            {
                nameString[pos] = '\0';
                break;
            }
        }

        int listPos = activeListBox->rAddString(nameString);
        activeListBox->rInsertData(listPos, (void*)selection);
    }

    if (activeListBox->rGetTotal() > 0)
    {
        activeListBox->rSetCurrentLine(currentLine);
        if (currentPos >= activeListBox->rGetTotal())
            activeListBox->rSelectPos(activeListBox->rGetTotal() - 1, false);
        else
            activeListBox->rSelectPos(currentPos, false);
    }
}

WlanBssDesc rWifiConnect::selectedAp;
WlanAuthData rWifiConnect::authData;
WlanBssAuthType rWifiConnect::pendingAuthType = WlanBssAuthType_Open;
int rWifiConnect::setupStep = NET_STEP_NOTRUNNING;
int rWifiConnect::currentDNS = -1;
int rWifiConnect::connectPolls = 0;
bool rWifiConnect::setupComplete = false;
bool rWifiConnect::isConnecting = false;
bool rWifiConnect::sawConnectionProgress = false;
bool rWifiConnect::useFirmwareSettings = true;
bool rWifiConnect::selectedApValid = false;
bool rWifiConnect::scanStarted = false;
