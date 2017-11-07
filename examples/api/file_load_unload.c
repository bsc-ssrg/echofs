#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <efs-api.h>

int main(int argc, char* argv[]) {

    (void) argc;
    (void) argv;

    int rv;
    struct efs_iocb iocb = EFS_IOCB_INIT("NVRAM-NVML", "/home/amiranda/var/projects/efs-ng/build/root/file.tmp", 0, 0);

    if((rv = efs_load(&iocb)) != EFS_API_SUCCESS) {
        fprintf(stderr, "efs_load error: %s\n", efs_strerror(rv) );
        exit(EXIT_FAILURE);
    }

retry:
    if((rv = efs_status(&iocb)) != EFS_API_SUCCESS) {
        fprintf(stderr, "efs_status error: %s\n", efs_strerror(rv));

        if(rv != EFS_API_ETASKPENDING && rv != EFS_API_ETASKINPROGRESS) {
            exit(EXIT_FAILURE);
        }

        // wait 1s to avoid flooding the filesystem
        usleep(1000e3);
        goto retry;
    }

    if((rv = efs_unload(&iocb)) != EFS_API_SUCCESS) {
        fprintf(stderr, "efs_unload error: %s\n", efs_strerror(rv));
        exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
}
