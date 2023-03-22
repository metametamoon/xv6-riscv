#include "kernel/types.h"
#include "user/user.h"
#include "kernel/stat.h"


int const get_available_type = 0, 
    release_ownership_type = 1, 
    acquire_type = 2, 
    release_type = 3;


int main(int argc, char** argv) {
    if (argc != 2) {
        printf("Two arguments expected, got %d", argc);
    }
    int parent_to_child[2];
    int child_to_parent[2];
    int pipesucc = pipe(parent_to_child);
    
    int printlock = lock(get_available_type, 0);
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
            lock(acquire_type, printlock);
            fprintf(1, "pid <%d>: received <%c>\n", getpid(), *buf);
            lock(release_type, printlock);
        }
        lock(acquire_type, printlock);
        fprintf(1, "pid <%d>: ended\n", getpid());
        lock(release_type, printlock);
        close(child_to_parent[0]);
        
        wait(0);
        exit(0);

    } else {
        close(parent_to_child[1]);
        close(child_to_parent[0]);
        char* buf = malloc(1);
        while(read(parent_to_child[0], buf, 1) == 1) {
            lock(acquire_type, printlock);
            
            fprintf(1, "pid <%d>: received <%c>\n", getpid(), *buf);
            write(child_to_parent[1], buf, 1);
            fprintf(1, "pid <%d>: sent <%c>\n", getpid(), *buf);
            lock(release_type, printlock);
        }
        lock(acquire_type, printlock);
        fprintf(1, "pid <%d>: ended\n", getpid());
        lock(release_type, printlock);
        close(child_to_parent[1]);
        close(parent_to_child[0]);
        exit(0);
    }

    return 0;
}