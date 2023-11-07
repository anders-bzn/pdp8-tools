/*
 * A little program to parse the two boot ROM's on M8317 in a PDP-8A
 *
 * Written in 2018 by Anders Sandahl.
 *
 */

#include <stdio.h>

int read_rom_file (char *filename, int *buff)
{
	FILE *fp;
	int i=0, ch;

	fp = fopen(filename, "r");
	if (fp == NULL) {
		fprintf(stderr, "Could not open file: %s\n", filename);
		return -1;
	}

	while ( (ch = getc(fp)) != EOF ){
		if (i > 255) {
			fprintf(stderr, "%s: Only 256 bytes expected.\n", filename);
			return -1;
		}
		buff[i] = ch;
		i++;
	}
	fclose(fp);
	return 0;
}


int main(int argc, char *argv[])
{
	int i=0;
	int buff_prom1[256];
	int buff_prom2[256];
	int addr = 0;
	int ext_addr = 0;

	if (argc != 3) {
		fprintf(stderr, "Usage: %s [boot ROM #1 filename] [boot ROM #2 filename]\n", argv[0]);
		fprintf(stderr, "Takes two PDP-8A M8317 boot ROM files, parse them and dump the content\n");
		return -1;
	}

	if (read_rom_file(argv[1], buff_prom1) < 0) {
		return -1;
	}

	if (read_rom_file(argv[2], buff_prom2) < 0) {
		return -1;
	}

	for (i=0; i<256; i+=2){
		int data = (buff_prom2[i] << 8) |
				   (buff_prom1[i+1] << 4) |
				   (buff_prom2[i+1]);
		int opr = buff_prom1[i];

		printf("%4.4x %4.4o " ,i,i);
		printf(":%c%c%c%c ", opr & 8 ? 'A': ' ',
							 opr & 4 ? 'E': ' ',
							 opr & 2 ? 'D': ' ',
							 opr & 1 ? 'S': ' ');

		if (opr & 8) addr = data;
		if (opr & 4) ext_addr = data & 7;
		if (opr & 2 ) {
			printf("%1.1o%4.4o ", ext_addr, addr++);
		} else {
			printf("      ");
		}
		printf(": %4.4o\n", data);
	}
}
