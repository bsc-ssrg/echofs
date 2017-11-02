
#ifndef __FUSE_MOUNT_HELPER__
#define __FUSE_MOUNT_HELPER__

#include <efs-ng.h>
#include <fuse.h>

//namespace efsng {

//extern "C" int fuse_custom_mounter(int argc, char *argv[], const struct fuse_operations *op, void *user_data);
int fuse_custom_mounter(const efsng::settings& user_opts, const struct fuse_operations* op);

//} // namespace efsng

#endif /* __FUSE_MOUNT_HELPER__ */
