/*
 * Copyright (C) 2016 FIX94
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#include <gccore.h>
#include <stdio.h>
#include <unistd.h>
#include <malloc.h>
extern void __SYS_ReadROM(void *buf,u32 len,u32 offset);
void none(){};
volatile bool myres = false;
void exitOut()
{
	VIDEO_WaitVSync();
	VIDEO_WaitVSync();
	CARD_Unmount(0);
	printf("Restarting console\n");
	sleep(5);
	SYS_ResetSystem(SYS_HOTRESET,0,0);
}
void fuckingcb(int chan, int res)
{
	myres = true;
	printf("%i\n",res);
}
int main(int argc, char *argv[])
{
	void *xfb = NULL;
	GXRModeObj *rmode = NULL;
	VIDEO_Init();
	rmode = VIDEO_GetPreferredMode(NULL);
	xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
	VIDEO_Configure(rmode);
	VIDEO_SetNextFramebuffer(xfb);
	VIDEO_SetBlack(FALSE);
	VIDEO_Flush();
	VIDEO_WaitVSync();
	if(rmode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();
	int x = 24, y = 32, w, h;
	w = rmode->fbWidth - (32);
	h = rmode->xfbHeight - (48);
	CON_InitEx(rmode, x, y, w, h);
	VIDEO_ClearFrameBuffer(rmode, xfb, COLOR_BLACK);
	PAD_Init();
	printf("GC IPL Dumper by FIX94\n");
	VIDEO_WaitVSync();
	u8 *iplMem = memalign(32,0x200000);
	__SYS_ReadROM(iplMem,0x200000,0);
	void *cardArea = memalign(32,CARD_WORKAREA);
	CARD_Init("DOLX","00");
	printf("Trying to mount CARD...\n");
	VIDEO_WaitVSync();
	while(CARD_Mount(0,cardArea,none) < 0) sleep(1);
	u32 ssize;
	CARD_GetSectorSize(0,&ssize);
	printf("Sector size: %i\n",ssize);
	VIDEO_WaitVSync();
	card_file f;
	if(CARD_Open(0,"ipl.bin",&f) == 0)
	{
		printf("File already exists!\n");
		CARD_Close(&f);
		exitOut();
	}
	if(CARD_Create(0,"ipl.bin",0x200000,&f) == 0)
	{
		printf("Writing full ipl...\n");
		VIDEO_WaitVSync();
		u32 i;
		for(i = 0; i < 0x200000; i+= ssize)
			CARD_Write(&f,iplMem+i,ssize,i);
		CARD_Close(&f);
		printf("All done!\n");
		exitOut();
	}
	else if(CARD_Create(0,"ipl_part1.bin",0x100000,&f) == 0)
	{
		printf("Writing ipl part 1 of 2...\n");
		VIDEO_WaitVSync();
		u32 i;
		for(i = 0; i < 0x100000; i+= ssize)
			CARD_Write(&f,iplMem+i,ssize,i);
		CARD_Close(&f);
		CARD_Unmount(0);
		printf("Done, please use GCMM on a different console to copy the file over.\n");
		printf("After you copied it over delete it from the memory card, reinsert it and press any button to write part 2\n");
		while(1)
		{
			VIDEO_WaitVSync();
			if(PAD_ScanPads()&1 && (PAD_ButtonsUp(0) || PAD_ButtonsDown(0) || PAD_ButtonsHeld(0)))
				break;
		}
		printf("Trying to mount CARD...\n");
		VIDEO_WaitVSync();
		while(CARD_Mount(0,cardArea,none) < 0) sleep(1);
		if(CARD_Create(0,"ipl_part2.bin",0x100000,&f) != 0)
		{	
			printf("ERROR:Unable to create part 2!\n");
			exitOut();
		}
		printf("Writing ipl part 2 of 2...\n");
		VIDEO_WaitVSync();
		for(i = 0; i < 0x100000; i+= ssize)
			CARD_Write(&f,iplMem+0x100000+i,ssize,i);
		CARD_Close(&f);
	}
	else
	{
		printf("IPL could not be created, please make sure you have at least 128 blocks free!\n");
		exitOut();
	}
	printf("All done!\n");
	exitOut();
	return 0;
}