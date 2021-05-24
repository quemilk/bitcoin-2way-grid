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


#endif
