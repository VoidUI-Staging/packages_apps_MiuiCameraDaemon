#ifndef __NONE__
#define __NONE__

namespace almalence {

namespace detail {
struct none_helper
{
};
} // namespace detail

typedef int detail::none_helper::*None;

None const none = (static_cast<None>(0));

} // namespace almalence

#endif
