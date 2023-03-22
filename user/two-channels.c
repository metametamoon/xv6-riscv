#include "kernel/types.h"
#include "user/user.h"
#include "kernel/stat.h"
#include "kernel/lock_consts.h"


int main(int argc, char** argv) {
    if (argc != 2) {
        printf("Two arguments expected, got %d", argc);
    }
    int parent_to_child[2];
    int child_to_parent[2];
    int pipesucc = pipe(parent_to_child);
    
    int printlock = lock(LK_OPEN, 0);
    if (printlock < 0) {
        printf("Error creating a lock.");
        exit(-1);
    }
    if (pipesucc == -1) {
        printf("Error creating a pipe.");
        exit(-1);
    }
    pipesucc = pipe(child_to_parent);
    if (pipesucc == -1) {
        printf("Error creating a pipe.");
        exit(-1);
    }
    int pid = fork();
    if (pid < 0) {
        printf("Error executing fork().");
        exit(-1);
    } else if (pid > 0) {
        close(parent_to_child[0]);
        close(child_to_parent[1]);
        char* str = argv[1];
        for (int i = 0; str[i] != 0; ++i) {
            write(parent_to_child[1], str + i, 1);
        }
        close(parent_to_child[1]);

        char buf[1];;
        while(read(child_to_parent[0], buf, 1) == 1) {
            lock(LK_ACQ, printlock);
            fprintf(1, "pid <%d>: received <%c>\n", getpid(), *buf);
            lock(LK_REL, printlock);
        }
        lock(LK_ACQ, printlock);
        fprintf(1, "pid <%d>: ended\n", getpid());
        lock(LK_REL, printlock);
        close(child_to_parent[0]);
        wait(0);
        lock(LK_CLOSE, printlock);
        exit(0);

    } else {
        close(parent_to_child[1]);
        close(child_to_parent[0]);
        char* buf = malloc(1);
        while(read(parent_to_child[0], buf, 1) == 1) {
            lock(LK_ACQ, printlock);
            
            fprintf(1, "pid <%d>: received <%c>\n", getpid(), *buf);
            write(child_to_parent[1], buf, 1);
            fprintf(1, "pid <%d>: sent <%c>\n", getpid(), *buf);
            lock(LK_REL, printlock);
        }
        lock(LK_ACQ, printlock);
        fprintf(1, "pid <%d>: ended\n", getpid());
        lock(LK_REL, printlock);
        close(child_to_parent[1]);
        close(parent_to_child[0]);
        exit(0);
    }

    return 0;
}