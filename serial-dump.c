/* 
 * Simple serial-dump program. 
 *
 * Licence GPL 2.0 
 * 
 * By Anders Sandahl 2024
 *
 */

#include <argp.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <termios.h>
#include <unistd.h>


const char *argp_program_version =
    "serial-dump 0.099";

const char *argp_program_bug_address =
    "<anders@abc80.net>";

/* Program documentation. */
static char doc[] =
    "serial-dump program, takes input from serial port and prints in a hexdump style. Default is 9600 8N1 on device /dev/ttyUSB0.";

/* The options we understand. */
static struct argp_option options[] = {
    {"device", 'd', "DEV",     OPTION_ARG_OPTIONAL,  "Serial device"},
    {"log",    'l', "FILE",    OPTION_ARG_OPTIONAL,  "Dump recieved data to file"},
    {"bits",   'b', "5,6,7,8", OPTION_ARG_OPTIONAL,  "Number of data bits"},
    {"parity", 'p', "N,E,O,M", OPTION_ARG_OPTIONAL,  "Parity"},
    {"stop",   'S', "1,2",     OPTION_ARG_OPTIONAL,  "Number of stop bits"},
    {"speed",  's', "BAUD",    OPTION_ARG_OPTIONAL,  "Serial com speed"},
    {"quiet",  'q', 0,         OPTION_ARG_OPTIONAL,  "Don't print on stdout"},
    { 0 }
};

/* Used by main to communicate with parse_opt. */
struct argp_arguments
{
    char *device;
    char *log_file;
    int bits;
    char parity;
    int stop_bits;
    int speed;
    bool quiet;
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
    case 'l':
        arguments->log_file = arg;
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
                argp_usage (state);
                return ARGP_ERR_UNKNOWN;
                break;
            }
        }
        break;
    case 's':
        if (arg != NULL) {
            arguments->speed = atoi(arg);
        } else {
            argp_usage (state);
            return ARGP_ERR_UNKNOWN;
        }
        break;
        case 'q':
        arguments->quiet = true;
    break;

    case ARGP_KEY_ARG:
        if (state->arg_num != 0){
            argp_usage (state);
            return ARGP_ERR_UNKNOWN;
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

int set_interface_attribs(int fd, int baud, char parity, int bits, int stop_bits)
{
    struct termios tty;
    speed_t speed;

    if (tcgetattr(fd, &tty) < 0) {
        fprintf(stderr, "Error from tcgetattr: %s\n", strerror(errno));
        return -1;
    }

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
            fprintf(stderr, "Invalid baudrate: %d\n", baud);
            return -1;

    }

    cfsetspeed(&tty, (speed_t)speed);
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
        
    tty.c_cflag &= ~CRTSCTS;        /* no hardware flowcontrol */

    /* setup for non-canonical mode */
    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
    tty.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    tty.c_oflag &= ~OPOST;

    // Read with timeout, 0.1 s
    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 1;

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        fprintf(stderr, "Error from tcsetattr: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}


void printchar(char data, int num_recived)
{
    int i;
    static char buff[17];

/* 
Printed format:
00000000  71 71 71 71 71 71 71 71  71 71 71 71 71 71 71 71  |qqqqqqqqqqqqqqqq|
*/
    buff[16] = '\0';

    i = num_recived & 0xf;
    buff[i] = isprint(data) ? data : '.';

    if ((num_recived & 0xf) == 0) 
        printf("%8.8x  ", num_recived);

    printf("%2.2x ", data & 0xff);

    if (i == 15) {
        printf(" |%s|\n", buff);
    } 
    
    if (i == 7) 
        printf(" ");
}


void set_term_quiet_input()
{
    struct termios tc;
    tcgetattr(0, &tc);
    tc.c_lflag &= ~(ICANON | ECHO);
    tc.c_cc[VMIN] = 0;
    tc.c_cc[VTIME] = 0;
    tcsetattr(0, TCSANOW, &tc);
}


int main(int argc, char **argv)
{
    int fd, fd_log;
    struct argp_arguments args;
    int num_recived = 0;
    int num;
    int ret = 0;
    struct pollfd pfd = { .fd = 0, .events = POLLIN };
    struct termios tc;
    bool exit = false;

    args.bits = 8;
    args.parity = 'N';
    args.stop_bits = 1;
    args.device = "/dev/ttyUSB0";
    args.log_file = NULL;
    args.quiet = false;
    args.speed = 9600;

    argp_parse (&argp, argc, argv, 0, 0, &args);

    fd = open(args.device, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0) {
        fprintf(stderr, "Error opening device %s: %s\n", args.device, strerror(errno));
        return -1;
    }

    /* Set communication parameters */
    if (set_interface_attribs(fd, args.speed, args.parity, args.bits, args.stop_bits) < 0) {
        ret = -1;
        goto exit;
    }

    if (args.log_file) {
        fd_log = open(args.log_file, O_WRONLY | O_CREAT | O_SYNC, 0644);
        if (fd_log < 0) {
            fprintf(stderr, "Error opening log file %s: %s\n", args.log_file, strerror(errno));
            ret = -1;
            goto exit;
        }
    }

    tcgetattr(0, &tc);
    set_term_quiet_input();
    
    do {
        unsigned char ch;
        num = read(fd, &ch, 1);

        if (poll(&pfd, 1, 0)>0) {
            int c = getchar();
            if (c == 'q') 
                exit = true;
        }

        if (num < 1)
            continue;

        if (args.log_file)
            write(fd_log, &ch, 1);

        if (args.quiet == false)
            printchar(ch, num_recived);

        num_recived++;
    } while (num != -1 && !exit);

    tcsetattr(0, TCSANOW, &tc);
    printf("\n");

exit:
    if (fd_log > 0)
        close(fd_log);
    close(fd);
    return ret;
}