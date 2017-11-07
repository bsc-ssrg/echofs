
#include <fuse.h>
#include <stdlib.h>
#include <utils.h>
#include <fuse-mount-helper.h>


#if 0

//extern "C" int fuse_chan_fd(struct fuse_chan *ch);

//namespace efsng {

int xfuse_set_signal_handlers(struct fuse_session *se);

struct fuse *fuse_setup_custom(int argc, char *argv[],
			       const struct fuse_operations *op,
			       size_t op_size,
			       char **mountpoint,
			       int *multithreaded,
			       int *fd,
			       void *user_data) {
	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
	struct fuse_chan *ch;
	struct fuse *fuse;
	int foreground;
	int res;

	res = fuse_parse_cmdline(&args, mountpoint, multithreaded, &foreground);

	if(res == -1) {
		return NULL; 
    }

	ch = fuse_mount(*mountpoint, &args);

	if(!ch) {
		fuse_opt_free_args(&args);
		goto err_free;
	}

	fuse = fuse_new(ch, &args, op, op_size, user_data);
	fuse_opt_free_args(&args);

	if(fuse == NULL) {
		goto err_unmount;
    }

	res = fuse_daemonize(foreground);

	if(res == -1) {
		goto err_unmount;
    }

	//res = fuse_set_signal_handlers(fuse_get_session(fuse));

	res = xfuse_set_signal_handlers(fuse_get_session(fuse));

	if(res == -1) {
		goto err_unmount;
    }


	if(fd) {
		*fd = fuse_chan_fd(ch);
    }

	return fuse;

err_unmount:
	fuse_unmount(*mountpoint, ch);

	if(fuse) {
		fuse_destroy(fuse);
    }

err_free:
	free(*mountpoint);
	return NULL;
}
#endif

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
