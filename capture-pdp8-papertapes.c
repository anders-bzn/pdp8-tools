#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <stdbool.h>

// Define control codes and bit masks
#define CC_LEAD         0x80
#define CC_TRAIL        0x80
#define CC_ORIGIN       0x40
#define CC_FIELD        0xC0
#define CC_FIELD_MASK   0x1C
#define CC_DATA_MASK    0x3F
#define CC_CONTROL_MASK 0xC0

int set_interface_attribs(int fd, int speed)
{
	struct termios tty;

	if (tcgetattr(fd, &tty) < 0) {
		fprintf(stderr, "Error from tcgetattr: %s\n", strerror(errno));
		return -1;
	}

	cfsetospeed(&tty, (speed_t)speed);
	cfsetispeed(&tty, (speed_t)speed);
	tty.c_cflag |= (CLOCAL | CREAD);    /* ignore modem controls */
	tty.c_cflag &= ~CSIZE;
	tty.c_cflag |= CS8;         /* 8-bit characters */
	tty.c_cflag &= ~PARENB;     /* no parity bit */
	tty.c_cflag &= ~CSTOPB;     /* only need 1 stop bit */
	tty.c_cflag &= ~CRTSCTS;    /* no hardware flowcontrol */

	/* setup for non-canonical mode */
	tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
	tty.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
	tty.c_oflag &= ~OPOST;

	// Read with timeout, 1 s
	tty.c_cc[VMIN] = 0;
	tty.c_cc[VTIME] = 10;

	if (tcsetattr(fd, TCSANOW, &tty) != 0) {
			fprintf(stderr, "Error from tcsetattr: %s\n", strerror(errno));
			return -1;
	}
	return 0;
}

enum captureState_e {
	CS_START = 0,
	CS_LEAD_IN,
	CS_DATA_H,
	CS_DATA_L,
	CS_TRAIL,
	CS_DONE
};

void capture_raw(FILE *f, enum captureState_e *state, unsigned char c)
{
	// Just capture every byte recived until time out. Leave CS_START state to allow time out
	switch (*state) {
		case CS_START:
			*state = CS_LEAD_IN;
		case CS_LEAD_IN:
		case CS_DATA_H:
		case CS_DATA_L:
		case CS_TRAIL:
		case CS_DONE:
			fputc(c, f);
			break;
	}
}

void capture_rim(FILE *f, enum captureState_e *state, unsigned char c)
{
	static int leadinCount = 0;

	switch (*state) {
		case CS_START:
			if (c == CC_LEAD)
				leadinCount++;
			*state = CS_LEAD_IN;
			break;

		case CS_LEAD_IN:
			if (c == CC_LEAD) {
				leadinCount++;
			} else if (((c & CC_CONTROL_MASK) == CC_ORIGIN && leadinCount > 7)){
				while (leadinCount--) {
					fputc(CC_LEAD, f);
				}
				fputc(c, f);
				*state = CS_DATA_H;
			} else {
				leadinCount = 0;
			}
			break;

		case CS_DATA_H:
			if (c == 0x80) {
				*state = CS_TRAIL;
			}
			fputc(c, f);
			break;

		case CS_TRAIL:
			if (c != CC_TRAIL) {
				*state = CS_DONE;
			} else {
				fputc(c, f);
			}
			break;

		case CS_DONE:
			break;

		case CS_DATA_L:
			break;
	}
}

void capture_bin(FILE *f, enum captureState_e *state, unsigned char c)
{
	static int leadinCount = 0;
	static int csum = 0;
	static int c1 = 0;
	static int c2 = 0;

	switch (*state) {
		case CS_START:
			if (c == CC_LEAD)
				leadinCount++;
			*state = CS_LEAD_IN;
			break;

		case CS_LEAD_IN:
			if (c == CC_LEAD) {
				leadinCount++;
			} else if (((c & CC_CONTROL_MASK) == CC_ORIGIN && leadinCount > 7) ||
								 ((c & CC_CONTROL_MASK) == CC_FIELD && leadinCount > 7))
			{
				while (leadinCount--) {
					fputc(CC_LEAD, f);
				}

				// CC_FIELD is NOT used for checksum
				if ((c & CC_CONTROL_MASK) == CC_ORIGIN) {
					csum += c;
				}

				fputc(c, f);
				*state = CS_DATA_L;
			} else {
				leadinCount = 0;
			}
			break;

		case CS_DATA_L:
			fputc(c, f);

			// CC_TRAIL or CC_FIELD is not used in checksum calculation
			if ( !(c & 0x80)) {
				csum += c;
				c2 = c1;
				c1 = c;
			}

			if (c == 0x80) {
				int checksum = (c2 & 0x3f) << 6 | (c1 & 0x3f);
				csum = (csum - c1 - c2) & 0xfff;

				// Verify C-SUM
				if (csum  == checksum){
					printf("Checksum OK!: %4o\n", checksum);
				} else {
					printf("Checksum FAIL!: calc %4o <-> recv %4o\n", csum, checksum);
				}
				*state = CS_TRAIL;
			}
			break;

		case CS_TRAIL:
			if (c != CC_TRAIL) {
				*state = CS_DONE;
			} else {
				fputc(c, f);
			}
			break;

		case CS_DONE:
		case CS_DATA_H:
			break;
	}
}

void print_usage(char *name)
{
	fprintf(stdout, "Usage:\n");
	fprintf(stdout, "%s --[arg] [output filename]\n", name);
	fprintf(stdout, "  --bin   Capture binary format\n");
	fprintf(stdout, "  --rim   Capture rim format\n");
	fprintf(stdout, "  --raw   Capture all bytes, no check\n");
}


int main(int argc, char **argv)
{
	char *portname = "/dev/ttyUSB0";
	int fd;
	char *filename;
	FILE *fCapture;
	enum captureState_e state = CS_START;
	bool time_out = false;
	bool bin = false, rim = false, raw = false;

	if (argc != 2 && argc != 3) {
		print_usage(argv[0]);
		return -1;
	}

	if (argc > 1) {
		if (0 == strncmp(argv[1], "--bin", 5)) {
			bin = true;
		} else if (0 == strncmp(argv[1], "--rim", 5)) {
			rim = true;
		} else if (0 == strncmp(argv[1], "--raw", 5)) {
			raw = true;
		} else {
			print_usage(argv[0]);
			return -1;
		}
	}

	// File name specified, otherwise use default "out.bin"
	if (argc == 3) {
		filename = argv[argc-1];
	} else {
		filename = "out.bin";
	}

	fd = open(portname, O_RDWR | O_NOCTTY | O_SYNC);
	if (fd < 0) {
		fprintf(stderr, "Error opening device %s: %s\n", portname, strerror(errno));
		return -1;
	}

	/* Baudrate 1200, 8 bits, no parity, 1 stop bit */
	if (set_interface_attribs(fd, B1200) < 0) {
		return -1;
	}

	if ((fCapture = fopen(filename, "w")) == NULL) {
		fprintf(stderr, "Could not write to file \"%s\": %s\n", filename, strerror(errno));
		return -1;
	}

	/* simple noncanonical input */
	do {
		unsigned char buf[80];
		int rdlen;

		rdlen = read(fd, buf, sizeof(buf) - 1);
		if (rdlen > 0) {
			unsigned char *p;

			for (p = buf; rdlen-- > 0; p++) {
				if (bin) capture_bin(fCapture, &state, *p);
				if (rim) capture_rim(fCapture, &state, *p);
				if (raw) capture_raw(fCapture, &state, *p);
			}
			time_out = false;
		} else if (rdlen == 0) {
			time_out = true;
		} else {
			fprintf(stderr, "Error from read: %d: %s\n", rdlen, strerror(errno));
			time_out = true;
		}
	} while (state == CS_START ||
					 (state != CS_DONE && !time_out));
	close(fd);
	fclose(fCapture);
}
