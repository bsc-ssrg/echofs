#include <stdio.h>
#include <efs-api.h>

int main(int argc, char* argv[]) {

    EFS_FILE fdesc = EFS_FILE_INIT("NVRAM-NVML", "foobar", 0, 0);

    if(efs_load(&fdesc) != EFS_API_SUCCESS) {
        fprintf(stderr, "error\n");
    }

    return 0;
}
