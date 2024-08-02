/*
 * Program for capturing PDP-8 papertapes
 *
 * Licence GPL 2.0
 *
 * By Anders Sandahl 2024
 *
 */

#include <argp.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <stdbool.h>


const char *argp_program_version =
    "capture-papertape 0.99";

const char *argp_program_bug_address =
    "<anders@abc80.net>";


/* Program documentation. */
static char doc[] =
    "Capture program for PDP-8 papertapes, takes input from serial port and saves it to a file." \
    "PDP-8 rim and bin formats can be validated. Default is 9600 8N1 on device /dev/ttyUSB0.";


/* Options to be parsed. */
static struct argp_option options[] = {
    {"device",          'd', "DEV",         OPTION_ARG_OPTIONAL, "Serial device, /dev/ttyXXX"},
    {"bits",            'b', "5,6,7,8",     OPTION_ARG_OPTIONAL, "Number of data bits"},
    {"parity",          'p', "N,E,O,M",     OPTION_ARG_OPTIONAL, "Parity"},
    {"stop",            'S', "1,2",         OPTION_ARG_OPTIONAL, "Number of stop bits"},
    {"speed",           's', "BAUD",        OPTION_ARG_OPTIONAL, "Serial com speed"},
    {"handshake",       'h', 0,             OPTION_ARG_OPTIONAL, "Use RTS/CTS handshake"},
    {"format",          'F', "raw/rim/bin", OPTION_ARG_OPTIONAL, "Capture papertape format"},
    {"strip-lead-in",   'x', "0xXX",        OPTION_ARG_OPTIONAL, "Strip lead in chars, just add 16 bytes to get constant start pattern"},
    {"filename",        'f', "FILE",        OPTION_ARG_OPTIONAL, "Dump received data to file"},
    { 0 }
};


speed_t map_baudrate(int baud){
    speed_t speed;

    switch (baud) {
    case 110:
        speed = B110;
        break;
    case 150:
        speed = B150;
        break;
    case 200:
        speed = B200;
        break;
    case 300:
        speed = B300;
        break;
    case 600:
        speed = B600;
        break;
    case 1200:
        speed = B1200;
        break;
    case 1800:
        speed = B1800;
        break;
    case 2400:
        speed = B2400;
        break;
    case 4800:
        speed = B4800;
        break;
    case 9600:
        speed = B9600;
        break;
    case 19200:
        speed = B19200;
        break;
    case 38400:
        speed = B38400;
        break;
    case 57600:
        speed = B57600;
        break;
    case 115200:
        speed = B115200;
        break;
    case 230400:
        speed = B230400;
        break;
    case 460800:
        speed = B460800;
        break;
    case 500000:
        speed = B500000;
        break;
    case 576000:
        speed = B576000;
        break;
    case 921600:
        speed = B921600;
        break;
    case 1000000:
        speed = B1000000;
        break;
    case 11520000:
        speed = B1152000;
        break;
    case 15000000:
        speed = B1500000;
        break;
    case 20000000:
        speed = B2000000;
    default:
        speed = -1;
    }
    return speed;
}

enum tape_format {
    TF_RAW,
    TF_BIN,
    TF_RIM,
};


/* Used by main to communicate with parse_opt. */
struct argp_arguments
{
    char *device;
    char *file;
    int bits;
    char parity;
    int stop_bits;
    bool handshake;
    enum tape_format format;
    speed_t speed;
    int leadin_strip;
};


/* Parse a single option. */
static error_t
parse_opt (int key, char *arg, struct argp_state *state)
{
    /* Get the input argument from argp_parse, which we
    know is a pointer to our arguments structure. */
    struct argp_arguments *arguments = state->input;

    switch (key){
    case 'd':
        arguments->device = arg;
        break;
    case 'f':
        arguments->file = arg;
        break;
    case 'b':
        if (arg != NULL) {
            switch (arg[0]){
            case '5':
                arguments->bits = 5;
                break;
            case '6':
                arguments->bits = 6;
                break;
            case '7':
                arguments->bits = 7;
                break;
            case '8':
                arguments->bits = 8;
                break;
            default:
                fprintf(stderr, "Error, invalid number of bits: %s\n", arg);
                argp_usage (state);
                return ARGP_ERR_UNKNOWN;
                break;
            break;
            }
        }
        break;
    case 'p':
        if (arg != NULL) {
            switch (arg[0]){
            case 'N':
            case 'O':
            case 'E':
            case 'M':
                arguments->parity = arg[0];
                break;
            default:
                fprintf(stderr, "Error, invalid parity (N/O/E/M): %s\n", arg);
                argp_usage (state);
                return ARGP_ERR_UNKNOWN;
                break;
            break;
            }
        }
        break;
    case 'S':
        if (arg != NULL) {
            switch (arg[0]){
            case '1':
                arguments->stop_bits = 1;
                break;
            case '2':
                arguments->stop_bits = 2;
                break;
            default:
                fprintf(stderr, "Error, invalid number of stop bits: %s\n", arg);
                argp_usage (state);
                return ARGP_ERR_UNKNOWN;
                break;
            }
        }
        break;
    case 's':
        if (arg != NULL) {
            int baud = atoi(arg);

            arguments->speed = map_baudrate(baud);

            if (arguments->speed == -1) {
                fprintf(stderr, "Invalid baudrate: %d\n", baud);
                argp_usage (state);
                return ARGP_ERR_UNKNOWN;
            }
        } else {
            argp_usage (state);
            return ARGP_ERR_UNKNOWN;
        }
        break;
    case 'h':
        arguments->handshake = true;
        break;
    case 'F':
        if (arg != NULL && (0 == strncmp(arg, "bin", 3))) {
            arguments->format = TF_BIN;
        } else if (arg != NULL && (0 == strncmp(arg, "rim", 3))) {
            arguments->format = TF_RIM;
        } else if (arg != NULL && (0 == strncmp(arg, "raw", 3))) {
            arguments->format = TF_RAW;
        } else {
            fprintf(stderr, "Invalid format: %s\n", arg == NULL ? "NONE" : arg);
            argp_usage (state);
            return ARGP_ERR_UNKNOWN;
        }
        break;
    case 'x':
        if (arg != NULL) {
            sscanf(arg, "0x%x", &arguments->leadin_strip);

            if (arguments->leadin_strip < 0x00 || arguments->leadin_strip > 0xff) {
                fprintf(stderr, "Invalid lead in, must be 0x00 - 0xff: %s\n", arg);
                argp_usage (state);
                return ARGP_ERR_UNKNOWN;
            }
        } else {
            fprintf(stderr, "Invalid lead in, must be 0x00 - 0xff: %s\n", arg == NULL ? "NONE" : arg);
            argp_usage (state);
            return ARGP_ERR_UNKNOWN;
        }
        break;

    case ARGP_KEY_ARG:
        if (state->arg_num != 0){
        }
        break;

    case ARGP_KEY_END:
        if (state->arg_num != 0){
            argp_usage (state);
            return ARGP_ERR_UNKNOWN;
        }
        break;

    default:
        return ARGP_ERR_UNKNOWN;
    }

    return 0;
}


static struct argp argp = { options, parse_opt, NULL, doc };


int set_interface_attribs(int fd, speed_t speed, char parity, int bits, int stop_bits, bool handshake)
{
    struct termios tty;

    if (tcgetattr(fd, &tty) < 0) {
        fprintf(stderr, "Error from tcgetattr: %s\n", strerror(errno));
        return -1;
    }

    cfsetspeed(&tty, speed);
    tty.c_cflag |= (CLOCAL | CREAD | PARENB);    /* ignore modem controls */
    tty.c_cflag &= ~CSIZE;

    if (bits == 5)
        tty.c_cflag |= CS5;         /* 5-bit characters */
    if (bits == 6)
        tty.c_cflag |= CS6;         /* 6-bit characters */
    if (bits == 7)
        tty.c_cflag |= CS7;         /* 7-bit characters */
    else if (bits == 8)
        tty.c_cflag |= CS8;         /* 8-bit characters */
    else
        return -1;

    if (parity == 'N') {
        tty.c_cflag &= ~PARENB;     /* no parity */
    } else if (parity == 'E') {
        tty.c_cflag |= PARENB;      /* parity */
        tty.c_cflag &= ~PARODD;     /* even parity */
    } else if (parity == 'O') {
        tty.c_cflag |= PARENB;      /* parity */
        tty.c_cflag |= PARODD;      /* even parity */
    } else return -1;

    if (stop_bits == 1)
        tty.c_cflag &= ~CSTOPB;     /* 1 stop bit */
    else
        tty.c_cflag |= CSTOPB;      /* 2 stop bits */

    if (handshake) {
        tty.c_cflag |= CRTSCTS;     /* hardware flowcontrol */
    } else {
        tty.c_cflag &= ~CRTSCTS;    /* no hardware flowcontrol */
    }

    /* setup for non-canonical mode */
    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
    tty.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    tty.c_oflag &= ~OPOST;

    /* Read with timeout, 1 s */
    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 10;

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        fprintf(stderr, "Error from tcsetattr: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}


/* Define control codes and bit masks */
#define CC_LEAD         0x80
#define CC_TRAIL        0x80
#define CC_ORIGIN       0x40
#define CC_FIELD        0xC0
#define CC_FIELD_MASK   0x1C
#define CC_DATA_MASK    0x3F
#define CC_CONTROL_MASK 0xC0


enum captureState_e {
    CS_START = 0,
    CS_LEAD_IN,
    CS_DATA_H,
    CS_DATA_L,
    CS_TRAIL,
    CS_DONE
};


void capture_raw(FILE *f, enum captureState_e *state, unsigned char c, int strip_char)
{
    /*
     * Just capture every byte recived until time out. Leave CS_START state to allow time out
     */
    int i = 16;

    switch (*state) {
        case CS_START:
            if (strip_char != c) {
                if (strip_char != -1)
                    while(i--)
                        fputc(strip_char, f);

                *state = CS_LEAD_IN;
            }
            break;
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

                /* CC_FIELD is NOT used for checksum */
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

            /* CC_TRAIL or CC_FIELD is not used in checksum calculation */
            if ( !(c & 0x80)) {
                csum += c;
                c2 = c1;
                c1 = c;
            }

            if (c == 0x80) {
                int checksum = (c2 & 0x3f) << 6 | (c1 & 0x3f);
                csum = (csum - c1 - c2) & 0xfff;

                /* Verify C-SUM */
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


int main(int argc, char **argv)
{
    int fd;
    FILE *fCapture;
    enum captureState_e state = CS_START;
    bool time_out = false;
    struct argp_arguments args;

    args.bits = 8;
    args.parity = 'N';
    args.stop_bits = 1;
    args.device = "/dev/ttyUSB0";
    args.file = "capture.out";
    args.speed = B9600;
    args.handshake = false;
    args.format = TF_RAW;
    args.leadin_strip = -1;

    if (0 > argp_parse (&argp, argc, argv, 0, 0, &args))
        return -1;

    fd = open(args.device, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0) {
        fprintf(stderr, "Error opening device %s: %s\n", args.device, strerror(errno));
        return -1;
    }

    if (set_interface_attribs(fd, args.speed, args.parity, args.bits, args.stop_bits, args.handshake) < 0) {
        return -1;
    }

    if ((fCapture = fopen(args.file, "w")) == NULL) {
        fprintf(stderr, "Could not write to file \"%s\": %s\n", args.file, strerror(errno));
        return -1;
    }

    /* Simple non canonical input */
    do {
        unsigned char buf[80];
        int rdlen;

        rdlen = read(fd, buf, sizeof(buf) - 1);
        if (rdlen > 0) {
            unsigned char *p;

            for (p = buf; rdlen-- > 0; p++) {
                if (args.format == TF_BIN) capture_bin(fCapture, &state, *p);
                if (args.format == TF_RIM) capture_rim(fCapture, &state, *p);
                if (args.format == TF_RAW) capture_raw(fCapture, &state, *p, args.leadin_strip);
            }
            time_out = false;
        } else if (rdlen == 0) {
            time_out = true;
        } else {
            fprintf(stderr, "Error from read: %d: %s\n", rdlen, strerror(errno));
            time_out = true;
        }
    } while (state == CS_START || (state != CS_DONE && !time_out));

    close(fd);
    fclose(fCapture);
}
