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

#define FUSE_USE_VERSION 26
#include <fuse.h>
#include <memory>
#include <cstring>
#include <iostream>

#include "command-line.h"

// main entry point
int main (int argc, char *argv[]){

    /* 1. parse command-line arguments */
    std::shared_ptr<Arguments> user_args(new Arguments);

    for(int i=0; i<MAX_FUSE_ARGS; ++i){
        /* libfuse expects NULL args */
        user_args->fuse_argv[i] = NULL;
    }

    if(argc == 1 || !process_args(argc, argv, user_args)){
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    for(int i=0; i<user_args->fuse_argc; ++i){
        std::cerr << user_args->fuse_argv[i] << "\n";
    }

    fuse_operations efsng_ops;
    memset(&efsng_ops, 0, sizeof(fuse_operations));
    
    /* start the filesystem */
    int res = fuse_main(user_args->fuse_argc, 
                        const_cast<char **>(user_args->fuse_argv), 
                        &efsng_ops, 
                        (void*) NULL);

    std::cout << "(" << res << ") Bye!\n";

    return 0;
}
