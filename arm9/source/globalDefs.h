#ifndef _GLOBALS_
#define _GLOBALS_

#define sleep(x) for(int z_x_p=0; z_x_p < x; z_x_p++) swiWaitForVBlank()

#define USEWIFI
//#define USEDEBUG
//#define WIFIDEBUG

#define STATE_LOGIN                 0
#define STATE_AIM                   1

#define STATUS_NORMAL               0
#define STATUS_NOTOC                1
#define STATUS_DISCONNECT           2

namespace rKey
{
    enum
    {
        KEY_NONE            =       -1,
        KEY_SHIFT           =       -2,
        KEY_CAPS            =       -3,
        KEY_NAV_UP          =       -4,
        KEY_NAV_DOWN        =       -5,
        KEY_NAV_LEFT        =       -6,
        KEY_NAV_RIGHT       =       -7,

        KEY_BUDDY_IM        =       -7,
        KEY_BUDDY_ADD       =       -8,
        KEY_BUDDY_INFO      =       -9,
	
        KEY_LISTBOX_OK      =       -7,

        KEY_SEND            =       -8,
        KEY_BUDDY           =       -8,
        KEY_TEXT_SCROLLUP   =       -9,
        KEY_TEXT_SCROLLDOWN =       -10,

        KEY_CHATLIST_SCROLLUP =     -11,
        KEY_CHATLIST_SCROLLDOWN =   -12,
        KEY_BUDDYLIST_SCROLLUP  =   -11,
        KEY_BUDDYLIST_SCROLLDOWN =  -12,
        KEY_LISTBOX_SCROLLUP =      -11,
        KEY_LISTBOX_SCROLLDOWN =    -12,

        KEY_LIST_1ST        =       -13,
        KEY_LIST_2ND        =       -14,
        KEY_LIST_3RD        =       -15,
        KEY_LIST_4TH        =       -16,
        KEY_LIST_5TH        =       -17,
        KEY_LIST_6TH        =       -18,
        KEY_LIST_7TH        =       -19,
        KEY_LIST_8TH        =       -20,
        KEY_LIST_9TH        =       -21,
        KEY_LIST_10TH       =       -22,

        KEY_TRIGGER_R       =       -23,
        KEY_TRIGGER_L       =       -24,
        KEY_BUTTON_START    =       -25,
        KEY_BUTTON_SELECT   =       -26,
        KEY_DIRECTION_UP    =       -27,
        KEY_DIRECTION_DOWN  =       -28,
        KEY_SHIFT_DOWN      =       -29,
        KEY_SHIFT_UP        =       -30,
        KEY_WARN            =       -31,
        KEY_BLOCK           =       -32
    };
}

#define IBUFSIZE 1024

#define MAIN_SCREEN                 1
#define BUDDY_SCREEN	            2
#define PROFILE_SCREEN	            3
#define LOGIN_SCREEN	            4
#define MENU_SCREEN	            5
#define DIALOG_SCREEN	            6
#define LISTBOX_SCREEN	            7
#define EDITBOX_SCREEN	            8

#define COLOR_RED                   8
#define COLOR_BLUE                  9

#define D_OK                        1
#define D_CANCEL                    99
#define D_CONTINUE                  -1
#define D_ERROR                     -2

enum {
    MENU_CONNECT_TYPE = 1,
    MENU_ACCESS_POINTS,
    MENU_MAIN_OPTIONS,
    MENU_NETWORK_SETUP,
    MENU_NETWORK_ASK_WEP,
    MENU_IP_SETUP,
    
    DIALOG_DISCONNECT,
    DIALOG_GENERALERROR,
    DIALOG_SIGNOFF,
    DIALOG_CHOSE_AP,
    DIALOG_INVALID_CHANNEL,
    DIALOG_WEP_USE,
    DIALOG_INVALID_WEP,
    DIALOG_INVALID_IP,
    DIALOG_INVALID_NETMASK,
    DIALOG_INVALID_GATEWAY,
    DIALOG_INVALID_DNS,
    DIALOG_SETUP_COMPLETE,

    EDITBOX_AWAY,
    EDITBOX_SENDIM,
    EDITBOX_JOINCHAT,
    EDITBOX_GETINFO,
    EDITBOX_INPUT_AP,
    EDITBOX_INPUT_CHANNEL,
    EDITBOX_INPUT_WEP,
    EDITBOX_INPUT_USERIP,
    EDITBOX_INPUT_USERNETMASK,
    EDITBOX_INPUT_USERGATEWAY,
    EDITBOX_INPUT_DNS1,
    EDITBOX_INPUT_DNS2,

    LISTBOX_GETINFO,
    LISTBOX_BROWSE_AP,
    OPTION_CANCEL
};

#define CONNECT_TYPE_DHCP           1
#define CONNECT_TYPE_STATIC         2
#define CONNECT_TYPE_FIRMWARE       3

#define NETWORK_BROWSE_AP           0
#define NETWORK_INPUT_AP            1
#define NETWORK_USE_FIRMWARE        2
#define NETWORK_64BIT_WEP           3
#define NETWORK_128BIT_WEP          4

#define IP_STATIC                   0
#define IP_DHCP_STATIC_DNS          1
#define IP_DHCP_DHCP_DNS            2
#define DIALOG_DELAY	            6	
 
#define INSIDE(a,b,x1,y1,x2,y2)     ( (a > x1) && (a < x2) && (b > y1) && (b < y2) )

#endif
