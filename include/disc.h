#ifndef _DISC_H
#define _DISC_H

#define BC 	0x0000000100000100ULL
#define MIOS 	0x0000000100000101ULL

void LoadGame();
void LoadWiiGame();					// DVD Loader! HUGE thanks to crediar who did absolutely all the work.
int LoadGCGame();					// HUGE THANKS to emu_kidid who wrote this.
void SortList(u32 *list, int entries);
int compare(const void * a, const void * b);

#endif //_DISC_H

