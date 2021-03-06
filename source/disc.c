#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <ogcsys.h>
#include <gccore.h>
#include <stdarg.h>
#include <ctype.h>
#include <unistd.h>
#include <ogc/es.h>
#include <ogc/isfs.h>
#include <ogc/ipc.h>
#include <ogc/ios.h>
#include <ogc/dvd.h>
#include <ogc/wiilaunch.h>	
#include <wiiuse/wpad.h>
#include <fat.h>
#include <gcmodplay.h>

#include "screen.h"
#include "cleanup.h"
#include "rethandle.h"
#include "auth.h"
#include "general.h"
#include "disc.h"

#include "pretty.h"
#include "IOS_DVD.h"

static tikview gcviews ATTRIBUTE_ALIGN(32);

int compare(const void * a, const void * b)
{
	return (*(u32*)a - *(u32*)b);
}

void SortList(u32 *list, int entries)
{
	qsort(list,entries,sizeof(u32),	compare);
}

int LoadGCGame()		// HUGE THANKS to emu_kidid who wrote this.
{
	printf("\nGCBooter v1.0\n");
	
	CheckIPCRetval(DVDLowReset());
	dvddiskid *diskid = (dvddiskid *)0x80000000;
	CheckIPCRetval(DVDLowReadDiskID( diskid ));
	printf("Launching Game...\n");
	*(volatile unsigned int *)0xCC003024 |= 7;
	
	CheckESRetval(ES_GetTicketViews(BC, &gcviews, 1));
	CheckESRetval(ES_LaunchTitle(BC, &gcviews));
	while(1);
	return 0;
}

void LoadWiiGame()		// DVD Loader! HUGE thanks to crediar who did absolutely all the work.
{
	ioctlv *buffer=(void*)0x93000000;
	int game_partition_offset=0;

	void	(*app_init)(void (*report)(const char *fmt, ...));
	int		(*app_main)(void **dst, int *size, int *offset);
	void *	(*app_final)();
	void	(*entrypoint)();
	void	(*app_entry)(void(**init)(const char *fmt, ...), int (**main)(), void *(**final)());


	// TODO: Choose the correct IOS from the games TMD
	//printf("\n");
	//printf("Current IOS: %d\n", (*(vu32*)0x80003140)>>16);
	//printf("IOS_ReloadIOS(): %d\n", IOS_ReloadIOS(36));
	//printf("Current IOS: %d\n", (*(vu32*)0x80003140)>>16);

//	SystemMenuAuth();

	u32 TitleCount ATTRIBUTE_ALIGN(32);
	ES_GetNumTitles( &TitleCount );

	printf("\n\nGot Number of Titles!\n");

	u64 *title_list = (u64*)memalign( 32, sizeof(u64)*TitleCount );
	u64 *TitlesIDs = (u64*)memalign( 32, sizeof(u64)*TitleCount );
	u32 ios[TitleCount];
	u32 IOS_Chosen=36;

	printf("Aligned and allocated stuff!\n");

	memset( title_list, 0, sizeof(u64)*TitleCount );
	memset( TitlesIDs, 0, sizeof(u64)*TitleCount );

	printf("Cleaned the stuff to 0!\n");

	CheckESRetval(ES_GetTitles( title_list, TitleCount ));
	printf("Got titles!\n");

	int MAX_ENTRIES=MAX_IOS_ENTRIES;

	int ListOff=0;
	int ListEntries=0;
	int StartEntry=0;
	int cur_off=0;
	int i=0;
	int j=0;
	int eol=0; //End of list
	int redraw=1;
	while(1)
	{
		u32 TitleIDH ATTRIBUTE_ALIGN(32) = title_list[j+ListOff]>>32;
		u32 TitleIDL ATTRIBUTE_ALIGN(32) = title_list[j+ListOff];

		j++;

		if( j+ListOff > TitleCount )
		{
			printf("Done creating list!\n");
			sleep(2);
			break;
		}

		if(TitleIDH == 0x00000001 && TitleIDL > 0x00000002 && TitleIDL < 0x000000100)
			ios[ListEntries++]=TitleIDL;
		VIDEO_WaitVSync();
	}
	ListOff=StartEntry;
	eol=0;
	printf("Finished making list! List Entries %d\n\n\n", ListEntries);
	VIDEO_WaitVSync();
	if(ListEntries<=0)
	{
		printf("NO IOS AVAILABLE!!!\n\n\n");
		VIDEO_WaitVSync();
		sleep(20);
		return;
	}
	SortList(ios,ListEntries);
	for(i=0;i<ListEntries;i++)
	{
		if(ios[i]==0x00000024)		// default is IOS36
			cur_off=i;
	}
	while(1)
	{
		printf("\x1b[1;30H%d:%d:%d:%d     ", cur_off, ListOff, ListEntries, eol);
		WPAD_ScanPads();
		PAD_ScanPads();
		u32 WPAD_Pressed = WPAD_ButtonsDown(0);
		WPAD_Pressed |= WPAD_ButtonsDown(1);
		WPAD_Pressed |= WPAD_ButtonsDown(2);
		WPAD_Pressed |= WPAD_ButtonsDown(3);

		u32 PAD_Pressed  = PAD_ButtonsDown(0);
		PAD_Pressed  |= PAD_ButtonsDown(1);
		PAD_Pressed  |= PAD_ButtonsDown(2);
		PAD_Pressed  |= PAD_ButtonsDown(3);

		if ( (WPAD_Pressed & WPAD_BUTTON_B) || (PAD_Pressed & PAD_BUTTON_B) )
		{
			free(TitlesIDs);
			free(title_list);
			return;
		}

		if ( (WPAD_Pressed & WPAD_BUTTON_A) || (PAD_Pressed & PAD_BUTTON_A) )
		{
			IOS_Chosen=ios[ListOff];
			break;		}
		if ( (WPAD_Pressed & WPAD_BUTTON_DOWN) || (PAD_Pressed & PAD_BUTTON_DOWN) )
		{
			printf("\x1b[%d;0H  ", HEIGHT_BEGIN+cur_off);
			cur_off++;
			ListOff++;
			if(ListOff>=ListEntries)
			{
				eol=1;
				ListOff=ListEntries-1;
			}
			if( cur_off >= MAX_ENTRIES )
			{
				cur_off = MAX_ENTRIES-1;
				if( !eol )
					redraw=1;
			}
			if( cur_off >= ListEntries )
			{
				cur_off = ListEntries-1;
				if( !eol )
					redraw=1;
			}

			printf("\x1b[%d;0H->", HEIGHT_BEGIN+cur_off);
		} else if ( (WPAD_Pressed & WPAD_BUTTON_UP) || (PAD_Pressed & PAD_BUTTON_UP) )
		{
			printf("\x1b[%d;0H  ", HEIGHT_BEGIN+cur_off);
			cur_off--;
			ListOff--;
			if( cur_off < 0 )
			{
				cur_off=0;
				redraw=1;
			}
			if(ListOff<0)
				ListOff=0;
			if(ListOff>=ListEntries)
				eol=1;
			else
				eol=0;
			printf("\x1b[%d;0H->", HEIGHT_BEGIN+cur_off);
		}

		if( redraw )
		{
			ClearScreen();
			printf("\x1b[%d;0HChoose an IOS To load the game with:\n\n",HEIGHT_BEGIN-1);

			i=0;
			j=0;
//			ListEntries=0;
			if(ListOff-(MAX_ENTRIES-1) > 0)
				i=ListOff-(MAX_ENTRIES-1);
			else 
				i=ListOff;
			int x;
			for(x=0;i<ListEntries && x<MAX_ENTRIES;i++,x++)
				printf("\x1b[%d;0H   IOS% 3d (00000001-%08X)\n", i+HEIGHT_BEGIN, ios[i], ios[i]);

			printf("\x1b[%d;0H->", HEIGHT_BEGIN+cur_off);

			redraw = 0;
		}
		VIDEO_WaitVSync();
	}
	ClearScreen();
	printf("\x1b[2;0HLoading game with IOS%d!\n",IOS_Chosen);
	sleep(1);
	ClearScreen();
	int VideoMode;
	char *tab[] = {"Default", "NTSC", "PAL"};
	char cap[100]; sprintf(cap, "\n\nWhat video mode would you like to load your game in:");
	s32 res = showmenu(cap, tab, 3, 0, " ->");
	switch(res)
	{
		case 1:
			VideoMode=VIDEO_MODE_NTSC;
			break;
		case 2:
			VideoMode=VIDEO_MODE_PAL;
			break;
		default:
			VideoMode=VIDEO_MODE_DEFAULT;
			break;
	}
//	IOS_ReloadIOS(36);
	dbgprintf("DVDInit();\n");
	CheckIPCRetval(DVDInit());
	dbgprintf("DVDLowClosePartition();\n");
	CheckIPCRetval(DVDLowClosePartition());
	dbgprintf("DVDLowStopMotor();\n");
	CheckIPCRetval(DVDLowStopMotor());

	if( DVDLowGetCoverStatus() == DVD_COVER_OPEN )
	{
		printf("No Disc Inserted! Please insert a disc...\nReturning to main menu.");
		sleep(4);
		CheckIPCRetval(IOS_Close(DVDGetHandle()));
		return;
	}
	printf("Disc found! Launching...\n");

	dbgprintf("IOS_ReloadIOS();\n");
	CheckIOSRetval(IOS_ReloadIOS(IOS_Chosen));
	dbgprintf("DVDLowReset();\n");
	CheckIPCRetval(DVDLowReset());
	dbgprintf("DVDLowReadDiskID();\n");
	CheckIPCRetval(DVDLowReadDiskID( (void*)0x80000000 ));
	dbgprintf("DVDLowUnencryptedRead();\n");
	CheckIPCRetval(DVDLowUnencryptedRead( buffer, 0x20, 0x40000>>2));

	DCFlushRange(buffer, 0x100);

	int partitions = ((u32*)buffer)[0];
	int partition_info_offset = ((u32*)buffer)[1] << 2;

	dbgprintf("DVDLowUnencryptedRead();\n");
	CheckIPCRetval(DVDLowUnencryptedRead( buffer, 0x20, partition_info_offset>>2 ));

	for( i=0; i < partitions; i++)
		if( ((u32*)buffer)[i*2+1] == 0 )
			game_partition_offset = ((u32*)buffer)[i*2]<<2;

	//let's open the partition

	((u32*)buffer)[(0x40>>2)]	=  0x8B000000;
	((u32*)buffer)[(0x40>>2)+1]	=  game_partition_offset>>2;

	//in
	((u32*)buffer)[0x00] = *((unsigned long *)(0x7FFFFFFF & ((unsigned long)(buffer+0x40))));
	((u32*)buffer)[0x01] = 0x20;					//0x04
	((u32*)buffer)[0x02] = 0;						//0x08
	((u32*)buffer)[0x03] = 0x2A4;					//0x0C 
	((u32*)buffer)[0x04] = 0;						//0x10
	((u32*)buffer)[0x05] = 0;						//0x14

	//out
	((u32*)buffer)[0x00] = *((unsigned long *)(0x7FFFFFFF & ((unsigned long)(buffer+0x380))));
	((u32*)buffer)[0x07] = 0x49E4;					//0x1C
	((u32*)buffer)[0x00] = *((unsigned long *)(0x7FFFFFFF & ((unsigned long)(buffer+0360))));
	((u32*)buffer)[0x09] = 0x20;					//0x24

	DCFlushRange(buffer, 0x100);

	CheckIPCRetval(IOS_Ioctlv( DVDGetHandle(), 0x8B, 3, 2, buffer));

	CheckIPCRetval(DVDLowRead( buffer, 0x40, 0x20>>2));
	
	CheckIPCRetval(DVDLowRead( buffer, 0x20, 0x2440>>2));

	CheckIPCRetval(DVDLowRead((void*)0x81200000, *(unsigned long*)(buffer+0x14)+*(unsigned long*)(buffer+0x18), 0x2460>>2));

	app_entry = (void*)(buffer+0x10);
	app_entry((void*)&app_init, &app_main, &app_final);
	app_init((void*)printf);

	void *dst=0;
	int lenn, offset;

	while (1)
	{
		lenn=0;
		offset=0;
		dst=0;
		if( app_main(&dst, &lenn, &offset) != 1)
			break;

		CheckIPCRetval(DVDLowRead( dst, lenn, offset ));
		DCFlushRange(dst, lenn);
	}

	CheckIPCRetval(IOS_Close(DVDGetHandle()));
	__IOS_ShutdownSubsystems();
	
	entrypoint = app_final();
	printf("Entrypoint set.\n");

	*(vu32*)0x80000020 = 0x0D15EA5E;				// Magic word (how did the console boot?)
	*(vu32*)0x80000024 = 0x00000001;				// Version (usually set to 1 by apploader)
	*(vu32*)0x80000028 = 0x01800000;				// physical Memory Size
	*(vu32*)0x8000002C = 0x00000023;				// Console type
	*(vu32*)0x80000030 = 0x00000000;				// ArenaLo 
	*(vu32*)0x80000034 = 0x9000FFC0;				// ArenaHi 
	*(vu32*)0x80000040 = 0x00000000;				// Debugger present?
	*(vu32*)0x80000044 = 0x00000000;				// Debugger Exception mask
	*(vu32*)0x80000048 = 0x00000000;				// Exception hook destination 
	*(vu32*)0x8000004C = 0x00000000;				// Temp for LR
	*(vu32*)0x80000050 = 0x00000000;				// padding zeros
	*(vu32*)0x80000060 = 0x38A00040;				// Exception init
	*(vu32*)0x800000E4 = 0x8008F7B8;				// Thread Init
	*(vu32*)0x800000CC = 0x00000005;				// VIInit
	*(vu32*)0x800000F0 = 0x01800000;				// Console Simulated Memory Size
	*(vu32*)0x800000F4 = 0x00000000;				// DVD BI2 location in main memory
	*(vu32*)0x800000F8 = 0x0E7BE2C0;				// Bus Clock Speed#define DUMP_BUF_SIZE 256
	*(vu32*)0x800000FC = 0x2B73A840;				// CPU Clock Speed

	memcpy((void*)0x80003180,(void*)0x80000000,4);

	if(VideoMode==VIDEO_MODE_DEFAULT)
	{
		printf("Switching to: ");
		switch(*(vu8*)0x80000003)
		{
			case 'P':
					printf("PAL60\n");
					*(vu32*)0x800000CC = 0x00000005;
				break;
			case 'J':
			case 'U':
					printf("NTSC\n");
					*(vu32*)0x800000CC = 0x00000001;
				break;
			default:
					printf("AUTO\n");
					*(vu32*)0x800000CC = 0x00000000;
				break;
		}
	}else if(VideoMode==VIDEO_MODE_PAL)
	{
		printf("Switching to: PAL60\n");
		*(vu32*)0x800000CC = 0x00000005;
	}else if(VideoMode==VIDEO_MODE_NTSC)
	{
		printf("Switching to: NTSC\n");
		*(vu32*)0x800000CC = 0x00000001;
	}		
	*(vu32*)0xCC003024 = 0x00000007;
	*(vu32*)0xC00030F0 = 0x0000001C;
	*(vu32*)0xC000318C = 0x00000000;
	*(vu32*)0xC0003190 = 0x00000000;
	//sleep(10);

	printf("Loading entrypoint\n");
	WPAD_Disconnect(0);
	WPAD_Shutdown();
	ISFS_Deinitialize();
	__ES_Close();

	entrypoint();
}

void LoadGame()
{
	CheckIPCRetval(DVDInit());
	CheckIPCRetval(DVDLowClosePartition());
	CheckIPCRetval(DVDLowStopMotor());
	ClearScreen();
	if( DVDLowGetCoverStatus() == DVD_COVER_OPEN )
	{
		printf("\n\nNo Disc Inserted! Please insert a disc...\nReturning to main menu.");
		sleep(4);
		IOS_Close(DVDGetHandle());
		return;
	}
	printf("\n\nDisc found!\n");

	CheckIPCRetval(DVDLowReset());
	dvddiskid *diskid = (dvddiskid *)0x80000000;
	CheckIPCRetval(DVDLowReadDiskID( diskid ));
	u8 disc_magic[4];
	DVDLowUnencryptedRead(disc_magic, 4, 0x4FFFC);
	if (diskid->gamename[0] == 'G')
	{
		printf("Disc is gamecube\n");
		printf("Magic is %02x%02x%02x%02x\n", disc_magic[0], disc_magic[1], disc_magic[2], disc_magic[3]);sleep(2);
		LoadGCGame();
	} else {
		if(diskid->gamename[0] == 'R' || diskid->gamename[0] == '0' || diskid->gamename[0] == '\x00' || diskid->gamename[0] == '4' || diskid->gamename[0] == '\x04')
			printf("Disc is Wii\n");
		printf("Magic is %02x%02x%02x%02x\n", disc_magic[0], disc_magic[1], disc_magic[2], disc_magic[3]);sleep(2);
		LoadWiiGame();
	}
}

