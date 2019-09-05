#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <stdint.h>

#include <X11/X.h>
#include <X11/Xlib.h>

int main(int argc, char** argv) {
    // serial port
    int fd = open("/dev/ttyUSB0", O_RDWR | O_SYNC | O_NOCTTY);

    struct termios tty;
    struct termios tty_old;
    memset(&tty, 0, sizeof tty);
    tty_old = tty;

    cfsetispeed(&tty, (speed_t)B9600);
    cfsetospeed(&tty, (speed_t)B9600);

    tty.c_cflag     &=  ~PARENB;            // Make 8n1
    tty.c_cflag     &=  ~CSTOPB;
    tty.c_cflag     &=  ~CSIZE;
    tty.c_cflag     |=  CS8;

    tty.c_cc[VMIN]   =  1;                  // read doesn't block
    tty.c_cc[VTIME]  =  5;                  // 0.5 seconds read timeout
    tty.c_cflag     |=  CREAD | CLOCAL;     // turn on READ & ignore ctrl lines

    cfmakeraw(&tty);
    tcflush(fd, TCIFLUSH);
    tcsetattr(fd, TCSANOW, &tty);

    if(fd < 0) {
        fprintf(stderr, "fuck");
        return 1;
    }



    Display* display = XOpenDisplay((char *) NULL);

    if(display == NULL) {
        fprintf(stderr, "an error occured");
        return 1;
    }

    Window rootWindow = XRootWindow(display, XDefaultScreen(display));
    unsigned long counter = 0;
    while(1) {
        XImage* image = XGetImage(display, rootWindow, 0, 0, 1920, 1080, AllPlanes,ZPixmap);
        int status = XInitImage(image);

        unsigned int a = image->f.get_pixel(image, 960, 540);

        uint8_t red = a >> 16;
        uint8_t green = a >> 8;
        uint8_t blue = a;

        printf("[%lu] %x\n", counter, a);
        printf("[%lu] %x %x %x\n", counter, red, green, blue);

        uint8_t payload[] = {0x01, red, green, blue};
        write(fd, payload, 4);

        sleep(1);

        uint8_t colours[3];
        read(fd, &colours, 3);
        printf("[%lu] READ: %x %x %x\n", counter, colours[0], colours[1], colours[2]);

        counter++;
    }

    close(fd);
}
