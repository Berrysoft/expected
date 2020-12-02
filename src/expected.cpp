#include <boost/stacktrace/frame.hpp>
#include <boost/stacktrace/stacktrace.hpp>
#include <expected.hpp>

using namespace boost::stacktrace;

namespace expected::impl
{
    std::string get_stacktrace()
    {
        return to_string(stacktrace{});
    }
} // namespace expected::impl
