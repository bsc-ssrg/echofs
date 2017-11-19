
#ifndef __FUSE_MOUNT_HELPER__
#define __FUSE_MOUNT_HELPER__

#include <fuse.h>

#include "efs-ng.h"

namespace efsng {

int fuse_custom_mounter(const config::settings& user_opts, const struct fuse_operations* op);

} // namespace efsng

#endif /* __FUSE_MOUNT_HELPER__ */
