/*************************************************************************
 * (C) Copyright 2016 Barcelona Supercomputing Center                    * 
 *                    Centro Nacional de Supercomputacion                *
 *                                                                       *
 * This file is part of the Echo Filesystem NG.                          *
 *                                                                       *
 * See AUTHORS file in the top level directory for information           *
 * regarding developers and contributors.                                *
 *                                                                       *
 * This library is free software; you can redistribute it and/or         *
 * modify it under the terms of the GNU Lesser General Public            *
 * License as published by the Free Software Foundation; either          *
 * version 3 of the License, or (at your option) any later version.      *
 *                                                                       *
 * Mercurium C/C++ source-to-source compiler is distributed in the hope  *
 * that it will be useful, but WITHOUT ANY WARRANTY; without even the    *
 * implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR       *
 * PURPOSE.  See the GNU Lesser General Public License for more          *
 * details.                                                              *
 *                                                                       *
 * You should have received a copy of the GNU Lesser General Public      *
 * License along with Mercurium C/C++ source-to-source compiler; if      *
 * not, write to the Free Software Foundation, Inc., 675 Mass Ave,       *
 * Cambridge, MA 02139, USA.                                             *
 *************************************************************************/


/* C includes */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#include <fuse.h>
#include <getopt.h>

/* C++ includes */
#include <sstream>
#include <boost/filesystem.hpp>
#include <boost/log/trivial.hpp>

/* project includes */
#include "command-line.h"
#include "configuration.h"

namespace bfs = boost::filesystem;

namespace efsng {

void usage(const char* name, bool is_error){

    std::stringstream ss;  

    if(!is_error){
        ss << PACKAGE_NAME << " v" << VERSION << "\n\n";
    }

    ss << "Usage: " << name << " [options] root_dir mount_point [-- [FUSE Mount Options]]\n";

    if(is_error){
        std::cerr << ss.str();
        return;
    }

    std::cout << ss.str();
}

static void fuse_usage(const char* name){

    std::cout << "Usage: " << name << " [options] root_dir mount_point [-- [FUSE Mount Options]]\n"
              << "Valid FUSE Mount Options:\n\n";

    int argc = 2;
    const char* argv[] = {"...", "-h"};
    fuse_main(argc, const_cast<char**>(argv), (fuse_operations*) NULL, NULL);
}

bool process_args(int argc, char* argv[], Arguments* out){

    /* pass through (and remember) executable name */
    std::string exec_name = bfs::basename(argv[0]);
    out->exec_name = exec_name;
    out->fuse_argv[0] = exec_name.c_str();
    ++out->fuse_argc;

    /* leave a space for the mount point given that FUSE expects the mount point before any flags */
    ++out->fuse_argv[1] = NULL;
    ++out->fuse_argc;

    /* command line options */
    struct option long_options[] = {
        {"root-dir",        1, 0, 'r'}, /* directory to mirror */
        {"mount-point",     1, 0, 'm'}, /* mount point */
        {"config-file",     1, 0, 'c'}, /* configuration file */
        {"help",            0, 0, 'h'}, /* display usage */
        {"foreground",      0, 0, 'f'}, /* foreground operation */
        {"fuse-debug",      0, 0, 'd'}, /* FUSE debug mode */
        {"fuse-help",       0, 0, 'H'}, /* fuse_mount usage */
        {"version",         0, 0, 'V'}, /* version information */
        {0, 0, 0, 0}
    };

    /* build opt_string automatically to pass to getopt_long */ 
    std::string opt_string;
    int num_options = sizeof(long_options)/sizeof(long_options[0]) - 1;

    for(int i=0; i<num_options; i++){
        opt_string += static_cast<char>(long_options[i].val);

        if(long_options[i].has_arg){
            opt_string += ":";
        }
    }

    /* o: arguments directly passed to FUSE */
    opt_string += ("o:");

    /* helper lambda function to safely add an argument to fuse_argv */
    auto push_arg = [&out](const char* arg){
        assert(out->fuse_argc < MAX_FUSE_ARGS);
        size_t n = strlen(arg);
        char* arg_copy = strndup(arg, n+1);
        arg_copy[n] = '\0';
        out->fuse_argv[out->fuse_argc++] = arg_copy;
    };

    /* process the options */
    while(true){

        int opt_idx = 0;

        int rv = getopt_long(argc, argv, opt_string.c_str(), long_options, &opt_idx);

        if(rv == -1){
            break;
        }

        switch(rv){
            /* efs-ng options */
            case 'r':
                /* configure FUSE to prepend the root-dir to all paths */
                out->root_dir = std::string(optarg);
                break;
            case 'm':
                out->mount_point = std::string(optarg);
                break;
            case 'c':
                out->config_file = std::string(optarg);
                break;
            case 'h':
                usage(exec_name.c_str());
                exit(EXIT_SUCCESS);
                break;
            case 'f':
                /* prevent that FUSE starts as a daemon */
                push_arg("-f");
                break;
            case 'd':
                push_arg("-d");
                break;
            case 'H':
                fuse_usage(exec_name.c_str());
                exit(EXIT_SUCCESS);
                break;
            case 'V':
                std::cout << exec_name.c_str() << " version " << VERSION << "\n"
                          << "Copyright (C) 2016 Barcelona Supercomputing Center (BSC-CNS)\n" 
                          << "This is free software; see the source for copying conditions. There is NO\n"
                          << "warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n";
                exit(EXIT_SUCCESS);
                break;
            case 'o':
                push_arg("-o");
                push_arg(optarg);
                break;
            case '?':
                /* invalid option */
                return false;
            case ':':
                /* missing parameter for option */
                return false;
            default:
                break;
        }
    }

    if(out->config_file != ""){
        BOOST_LOG_TRIVIAL(info) << "Reading configuration file " << out->config_file;

        if(!efsng::Configuration::load(out->config_file, out)){
            BOOST_LOG_TRIVIAL(warning) << "  Errors occurred when reading the configuration file. "
                                       << "Any of its contents will be ignored.";
        }
    }

    assert(out->root_dir != out->mount_point);

    if(out->root_dir != ""){
        std::string option = "modules=subdir,subdir=";
        option += out->root_dir.c_str();
        push_arg("-o");
        push_arg(option.c_str());
    }


    /** 
     * other default options for FUSE:
     * - nonempty: needed to allow the filesystem to be mounted on top of non empty directories.
     * - use_ino: needed to allow files to retain their original inode numbers.
     * - attr_timeout=0: set cache timeout for names to 0s
     */
    push_arg("-o");
    push_arg("nonempty,use_ino,attr_timeout=0");

    /* if there are still extra unparsed arguments, pass them onto FUSE */
    if(optind < argc){
        push_arg(argv[optind]);
        ++optind;
    }

    /* fill in the mount point for FUSE */
    out->fuse_argv[1] = out->mount_point.c_str();

    return true;
}

} // namespace efsng
