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

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <chrono>
#include <thread>

int fd;

void int_handler(int sig) {
    write(fd, "\x04\x00", 2);
    close(fd);

    exit(0);
}

void init_serial(int fd) {
    struct termios tty;
    struct termios tty_old;
    memset(&tty, 0, sizeof tty);
    tty_old = tty;

    cfsetispeed(&tty, (speed_t)B115200);
    cfsetospeed(&tty, (speed_t)B115200);

    tty.c_cflag &= ~PARENB; // Make 8n1
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;

    tty.c_cc[VMIN] = 1;            // read doesn't block
    tty.c_cc[VTIME] = 5;           // 0.5 seconds read timeout
    tty.c_cflag |= CREAD | CLOCAL; // turn on READ & ignore ctrl lines

    cfmakeraw(&tty);
    tcflush(fd, TCIFLUSH);
    tcsetattr(fd, TCSANOW, &tty);
}

uint32_t average_colour(uint32_t* data, uint width, uint height) {
    unsigned long redSum = 0, greenSum = 0, blueSum = 0;
    for(uint y = 0; y < height; y++) {
        for(uint x = 0; x < width; x++) {
            uint32_t pixel = data[1920 * y + x];

            uint red = (pixel >> 16) & 0xff;
            uint green = (pixel >> 8) & 0xff;
            uint blue = pixel & 0xff;

            redSum += red;
            greenSum += green;
            blueSum += blue;
        }
    }
    uint redavg = redSum / (width * height);
    uint greenavg = greenSum / (width * height);
    uint blueavg = blueSum / (width * height);

    uint8_t colours[] = {redavg, greenavg, blueavg};
    return *(uint32_t*)colours;
}

void send_colour_data(int fd, uint32_t colour) {
    uint8_t red = colour >> 16;
    uint8_t green = colour >> 8;
    uint8_t blue = colour;

    uint8_t payload[] = {0x03, red, green, blue};
    write(fd, payload, 4);
}

void enable_capture_mode(int fd) {
    write(fd, "\x04\xff", 2);
}

int main(int argc, char** argv) {
    // serial port
    // fd = open("/dev/ttyUSB0", O_RDWR | O_SYNC | O_NOCTTY);
    // if(fd < 0) {
    //     fprintf(stderr, "fuck\n");
    //     return 1;
    // }

    // init_serial(fd);
    // enable_capture_mode(fd);
    // signal(SIGINT, int_handler);

    if(!glfwInit()) {
        fprintf(stderr, "error init glfw");
        return 3;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow *window = glfwCreateWindow(800, 600, "LearnOpenGL", NULL, NULL);
    if (window == NULL) {
        fprintf(stderr, "Failed to create GLFW window\n");
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        fprintf(stderr, "Failed to initialize GLAD\n");
        return -1;
    }

    glViewport(0, 0, 800, 600);

    Display* display = XOpenDisplay((char *) NULL);
    if(display == NULL) {
        fprintf(stderr, "an error occured");
        return 1;
    }

    int width = XDisplayWidth(display, 0);
    int height = XDisplayHeight(display, 0);

    Window rootWindow = XRootWindow(display, XDefaultScreen(display));
    XImage* image = XGetImage(display, rootWindow, 0, 0, width, height, AllPlanes, ZPixmap);

    GLuint vbo = 0;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, width * height * 4, image->data, GL_STATIC_DRAW);


    unsigned long counter = 0;
    while(!glfwWindowShouldClose(window)) {
        auto start = std::chrono::system_clock::now();

        XImage* image = XGetImage(display, rootWindow, 0, 0, width, height, AllPlanes, ZPixmap);
        int status = XInitImage(image);

        // uint32_t* data = (uint32_t*)image->data;
        // uint32_t avgColour = average_colour(data, width, height);

        // send_colour_data(fd, avgColour);

        XDestroyImage(image);

        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glfwSwapBuffers(window);
        glfwPollEvents();

        auto end = std::chrono::system_clock::now();
        auto duration = end - start;
        printf("took %ld ms\n", std::chrono::duration_cast<std::chrono::milliseconds>(duration).count());
        counter++;
    }

    close(fd);
    glfwTerminate();

    printf("shit worked\n");
}
