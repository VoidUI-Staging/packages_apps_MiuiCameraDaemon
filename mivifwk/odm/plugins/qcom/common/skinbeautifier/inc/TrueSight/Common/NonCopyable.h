#pragma once

namespace TrueSight {

/**
 * protocol class. used to delete assignment operator.
 */
class NonCopyable {
public:
    NonCopyable()                    = default;
    NonCopyable(const NonCopyable&)  = delete;
    NonCopyable(const NonCopyable&&) = delete;
    NonCopyable& operator=(const NonCopyable&) = delete;
    NonCopyable& operator=(const NonCopyable&&) = delete;
};

} // namespace TrueSight