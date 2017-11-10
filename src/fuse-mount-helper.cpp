
#include <fuse.h>
#include <stdlib.h>
#include <utils.h>
#include "context.h"
#include "fuse-mount-helper.h"

int fuse_custom_mounter(const efsng::settings& user_opts, const struct fuse_operations* ops) {
    
	struct fuse *fuse;
	char *mountpoint;
	int multithreaded;
	int res;

    /* create a context object to maintain the internal state of the filesystem 
     * so that we can later pass it around to the filesystem operations */
    auto efsng_ctx = std::make_unique<efsng::context>(user_opts);

	fuse = fuse_setup(user_opts.m_fuse_argc, 
	                  const_cast<char**>(user_opts.m_fuse_argv), 
	                  ops, 
	                  sizeof(*ops), 
	                  &mountpoint,
				      &multithreaded, 
				      (void*) efsng_ctx.get());

	if(fuse == NULL) {
		return 1;
    }

	if(multithreaded) {
		res = fuse_loop_mt(fuse);
    }
	else {
		res = fuse_loop(fuse);
    }

	fuse_teardown(fuse, mountpoint);

	if (res == -1) {
		return 1;
	}

	return 0;
}

//} //namespace efsng
