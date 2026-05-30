#ifndef _WIFICONNECT_
#define _WIFICONNECT_

#include <dswifi9.h>
#include <wfc.h>
extern "C"{
void * sgIP_malloc(int size);
void sgIP_free(void * ptr);
void initWifi();
int autoConnect();
}

enum {
    NET_STEP_AP_MENU,
    NET_STEP_AP_BROWSE,
    NET_STEP_AP_CONFIRM,
    NET_STEP_AP_MANUAL,
    NET_STEP_AP_CHANNEL,
    NET_STEP_AP_CHANNEL_INVALID,
    NET_STEP_WEP_USE,
    NET_STEP_WEP_MODE,
    NET_STEP_WEP_KEY,
    NET_STEP_WEP_INVALID,
    NET_STEP_IP_MODE,
    NET_STEP_IP_USERIP,
    NET_STEP_IP_USERIP_INVALID,
    NET_STEP_IP_NETMASK,
    NET_STEP_IP_NETMASK_INVALID,
    NET_STEP_IP_GATEWAY,
    NET_STEP_IP_GATEWAY_INVALID,
    NET_STEP_DNS_PRIMARY,
    NET_STEP_DNS_SECONDARY,
    NET_STEP_DNS_INVALID,
    NET_STEP_COMPLETE,
    NET_STEP_CANCEL,
    NET_STEP_NOTRUNNING
};

class rWifiConnect
{
public:
    static int rAutoConnect() { return autoConnect(); }
    static void rInitWifi();
    static int rConnect();
    static bool rTestConnection();
    static void rScanForAP(void *);

    static void rSaveSettings(void *data);
    static void rCleanUp();
    static int  rCheckNetSetupStep(int, unsigned short int**, void *extra = NULL);
    static void rDoAPSetupMenu(unsigned short int**);
    static void rDoAPBrowseList(unsigned short int**);
    static void rDoAPSelectConfirmDialog(unsigned short int**);
    static void rDoAPManualEntry(unsigned short int **);
    static void rDoEnterChannel(unsigned short int **);
    static void rDoQueryWep(unsigned short int **);
    static void rDoAskWepMode(unsigned short int **);
    static void rDoEnterWepKey(unsigned short int **);
    static bool rVerifyWepKey(char *);
    static bool rVerifyChannel(char *);
    static void rDoChannelInvalidDialog(unsigned short int **);
    static void rDoWepInvalidDialog(unsigned short int **);
    static void rDoIPInvalidDialog(unsigned short int **);
    static void rDoNetmaskInvalidDialog(unsigned short int **);
    static void rDoGatewayInvalidDialog(unsigned short int **);
    static void rDoDNSInvalidDialog(unsigned short int **, int);
    
    static bool rVerifyIP(char *);
    
    static void rDoAskIPMode(unsigned short int**);
    static void rDoEnterUserIP(unsigned short int**);
    static void rDoEnterUserNetmask(unsigned short int**);
    static void rDoEnterUserGateway(unsigned short int**);

    static void rDoEnterDNSPrimary(unsigned short int**);
    static void rDoEnterDNSSecondary(unsigned short int**);

    static void rDoSetupCompleteDialog(unsigned short int**);

private:
    static void rResetConnectState(bool disconnectAp = false);
    static int  rTranslateStatus();

    static int setupStep;
    static int currentDNS;
    static int connectPolls;
    static bool setupComplete;
    static bool isConnecting;
    static bool sawConnectionProgress;
    static bool useFirmwareSettings;
    static bool selectedApValid;
    static bool scanStarted;
    static WlanBssDesc selectedAp;
    static WlanAuthData authData;
    static WlanBssAuthType pendingAuthType;

};
#endif
