#include "kernel/types.h"
#include "user/user.h"
#include "kernel/stat.h"
#include "kernel/lock_consts.h"


int main(int argc, char** argv) {
    dmesg();
    return 0;
}