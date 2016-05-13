#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/support/date_time.hpp>

#include "logging.h"

namespace blog = boost::log;
namespace blkeywords = blog::keywords;
namespace blexpr = blog::expressions;
namespace blattrs = blog::attributes;

namespace efsng {

void init_logger(){

    blog::add_common_attributes();

    blog::add_file_log(
        blkeywords::file_name = "efsng.%N.log",
        blkeywords::rotation_size = 10 * 1024 * 1024,
        blkeywords::format =
        (
            blexpr::stream
                << "[" << blexpr::format_date_time<boost::posix_time::ptime>("TimeStamp","%Y-%m-%d %H:%M:%S.%f") << "]"
                << " [" << std::dec << blexpr::attr<blattrs::current_thread_id::value_type>("ThreadID") << "]"
                << " [" << blog::trivial::severity << "]"
                << "\t" << blexpr::smessage
        )
    );
}

void init_logger(const bfs::path& log_file){

    blog::add_common_attributes();

    blog::add_file_log(
        blkeywords::file_name = log_file,
        blkeywords::format =
        (
            blexpr::stream
                << "[" << blexpr::format_date_time<boost::posix_time::ptime>("TimeStamp","%Y-%m-%d %H:%M:%S.%f") << "]"
                << " [" << std::dec << blexpr::attr<blattrs::current_thread_id::value_type>("ThreadID") << "]"
                << " [" << blog::trivial::severity << "]"
                << "\t" << blexpr::smessage
        )
    );
}

} // namespace efsng
