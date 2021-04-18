#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>
#include <linux/input.h>


int HasKeyEvents(int device_fd) {
  unsigned long evbit = 0;
  ioctl(device_fd, EVIOCGBIT(0, sizeof(evbit)), &evbit);
  return evbit & (1 << EV_KEY);
}

int HasSpecificKey(int device_fd, unsigned int key) {
  size_t nchar = KEY_MAX/8 + 1;
  unsigned char bits[nchar];
  ioctl(device_fd, EVIOCGBIT(EV_KEY, sizeof(bits)), &bits);
  return bits[key/8] & (1 << (key % 8));
}

int IsKeyPressed(int device_fd, unsigned int key) {
    size_t nchar = KEY_MAX/8 + 1;
    unsigned char bits[nchar];
    ioctl(device_fd, EVIOCGKEY(sizeof(bits)), &bits);
    return bits[key/8] & (1 << (key % 8));
}

int main() {
    //We won't have more than 32 FDs with volume buttons...
    int *fds = malloc(sizeof(int) * 32);
    int nFds = 0;

    for(int i=0; i<128;i++) {
        char *path = NULL;
        asprintf(&path, "/dev/input/event%d", i);
        int fd = open(path, O_RDONLY);
        if(!HasKeyEvents(fd)) {
            fprintf(stderr, "event%d doesnt have keys\n", i);
            goto next;
        }

        if(HasSpecificKey(fd, KEY_VOLUMEDOWN) || HasSpecificKey(fd, KEY_VOLUMEUP)) {
            fprintf(stderr, "We have a winner with event%d!\n", i);
            fds[nFds++] = fd;
            free(path);
            continue;
        }

next:
        close(fd);
        fd = -1;
        free(path);
    }
    if(nFds==0) return -1;

    for(int i=0; i<nFds;i++) {
        if(IsKeyPressed(fds[i], KEY_VOLUMEUP)) {
            printf("UP\n");
            exit(0);
        }
        if(IsKeyPressed(fds[i], KEY_VOLUMEDOWN)) {
            printf("DOWN\n");
            exit(0);
        }
    }

    while(1) {
        struct input_event ev;
        fd_set fdset;
        FD_ZERO(&fdset);
        for(int i=0; i<nFds;i++) {
            FD_SET(fds[i], &fdset);
        }
        int ret = select(fds[nFds-1]+1, &fdset, NULL, NULL, NULL);
        if(ret <= 0) exit(-1);

        int fd = -1;
        for(int i=0; i<nFds;i++) {
            if(FD_ISSET(fds[i], &fdset)) {
                fd = fds[i];
                break;
            }
        }
        read(fd, &ev, sizeof(ev));
        fprintf(stderr, "Hello got new event %d %d %d!\n", ev.type, ev.code, ev.value);
        if(ev.type != EV_KEY) {
            fprintf(stderr, "Got non-key event\n");
            continue;
        }
        if(ev.value != 1) {
            fprintf(stderr, "Got non-pressed event\n");
            continue;
        }
        if(ev.code == KEY_VOLUMEUP) {
            printf("UP\n");
            exit(0);
        }
        if(ev.code == KEY_VOLUMEDOWN) {
            printf("DOWN\n");
            exit(0);
        }
    }
}
