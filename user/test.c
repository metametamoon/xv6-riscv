#include "kernel/types.h"
#include "user/user.h"
#include "kernel/stat.h"

int main() {
    sleep(1);
    exit(0);
    int p[2];
    char *argv[2];
    argv[0] = "wc";
    argv[1] = 0;
    int pipsucc = pipe(p); 
    if (pipsucc < 0) {
        printf("Error on piping");
        exit(-1);
    }
    int forksucc = fork();
    if (forksucc == 0) {
        close(0);
        dup(p[0]);
        close(p[0]);
        close(p[1]);
        int execsucc = exec("wc", argv);
        if (execsucc < 0) {
            printf("Error on executing exec");
            exit(-1);
        }
    } else if (forksucc > 0) {
        close(p[0]);
        char buf[512];
        int n = read(0, buf, sizeof buf);
        int wrote = write(p[1], buf, n);
        if (wrote < n) {
            close(p[1]);
            printf("Wrote less than expected.");
            exit(-1);
        }
        close(p[1]);
        wait(0);
    } else {
        printf("Error on forking");
        exit(-1);
    }
    exit(0);
    return 0;
}