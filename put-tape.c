/*
 * Program for sending papertapes
 *
 * Licence GPL 2.0
 *
 * By Anders Sandahl 2024
 *
 */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <stdbool.h>

int set_interface_attribs(int fd, int speed)
{
    struct termios tty;

    if (tcgetattr(fd, &tty) < 0) {
        fprintf(stderr, "Error from tcgetattr: %s\n", strerror(errno));
        return -1;
    }

    tty.c_cflag |= (CLOCAL | CREAD);    /* ignore modem controls */
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;         /* 8-bit characters */
    tty.c_cflag &= ~PARENB;     /* no parity bit */
    tty.c_cflag &= ~CSTOPB;     /* only need 1 stop bit */
    tty.c_cflag |= CRTSCTS;     /* hardware flowcontrol */

    /* setup for non-canonical mode */
    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
    tty.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    tty.c_oflag &= ~OPOST;

    // Read with timeout, 1 s
    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 10;

    //Set speed
    if ( 0 > cfsetspeed(&tty, speed)){
        printf("Can't set baud rate: %s\n", strerror(errno));
        return -1;
    }

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        fprintf(stderr, "Error from tcsetattr: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}


int main(int argc, char **argv)
{
    char *portname = "/dev/ttyUSB0";
    int fd;
    int buff[1];

    fd = open(portname, O_RDWR | O_NOCTTY | O_SYNC);

    if (fd < 0) {
        fprintf(stderr, "Error opening device %s: %s\n", portname, strerror(errno));
        return -1;
    }

    /* Baudrate 1200, 8 bits, no parity, 1 stop bit */
    if (set_interface_attribs(fd, B1200) < 0) {
        return -1;
    }

    int ch;
    while (EOF != (ch = fgetc(stdin))) {
        buff[0] = ch;
        write(fd, buff, 1);
    }

    close(fd);
}
