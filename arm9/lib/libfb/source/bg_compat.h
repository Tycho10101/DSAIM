#pragma once
#include <nds.h>

// MAIN engine BG2 affine registers
#define BG2_CR   (*(vu16*)0x0400000C)
#define BG2_XDX  (*(vs16*)0x04000020)
#define BG2_XDY  (*(vs16*)0x04000022)
#define BG2_YDX  (*(vs16*)0x04000024)
#define BG2_YDY  (*(vs16*)0x04000026)
#define BG2_CX   (*(vs32*)0x04000028)
#define BG2_CY   (*(vs32*)0x0400002C)

// MAIN engine BG3 affine registers
#define BG3_CR   (*(vu16*)0x0400000E)
#define BG3_XDX  (*(vs16*)0x04000030)
#define BG3_XDY  (*(vs16*)0x04000032)
#define BG3_YDX  (*(vs16*)0x04000034)
#define BG3_YDY  (*(vs16*)0x04000036)
#define BG3_CX   (*(vs32*)0x04000038)
#define BG3_CY   (*(vs32*)0x0400003C)

// SUB engine BG2 affine registers
#define SUB_BG2_CR   (*(vu16*)0x0400100C)
#define SUB_BG2_XDX  (*(vs16*)0x04001020)
#define SUB_BG2_XDY  (*(vs16*)0x04001022)
#define SUB_BG2_YDX  (*(vs16*)0x04001024)
#define SUB_BG2_YDY  (*(vs16*)0x04001026)
#define SUB_BG2_CX   (*(vs32*)0x04001028)
#define SUB_BG2_CY   (*(vs32*)0x0400102C)
