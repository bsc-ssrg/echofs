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
 * The Echo Filesystem NG is distributed in the hope that it will        *
 * be useful, but WITHOUT ANY WARRANTY; without even the implied         *
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR               *
 * PURPOSE.  See the GNU Lesser General Public License for more          *
 * details.                                                              *
 *                                                                       *
 * You should have received a copy of the GNU Lesser General Public      *
 * License along with Echo Filesystem NG; if not, write to the Free      *
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.    *
 *                                                                       *
 *************************************************************************/


/* C includes */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#include <fuse.h>

/* C++ includes */
#include <sstream>
#include <iostream>

/* project includes */
#include <command-line.h>

namespace cmdline {

void usage(const std::string& name, bool is_error){

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

void fuse_usage(const std::string& name){

    std::cout << "Usage: " << name << " [options] root_dir mount_point [-- [FUSE Mount Options]]\n"
              << "Valid FUSE Mount Options:\n\n";

    int argc = 2;
    const char* argv[] = {"...", "-h"};
    fuse_main(argc, const_cast<char**>(argv), (fuse_operations*) NULL, NULL);
}

} // namespace efsng
