#ifndef _WIFIDEBUG_
#define _WIFIDEBUG_
class rWifiDebug
{
public:
    static bool rConnect(char *, int);
    static bool rPrint(char *, ...);
    static void rClose();
    static void rInit();
private:
    static char dbuffer[5000];
    static bool isWifiConnect;
    static int dsock;
};
  
#endif 
