#include "kernel/types.h"
#include "user/user.h"
#include "kernel/stat.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"

int main(int argc, char** argv) {
    if (argc != 3) {
        printf("Expected two args\n");
        return -1;
    }
    int block_num = atoi(argv[2]);
    int fd = open(argv[1], O_CREATE | O_WRONLY);
    if (fd <= 0) {
        printf("Could not open file\n");
        return -1;
    }
    for (int i = 0; i < block_num; ++i) {
        char empty_block[BSIZE];
        if (write(fd, empty_block, BSIZE) < BSIZE) {
            printf("Error on write of the %dth block.\n", i);
            return -1;
        }
    }
    printf("Wrote %d blocks to file \"%s\"\n", block_num, argv[1]);

    return 0;
}