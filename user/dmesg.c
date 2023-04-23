#include "kernel/types.h"
#include "user/user.h"
#include "kernel/stat.h"
#include "kernel/lock_consts.h"
#include "kernel/msg_buff_size.h"
// #include "kernel/.h"

int main(int argc, char** argv) {
    char buff[MSG_BUFF_SIZE];
    dmesg(buff, MSG_BUFF_SIZE - 1);
    printf(buff);

    return 0;
}