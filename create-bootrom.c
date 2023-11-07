/*
 * A little program to create the two boot ROM's on M8317 in a PDP-8A
 *
 * Written in 2018 by Anders Sandahl.
 *
 */
#include <stdio.h>
#include <string.h>

#define TRAIL 0x80
#define FIELD 0xC0
#define ORIGIN 0x40
#define DATA 0x00
#define MASK  0xC0

#define ROM_LOADADDR  0x8
#define ROM_LOADEX    0x4
#define ROM_DEPOSIT   0x2
#define ROM_START     0x1

struct romData {
	int cmd;
	int data;
};

struct romData autoStart[] = {
	{ROM_LOADADDR,            00000},
	{ROM_START | ROM_LOADEX,  00000},
	{ROM_LOADADDR,            00200},
	{ROM_START | ROM_LOADEX,  00000},
	{ROM_LOADADDR,            02000},
	{ROM_START | ROM_LOADEX,  00000},
	{ROM_LOADADDR,            04200},
	{ROM_START | ROM_LOADEX,  00000}};

void write_entry (struct romData data, FILE *out1, FILE *out2)
{
	// Even address
	fputc(data.cmd, out1);
	fputc((data.data >> 8) & 0xf , out2);
	// Odd address
	fputc((data.data >> 4) & 0xf, out1);
	fputc(data.data & 0xf , out2);
}

int main(void)
{
	FILE *fp, *out1, *out2;
	int i=0, n=0, ch, ch1;
	int checksum = 0;
	struct romData bootloader[128];

	memset (bootloader, 0, sizeof bootloader);

	fp = fopen("bootloader.bin", "r");

	while ( (ch = getc(fp)) != EOF ){
		switch (ch & MASK) {
		case TRAIL:
			printf("L/T");
			break;
		case FIELD:
			printf("E-----%d\n", ch & 0x7);
			checksum += ch;
			break;
		case ORIGIN:
			ch1 = getc(fp);
			checksum += ch1;
			checksum += ch;
			bootloader[n].cmd = ROM_LOADADDR;
			bootloader[n++].data = (ch & 0x3f) << 6 | (ch1 & 0x3f);
			bootloader[n].cmd = ROM_LOADEX;
			bootloader[n++].data = 0;
			printf("A %4.4o\n", (ch & 0x3f) << 6 | (ch1 & 0x3f));
			break;
		case DATA:
			ch1 = getc(fp);
			checksum += ch1;
			checksum += ch;
			bootloader[n].cmd = ROM_DEPOSIT;
			bootloader[n++].data = (ch & 0x3f) << 6 | (ch1 & 0x3f);
			printf("D %4.4o %4.4o\n", (ch & 0x3f) << 6 | (ch1 & 0x3f), checksum);
			break;
		}
	}
	fclose(fp);

	out1 = fopen("rom1.bin", "w");
	out2 = fopen("rom2.bin", "w");

	// Write autostart data
	for (i=0; i < sizeof(autoStart) / (2 * sizeof(int)) ; i++)
		write_entry (autoStart[i], out1, out2);

	//Strip checksum
	n--;

	bootloader[n].cmd = ROM_LOADADDR | ROM_START;
	bootloader[n++].data = 020; //HARD START ADDRESS (should be in param)

	for (i=8; i < 128 ; i++)
		write_entry (bootloader[i-8], out1, out2);

	fclose (out1);
	fclose (out2);
}
