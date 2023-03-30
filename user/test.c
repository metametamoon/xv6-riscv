#include "kernel/types.h"
#include "user/user.h"
#include "kernel/stat.h"



int main() {
    printf("1.\n");
    vmprint();
    printf("2.\n");
    pgaccess();
    free(malloc(1000));
    printf("3.\n");
    pgaccess();
    printf("3.\n");
    pgaccess();
    return 0;
}