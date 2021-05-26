#ifndef BASE_TYPE_H_
#define BASE_TYPE_H_

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <memory>
#include <sstream>
#include <boost/assert.hpp>
#include <boost/noncopyable.hpp>


using std::string;

#define countof(arg) (sizeof(arg) / sizeof(arg[0]))

#define ASSERT_UNREACHABLE() assert("unreachable code" == 0)

#define UNUSED(expr) do { (void)(expr); } while (0)


template<typename CallableT>
struct ScopeExit final {
    explicit ScopeExit(CallableT&& callable)
        : callable_(std::forward<CallableT>(callable)) {
    }

    ScopeExit(const ScopeExit&) = delete;
    ScopeExit(ScopeExit&&) = default;
    ScopeExit& operator=(const ScopeExit&) = delete;
    ScopeExit& operator=(ScopeExit&&) = default;

    ~ScopeExit() {
        if (!cancelled_) {
            callable_();
        }
    }

    void cancel() {
        cancelled_ = true;
    }

private:
    CallableT callable_;
    bool cancelled_{ false };
};

template<typename CallableT>
ScopeExit<CallableT>
make_scope_exit(CallableT && callable) {
    return ScopeExit<CallableT>(std::forward<CallableT>(callable));
}


#endif
