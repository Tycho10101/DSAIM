/*---------------------------------------------------------------------------------

	Modern ARM7 core for libnds 2.x

---------------------------------------------------------------------------------*/
#include <calico.h>
#include <nds.h>

//---------------------------------------------------------------------------------
int main(void) {
//---------------------------------------------------------------------------------
	envReadNvramSettings();

	keypadStartExtServer();

	lcdSetIrqMask(DISPSTAT_IE_ALL, DISPSTAT_IE_VBLANK);
	irqEnable(IRQ_VBLANK);

	rtcInit();
	rtcSyncTime();

	pmInit();
	blkInit();

	touchInit();
	touchStartServer(80, MAIN_THREAD_PRIO);

	soundStartServer(MAIN_THREAD_PRIO - 0x10);
	micStartServer(MAIN_THREAD_PRIO - 0x18);
	wlmgrStartServer(MAIN_THREAD_PRIO - 8);

	while (pmMainLoop()) {
		threadWaitForVBlank();
	}

	return 0;
}
