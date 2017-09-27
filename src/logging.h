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

#ifndef __LOGGING_H__
#define __LOGGING_H__

#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>
#include <sstream>

namespace efsng {

class logger {

    static const int32_t queue_size = 8192; // must be a power of 2

public:
    logger(const std::string& ident, const std::string& type, const std::string& logfile="") {

        try {

            if(type == "console") {
                //spdlog::set_async_mode(queue_size);
                m_internal_logger = spdlog::stdout_logger_mt(ident);
            }
            else if(type == "console color") {
                spdlog::set_async_mode(queue_size);
                m_internal_logger = spdlog::stdout_color_mt(ident);
            }
#ifdef SPDLOG_ENABLE_SYSLOG 
            else if(type == "syslog") {
                m_internal_logger = spdlog::syslog_logger("syslog", ident, LOG_PID);
            }
#endif
            else if(type == "file") {
                 m_internal_logger = spdlog::basic_logger_mt(ident, logfile);
            }
            else {
                throw std::invalid_argument("Unknown logger type: '" + type + "'");
            }

            assert(m_internal_logger != nullptr);

            // %Y - Year in 4 digits
            // %m - month 1-12
            // %d - day 1-31
            // %T - ISO 8601 time format (HH:MM:SS)
            // %f - microsecond part of the current second
            // %E - epoch (microseconds precision)
            // %n - logger's name
            // %t - thread id
            // %l - log level
            // %v - message
            m_internal_logger->set_pattern("[%Y-%m-%d %T.%f] [%E] [%n] [%t] [%l] %v");


            spdlog::drop_all();

            // globally register the logger so that it can be accessed 
            // using spdlog::get(logger_name)
            spdlog::register_logger(m_internal_logger);
        }
        catch(const spdlog::spdlog_ex& ex) {
            throw std::runtime_error("logger initialization failed: " + std::string(ex.what()));
        }
    }

    ~logger(){
        spdlog::drop_all();
    }

    inline void enable_debug() const {
        m_internal_logger->set_level(spdlog::level::debug);
    }

    inline void flush() {
        m_internal_logger->flush();
    }

    template <typename... Args>
    inline void info(const char* fmt, const Args&... args) {
        m_internal_logger->info(fmt, args...);
    }

    template <typename... Args>
    inline void debug(const char* fmt, const Args&... args) {
        m_internal_logger->debug(fmt, args...);
    }

    template <typename... Args>
    inline void warn(const char* fmt, const Args&... args) {
        m_internal_logger->warn(fmt, args...);
    }

    template <typename... Args>
    inline void error(const char* fmt, const Args&... args) {
        m_internal_logger->error(fmt, args...);
    }

    template <typename... Args>
    inline void critical(const char* fmt, const Args&... args) {
        m_internal_logger->critical(fmt, args...);
    }

    template <typename T>
    inline void info(const T& msg) {
        m_internal_logger->info(msg);
    }

    template <typename T>
    inline void debug(const T& msg) {
        m_internal_logger->debug(msg);
    }

    template <typename T>
    inline void warn(const T& msg) {
        m_internal_logger->warn(msg);
    }

    template <typename T>
    inline void error(const T& msg) {
        m_internal_logger->error(msg);
    }

    template <typename T>
    inline void critical(const T& msg) {
        m_internal_logger->critical(msg);
    }

    template <typename... Args>
    static inline std::string build_message(Args&&... args) {

        // see:
        // 1. https://stackoverflow.com/questions/27375089/what-is-the-easiest-way-to-print-a-variadic-parameter-pack-using-stdostream
        // 2. https://stackoverflow.com/questions/25680461/variadic-template-pack-expansion/25683817#25683817

        std::stringstream ss;

        using expander = int[];
        (void) expander{0, (void(ss << std::forward<Args>(args)),0)...};

        return ss.str();
    }

private:
    std::shared_ptr<spdlog::logger> m_internal_logger;
    std::string                     m_type;
};

} // namespace efsng

#endif /* __LOGGING_H__ */
