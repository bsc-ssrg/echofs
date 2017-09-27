/*************************************************************************
 * (C) Copyright 2016-2017 Barcelona Supercomputing Center               *
 *                         Centro Nacional de Supercomputacion           *
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

#include <iostream>
#include <fuse.h>
#include <getopt.h>

/* C++ includes */
#include <libconfig.h++>
#include <sstream>

/* project includes */
#include "command-line.h"
#include "configuration.h"
#include "settings.h"

namespace efsng {

/*! Whatever */
settings::settings() 
    : m_exec_name("none"),
      m_daemonize(false),
      m_debug(false),
      m_root_dir("none"),
      m_mount_point("none"),
      m_config_file("none"),
      m_log_file("none"),
      m_fuse_argc(0),
      m_fuse_argv() { 
}

settings::settings(const settings& other) 
    : m_exec_name(other.m_exec_name),
      m_daemonize(other.m_daemonize),
      m_debug(other.m_debug),
      m_root_dir(other.m_root_dir),
      m_mount_point(other.m_mount_point),
      m_config_file(other.m_config_file),
      m_log_file(other.m_log_file),
      m_files_to_preload(other.m_files_to_preload),
      m_fuse_argc(other.m_fuse_argc),
      m_fuse_argv() {

    for(int i=0; i<s_max_fuse_args; ++i){
        if(other.m_fuse_argv[i] != NULL){
            size_t n = strlen(other.m_fuse_argv[i]);
            char* arg_copy = strndup(other.m_fuse_argv[i], n+1);
            arg_copy[n] = '\0';
            this->m_fuse_argv[i] = arg_copy;

        }
        else{
            this->m_fuse_argv[i] = NULL;
        }
    }
}

settings& settings::operator=(settings&& other) {

    if(&other != this){
        // make sure that we free all the state associated to this
        this->reset();

        // non-PODs can be moved with std::move
        m_exec_name = std::move(other.m_exec_name);
        m_root_dir = std::move(other.m_root_dir);
        m_mount_point = std::move(other.m_mount_point);
        m_config_file = std::move(other.m_config_file);
        m_log_file = std::move(other.m_log_file);
        m_files_to_preload = std::move(other.m_files_to_preload);

        // PODs need to be reset after copying
        m_daemonize = other.m_daemonize;
        other.m_daemonize = false;
        m_debug = other.m_debug;
        other.m_debug = false;
        m_fuse_argc = other.m_fuse_argc;
        other.m_fuse_argc = 0;

        for(int i=0; i<s_max_fuse_args; ++i){
            this->m_fuse_argv[i] = other.m_fuse_argv[i];
            other.m_fuse_argv[i] = NULL;
        }
    }

    return *this;
}

settings::~settings() {
    for(int i=0; i<s_max_fuse_args; ++i){
        if(m_fuse_argv[i] != NULL){
            free((void*) m_fuse_argv[i]);
        }
    }
}

void settings::reset() {
    m_exec_name = "none";
    m_daemonize = false;
    m_debug = false;
    m_root_dir = "none";
    m_mount_point = "none";
    m_config_file = "none";
    m_log_file = "none";
    m_fuse_argc = 0;

    for(int i=0; i<s_max_fuse_args; ++i){
        if(m_fuse_argv[i] != NULL){
            free((void*) m_fuse_argv[i]);
        }
    }
    memset(m_fuse_argv, 0, s_max_fuse_args);
}

void settings::from_cmdline(int argc, char* argv[]){

    /* helper lambda function to safely add an argument to fuse_argv */
    auto push_arg = [&](const char* arg, int pos=-1){

        assert(this->m_fuse_argc < s_max_fuse_args);

        char* arg_copy = NULL;

        if(arg != NULL){
            size_t n = strlen(arg);
            arg_copy = strndup(arg, n+1);
            arg_copy[n] = '\0';
        }

        if(pos != -1){
            if(pos >= s_max_fuse_args){
                if(arg_copy != NULL){
                    free(arg_copy);
                }
                return;
            }

            // add to user-defined position
            this->m_fuse_argv[pos] = arg_copy;
            return;
        }

        // add to next empty position and increase fuse_argc
        this->m_fuse_argv[this->m_fuse_argc++] = arg_copy;
    };


    /* pass through (and remember) executable name */
    std::string exec_name = bfs::basename(argv[0]);
    m_exec_name = exec_name;
    push_arg(exec_name.c_str()); // fuse_argv[0]

    /* daemonize by default if no options prevent it */
    m_daemonize = true;

    /* leave a space for the mount point given that FUSE expects the mount point before any flags */
    push_arg(NULL); // fuse_argv[1]

    /* command line options */
    struct option long_options[] = {
        {"root-dir",            1, 0, 'r'}, /* directory to mirror */
        {"mount-point",         1, 0, 'm'}, /* mount point */
        {"config-file",         1, 0, 'c'}, /* configuration file */
        {"help",                0, 0, 'h'}, /* display usage */
        {"foreground",          0, 0, 'f'}, /* foreground operation */
        {"debug",               0, 0, 'd'}, /* enfs-ng debug mode */
        {"log-file",            1, 0, 'l'}, /* log to file */
        {"fuse-debug",          0, 0, 'D'}, /* FUSE debug mode */
        {"fuse-single-thread",  0, 0, 'S'}, /* FUSE debug mode */
        {"fuse-help",           0, 0, 'H'}, /* fuse_mount usage */
        {"version",             0, 0, 'V'}, /* version information */
        {0, 0, 0, 0}
    };

    /* build opt_string automatically to pass to getopt_long */ 
    std::string opt_string;
    int num_options = sizeof(long_options)/sizeof(long_options[0]) - 1;

    ::opterr = 0; // disable getopt's default message for invalid options

    for(int i=0; i<num_options; i++){
        opt_string += static_cast<char>(long_options[i].val);

        if(long_options[i].has_arg){
            opt_string += ":";
        }
    }

    /* o: arguments directly passed to FUSE */
    opt_string += ("o:");

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
                m_root_dir = std::string(optarg);
                break;
            case 'm':
                m_mount_point = std::string(optarg);
                break;
            case 'c':
                m_config_file = std::string(optarg);
                break;
            case 'h':
                cmdline::usage(exec_name);
                exit(EXIT_SUCCESS);
                break;
            case 'f':
                /* remember flag */
                m_daemonize = false;
                /* prevent that FUSE starts as a daemon */
                push_arg("-f");
                break;
            case 'd':
                /* remember flag */
                m_daemonize = false;
                /* enable debug mode */
                m_debug = true;
                /* prevent that FUSE starts as a daemon */
                push_arg("-f");
                break;
            case 'l':
                /* log to file */
                m_log_file = std::string(optarg);
                break;
            case 'D':
                /* FUSE debug mode */
                push_arg("-d");
                break;
            case 'S':
                /* FUSE single thread mode */
                push_arg("-s");
                break;
            case 'H':
                cmdline::fuse_usage(exec_name);
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
                if(optopt != 0){
                    std::string msg = "Invalid option: ";
                    msg += (char) optopt;
                    throw std::invalid_argument(msg);
                }
                else{
                    std::string msg = "Invalid option: ";
                    msg += argv[optind-1];
                    throw std::invalid_argument(msg);
                }
            case ':':
                /* missing parameter for option */
                throw std::invalid_argument("Missing parameter");
            default:
                break;
        }
    }

    // read settings from configuration file, if one was passed at mount time.
    // Notice that command line arguments will always have precedence over 
    // configuration file options
    if(m_config_file != ""){

        settings backup(*this);

        try {
            from_file(m_config_file);
        }
        catch(const std::exception& e) {
            *this = std::move(backup);
            std::stringstream ss;
            ss << "Errors detected in configuration file. Ignored.\n";
            ss << "    [" << e.what() << "]";
            throw std::invalid_argument(ss.str());
        }
    }

    if(m_root_dir == m_mount_point) {
        throw std::invalid_argument("The root directory and the mount point must be different.");
    }

    if(m_root_dir != ""){
        std::string option = "modules=subdir,subdir=";
        option += m_root_dir.c_str();
        push_arg("-o");
        push_arg(option.c_str());
    }

    //if(m_log_file != "none"){
    //    init_logger(m_log_file);
    //}

    /** 
     * other default options for FUSE:
     * - nonempty: needed to allow the filesystem to be mounted on top of non empty directories.
     * - use_ino: needed to allow files to retain their original inode numbers.
     * - attr_timeout=0: set cache timeout for names to 0s
     */
    push_arg("-o");
#if FUSE_USE_VERSION < 30
    push_arg("nonempty,use_ino,attr_timeout=0,big_writes");
#else
    push_arg("attr_timeout=0");
#endif

    /* if there are still extra unparsed arguments, pass them onto FUSE */
    if(optind < argc){
        push_arg(argv[optind]);
        ++optind;
    }

    /* fill in the mount point for FUSE */
    push_arg(m_mount_point.string().c_str(), 1 /*fuse_argv[1]*/);
}

void settings::from_file(const bfs::path& config_file) {

    libconfig::Config cfg;

    try {
        cfg.readFile(config_file.c_str());
    }
    catch(const libconfig::FileIOException& fioex){
        throw std::invalid_argument("I/O error while reading configuration file");
    }
    catch(const libconfig::ParseException& pex){
        std::stringstream ss;
        ss << "Parse error at " << pex.getFile() << ":" << pex.getLine();
        throw std::invalid_argument(ss.str());
    }

    const libconfig::Setting& root = cfg.getRoot();

    /* parse 'root-dir' */
    try{
        const libconfig::Setting& cfg_root_dir = root["efs-ng"]["root-dir"];
        std::string optval = cfg_root_dir;

        /* command-line arguments override the options passed in the configuration file. 
         * Thus, if 'root_dir' already has a value different from the default one, ignore the passed cfg_value */
        if(m_root_dir == "none"){
            m_root_dir = std::string(optval);
        }
    }
    catch(const libconfig::SettingNotFoundException& nfex){
        if(m_root_dir == "none") {
            throw std::runtime_error("No root-dir defined");
        }
    }

    /* parse 'mount-point' */
    try{
        const libconfig::Setting& cfg_mount_point = root["efs-ng"]["mount-point"];
        std::string optval = cfg_mount_point;

        /* command-line arguments override the options passed in the configuration file. 
         * Thus, if 'root_dir' already has a value different from the default one, ignore the passed cfg_value */
        if(m_mount_point == "none"){
            m_mount_point = std::string(optval);
        }
    }
    catch(const libconfig::SettingNotFoundException& nfex){
        if(m_mount_point == "none") {
            throw std::runtime_error("No mount-point defined");
        }
    }

    /* parse 'log-file' */
    try {
        const libconfig::Setting& cfg_logfile = root["efs-ng"]["log-file"];
        std::string optval = cfg_logfile;

        /* command-line arguments override the options passed in the configuration file. 
         * Thus, if 'log_file' already has a value different from the default one, ignore the passed cfg_value */
        if(m_log_file == "none"){
            m_log_file = std::string(optval);
        }
    }
    catch(const libconfig::SettingNotFoundException& nfex){
        /* ignore, not a problem */
    }

    /* parse 'backend-stores' */
    try{
        const libconfig::Setting& cfg_backend_stores = root["efs-ng"]["backend-stores"];
        int count = cfg_backend_stores.getLength();

        for(int i=0; i<count; ++i){

            const libconfig::Setting& cfg_backend_store = cfg_backend_stores[i];
            std::string bend_type;
            kv_list bend_opts;

            for(int j=0; j<cfg_backend_store.getLength(); ++j){
                const libconfig::Setting& opt = cfg_backend_store[j];

                std::string opt_name = opt.getName();
                std::string opt_value = opt;

                if(opt_name == "type"){
                    bend_type = opt_value;
                }

                bend_opts.push_back({opt_name, opt_value});
            }

            if(bend_type != ""){
                m_backend_opts.insert({bend_type, bend_opts});
            }
        }
    }
    catch(const libconfig::SettingNotFoundException& nfex){
        throw std::runtime_error("No backend-stores defined");
    }

    /* parse 'preload-files' */
    try{
        const libconfig::Setting& cfg_files_to_preload = root["efs-ng"]["preload"];

        for(int i=0; i<cfg_files_to_preload.getLength(); ++i){

            const libconfig::Setting& filedef = cfg_files_to_preload[i];

            bfs::path filename;
            std::string backend;

            for(int j=0; j<filedef.getLength(); ++j){

                const libconfig::Setting& opt = filedef[j];

                std::string opt_name = opt.getName();
                std::string opt_value = opt;

                if(opt_name == "path"){
                    filename = opt_value;
                }
                else if(opt_name == "backend"){
                    backend = opt_value;
                }
                else{
                    std::stringstream ss;
                    ss << "Unsuported parameter '" << opt_name << "'";
                    throw std::invalid_argument(ss.str());
                }
            }

            m_files_to_preload.insert({filename, backend});
        }
    }
    catch(const libconfig::SettingNotFoundException& nfex){
        /* ignore, not a problem */
    }
}

} // namespace efsng
