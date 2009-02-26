#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <ogcsys.h>
#include <gccore.h>
//#include <update.h>
#include <network.h>
/*
#include "updater.h"
*/
void update_WiiMU_file(char* srcfile, char* dstfile)
{
/*	char filestring[255];
	sprintf(filestring, "/~squidman/wii/updates/wiimu/%s", srcfile);
	load_network();
	s32 main_server = connect_to_server("74.86.133.219");
	FILE *f = fopen(get_location(dstfile, "/apps/WiiMU"), "wb+");
	send_message(main_server, filestring, "www.wiibrew.exofire.net");
	instructions_update();
	get_file(main_server, f);
	fclose(f);
	net_close(main_server);*/
}

void update_WiiMU_dol()
{
	update_WiiMU_file("boot.dol", "boot.dol");
}

void update_WiiMU_elf()
{
	update_WiiMU_file("boot.elf", "boot.elf");
}

void update_WiiMU_icon()
{
	update_WiiMU_file("icon.png", "icon.png");
}

void update_WiiMU_xml()
{
	update_WiiMU_file("meta.xml", "meta.xml");
}

void update_WiiMU()
{
	update_WiiMU_icon();
	update_WiiMU_xml();
	update_WiiMU_dol();
}

