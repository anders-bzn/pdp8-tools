/*
 * Program for sending papertapes
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
    "put-tape 0.99";

const char *argp_program_bug_address =
    "<anders@abc80.net>";


/* Program documentation. */
static char doc[] =
    "Program for sending papertapes, takes input from stdin or file and sends it on a serial port." \
    "Default is 9600 8N1 on device /dev/ttyUSB0.";


/* Options to be parsed. */
static struct argp_option options[] = {
    {"device",          'd', "DEV",         OPTION_ARG_OPTIONAL, "Serial device, /dev/ttyXXX"},
    {"bits",            'b', "5,6,7,8",     OPTION_ARG_OPTIONAL, "Number of data bits"},
    {"parity",          'p', "N,E,O,M",     OPTION_ARG_OPTIONAL, "Parity"},
    {"stop",            'S', "1,2",         OPTION_ARG_OPTIONAL, "Number of stop bits"},
    {"speed",           's', "BAUD",        OPTION_ARG_OPTIONAL, "Serial com speed"},
    {"handshake",       'h', 0,             OPTION_ARG_OPTIONAL, "Use RTS/CTS handshake"},
    {"filename",        'f', "FILE",        OPTION_ARG_OPTIONAL, "Input data file"},
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

/* Used by main to communicate with parse_opt. */
struct argp_arguments
{
    char *device;
    char *file;
    int bits;
    char parity;
    int stop_bits;
    bool handshake;
    speed_t speed;
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
        tty.c_cflag |= PARODD;      /* odd parity */
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

int main(int argc, char **argv)
{

    struct argp_arguments args;
    int buff[1];
    int ch;
    int fd;
    FILE *f;

    args.bits = 8;
    args.parity = 'N';
    args.stop_bits = 1;
    args.device = "/dev/ttyUSB0";
    args.file = NULL;
    args.speed = B9600;
    args.handshake = false;

    if (0 > argp_parse (&argp, argc, argv, 0, 0, &args))
        return -1;

    fd = open(args.device, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0) {
        fprintf(stderr, "Error opening device %s: %s\n", args.device, strerror(errno));
        return -1;
    }

    if (set_interface_attribs(fd, args.speed, args.parity, args.bits, args.stop_bits, args.handshake) < 0) {
        close(fd);
        return -1;
    }

    /*
     * Use stdin if no filename is given.
     */
    if (args.file != NULL) {
        if ((f = fopen(args.file, "r")) == NULL) {
            fprintf(stderr, "Could not open file \"%s\": %s\n", args.file, strerror(errno));
            close(fd);
            return -1;
        }
    } else {
        f = stdin;
    }

    /*
     * Get every char from f and put them on the serialport until EOF.
     */
    while (EOF != (ch = fgetc(f))) {
        buff[0] = ch;
        write(fd, buff, 1);
    }

    close(fd);
}
