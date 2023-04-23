#include "kernel/types.h"
#include "user/user.h"
#include "kernel/stat.h"
#include "kernel/lock_consts.h"


int main(int argc, char** argv) {
    char buff[128];
    dmesg(buff, 128);
    return 0;
}