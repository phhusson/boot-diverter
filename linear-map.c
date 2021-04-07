#define _LARGEFILE64_SOURCE 1
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <strings.h>
#include <linux/fs.h>
#include <linux/fiemap.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

int main(int argc, char **argv) {
    if(argc != 2) exit(__LINE__);
    int fd = open(argv[1], O_RDWR);
    perror("open");
    struct fiemap f;
    bzero(&f, sizeof(f));
    f.fm_flags = FIEMAP_FLAG_SYNC;
    f.fm_length = -1;
    if(ioctl(fd, FS_IOC_FIEMAP, &f) != 0) {
        fprintf(stderr, "Fail at %d\n", __LINE__);
        perror("fiemap");
        exit(__LINE__);
    }
    struct fiemap *map = calloc(1, sizeof(struct fiemap) + f.fm_mapped_extents * sizeof(struct fiemap_extent));

    map->fm_extent_count = f.fm_mapped_extents;
    map->fm_flags = FIEMAP_FLAG_SYNC;
    map->fm_length = -1;
    if(ioctl(fd, FS_IOC_FIEMAP, map) != 0) {
        fprintf(stderr, "Fail at %d\n", __LINE__);
        perror("fiemap");
        exit(__LINE__);
    }
    fprintf(stderr, "Getting %d extents\n", f.fm_mapped_extents);

    __u64 last_logical = 0;
    int gotZero = 0;
    char zero[512];
    bzero(zero, sizeof(zero));
    for(int i=0; i<map->fm_mapped_extents; i++) {
        if(last_logical != map->fm_extents[i].fe_logical) {
            long long zeroSize = (map->fm_extents[i].fe_logical - last_logical)/512;
            printf("zero %lld %lld\n", last_logical/512, zeroSize);
            lseek(fd, last_logical, SEEK_SET);
            for(int j=0; j<zeroSize; j++) {
                write(fd, zero, 512);
            }
            gotZero = 1;
            //fprintf(stderr, "zero from %08llx to %08llx\n", last_logical, map->fm_extents[i].fe_logical);
        }
        #if 0
        fprintf(stderr, "file from %08llx to %08llx\n", map->fm_extents[i].fe_logical, (map->fm_extents[i].fe_logical + map->fm_extents[i].fe_length));
        fprintf(stderr, "\t@%08llx\n", map->fm_extents[i].fe_physical);
        #endif
        printf("linear %lld %lld /dev/mmcblk0p1 %lld\n", last_logical/512, map->fm_extents[i].fe_length/512, map->fm_extents[i].fe_physical/512);
        last_logical = map->fm_extents[i].fe_logical + map->fm_extents[i].fe_length;
    }
    if(gotZero) return 1;
    return 0;
}
