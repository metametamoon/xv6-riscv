#include "kernel/types.h"
#include "user/user.h"
#include "kernel/stat.h"

int main() {
    symlink("new", "cat");
    return 0;
}