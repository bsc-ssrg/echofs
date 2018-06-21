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
#include <yaml-cpp/yaml.h>
#include <sstream>

/* project includes */
#include "command-line.h"
//#include "configuration.h"
#include "defaults.h"
#include "settings.h"
#include "file-options.h"
#include "config-schema.h"

namespace efsng {
namespace config {

/*! Whatever */
settings::settings() 
    : m_exec_name("none"),
      m_daemonize(false),
      m_debug(false),
      m_root_dir("none"),
      m_mount_dir("none"),
      m_results_dir("none"),
      m_config_file("none"),
      m_log_file("none"),
      m_workers(0),
      m_transfer_size(0),
      m_api_sockfile(defaults::api_sockfile),
      m_fuse_argc(0),
      m_fuse_argv() { 
}

settings::settings(const settings& other) 
    : m_exec_name(other.m_exec_name),
      m_daemonize(other.m_daemonize),
      m_debug(other.m_debug),
      m_root_dir(other.m_root_dir),
      m_mount_dir(other.m_mount_dir),
      m_results_dir(other.m_results_dir),
      m_config_file(other.m_config_file),
      m_log_file(other.m_log_file),
      m_workers(other.m_workers),
      m_transfer_size(other.m_transfer_size),
      m_api_sockfile(other.m_api_sockfile),
      m_backend_opts(other.m_backend_opts),
      m_resources(other.m_resources),
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
        m_mount_dir = std::move(other.m_mount_dir);
        m_results_dir = std::move(other.m_results_dir);
        m_config_file = std::move(other.m_config_file);
        m_log_file = std::move(other.m_log_file);
        m_workers = std::move(other.m_workers);
        m_transfer_size = std::move(other.m_transfer_size);
        m_api_sockfile = std::move(other.m_api_sockfile);
        m_backend_opts = std::move(other.m_backend_opts);
        m_resources = std::move(other.m_resources);

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
    m_mount_dir = "none";
    m_results_dir = "none";
    m_config_file = "none";
    m_log_file = "none";
    m_workers = 0;
    m_transfer_size = 0;
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
        {"mount-dir",           1, 0, 'm'}, /* mount point */
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
                m_mount_dir = std::string(optarg);
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
    if(m_config_file != "none"){

        settings backup(*this);

        try {
            load_from_yaml_file(m_config_file);
        }
        catch(const std::exception& e) {
            *this = std::move(backup);
            std::stringstream ss;
            ss << "Errors detected in configuration file. Ignored.\n";
            ss << "    [" << e.what() << "]";
            throw std::invalid_argument(ss.str());
        }
    }

    if(m_root_dir == m_mount_dir) {
        throw std::invalid_argument("The root directory and the mount point must be different.");
    }

    if(m_root_dir != ""){
        /*std::string option = "modules=subdir,subdir=";
        option += m_root_dir.c_str();
        push_arg("-o");
        push_arg(option.c_str());*/
    }

    //if(m_log_file != "none"){
    //    init_logger(m_log_file);
    //}
 
    if (m_config_file == "none")
    {   kv_list extra_opts;
        extra_opts.emplace ("daxfs","/tmp");
        m_backend_opts.emplace(
                "nvml://", 
                backend_options {
                    "nvml://", "NVRAM-NVML", 12000000, m_root_dir,  // 12 MB maximum segment
                    m_mount_dir, m_results_dir, extra_opts
                });
    }
    /** 
     * other default options for FUSE:
     * - nonempty: needed to allow the filesystem to be mounted on top of non empty directories.
     * - use_ino: needed to allow files to retain their original inode numbers.
     * - attr_timeout=0: set cache timeout for names to 0s
     */
    push_arg("-o");
#if FUSE_USE_VERSION < 30
    push_arg("nonempty,attr_timeout=0,big_writes");
#else
    push_arg("attr_timeout=0");
#endif

    /* if there are still extra unparsed arguments, pass them onto FUSE */
    if(optind < argc){
        push_arg(argv[optind]);
        ++optind;
    }
    #if FUSE_USE_VERSION < 30
    /* fill in the mount point for FUSE */
    push_arg(m_mount_dir.string().c_str(), 1 /*fuse_argv[1]*/);
    #endif
	
}

void settings::load_from_yaml_file(const bfs::path& config_file) {

    file_options::options_map opt_map;
    file_options::parse_yaml_file(config_file, config_schema, opt_map);

    auto& parsed_global_settings = opt_map.get_as<file_options::options_group>("global-settings");

    // 1. initialize global settings keeping in mind that command-line 
    // arguments must override any options passed in the configuration file. 
    if(m_root_dir == "none") {
        m_root_dir = parsed_global_settings.get_as<bfs::path>(keywords::root_dir);
    }

    if(m_mount_dir == "none") {
        m_mount_dir = parsed_global_settings.get_as<bfs::path>(keywords::mount_dir);
    }

    if(m_results_dir == "none" && parsed_global_settings.count(keywords::results_dir)) {
        m_results_dir = parsed_global_settings.get_as<bfs::path>(keywords::results_dir);
    }

    if(m_log_file == "none" && parsed_global_settings.count(keywords::log_file)) {
        if(m_debug) {
            // if the FUSE debug flag was passed, we ignore the log file
            std::cout << "WARNING: Ignoring 'log-file' option in debug mode (-d)\n";
            std::cout << "WARNING: Use foreground mode (-f), if you want messages redirected\n";
        }
        else {
            m_log_file = parsed_global_settings.get_as<bfs::path>(keywords::log_file);
        }
    }
    
    // 'workers' and 'transfer-size' don't have a command line option, 
    // no need to check if they have been already set
    // Also, we have set a default value for them so they HAVE TO be 
    // in parsed_global_settings
    m_workers = parsed_global_settings.get_as<uint32_t>(keywords::workers);
    m_transfer_size = parsed_global_settings.get_as<uint32_t>(keywords::transfer_size);

    // 2. initialize m_backend_opts with the parsed information
    // about any configured backends
    auto& parsed_backends = opt_map.get_as<file_options::options_list>(keywords::backends);

    for(const auto& pb : parsed_backends) {

        std::string id = pb.get_as<std::string>(keywords::id);
        std::string type = pb.get_as<std::string>(keywords::type);
        uint32_t capacity = pb.get_as<uint32_t>(keywords::capacity);
        kv_list extra_opts;

        for(const auto& kv : pb) {

            // ignore options we already know
            if(kv.first == keywords::id || kv.first == keywords::type || kv.first == keywords::capacity) {
                continue;
            }

            extra_opts.emplace(kv.first, kv.second.get_as<std::string>());
        }

        m_backend_opts.emplace(
                id, 
                backend_options {
                    id, type, capacity, m_root_dir, 
                    m_mount_dir, m_results_dir, extra_opts
                });
    }

    // 3. initialize m_resources
    auto& parsed_resources = opt_map.get_as<file_options::options_list>(keywords::resources);

    for(const auto& res : parsed_resources) {
        kv_list res_opts;

        for(const auto& kv : res) {
            res_opts.emplace(kv.first, kv.second.get_as<std::string>());
        }

        m_resources.emplace_back(res_opts);
    }
}

} // namespace config
} // namespace efsng
