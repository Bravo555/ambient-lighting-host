#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <stdint.h>
#include <signal.h>
#include <stdlib.h>

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <chrono>

int fd;

void int_handler(int sig) {
    write(fd, "\x04\x00", 2);
    close(fd);

    exit(0);
}

int main(int argc, char** argv) {
    // serial port
    fd = open("/dev/ttyUSB0", O_RDWR | O_SYNC | O_NOCTTY);
    if(fd < 0) {
        fprintf(stderr, "fuck\n");
        return 1;
    }

    signal(SIGINT, int_handler);

    struct termios tty;
    struct termios tty_old;
    memset(&tty, 0, sizeof tty);
    tty_old = tty;

    cfsetispeed(&tty, (speed_t)B115200);
    cfsetospeed(&tty, (speed_t)B115200);

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


    Display* display = XOpenDisplay((char *) NULL);

    if(display == NULL) {
        fprintf(stderr, "an error occured");
        return 1;
    }
    Window rootWindow = XRootWindow(display, XDefaultScreen(display));
    unsigned long counter = 0;

    write(fd, "\x04\xff", 2);

    const std::chrono::milliseconds INTERVAL(32);
    auto lastTime = std::chrono::system_clock::now();

    while(1) {
        auto currentTime = std::chrono::system_clock::now();

        if(currentTime - lastTime > INTERVAL) {
            XImage* image = XGetImage(display, rootWindow, 0, 0, 1920, 1080, AllPlanes,ZPixmap);
            int status = XInitImage(image);

            unsigned int a = image->f.get_pixel(image, 960, 540);

            uint8_t red = a >> 16;
            uint8_t green = a >> 8;
            uint8_t blue = a;

            uint8_t payload[] = {0x03, red, green, blue};
            write(fd, payload, 4);

            XDestroyImage(image);

            auto end = std::chrono::system_clock::now();

            printf("took %ld ms\n", std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastTime).count());
            lastTime = std::chrono::system_clock::now();

            counter++;
        }
    }

    close(fd);
}
