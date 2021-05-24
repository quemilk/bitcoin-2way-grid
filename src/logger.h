#pragma once

#include <boost/log/trivial.hpp>

void init_logger();

#define LOG(lvl) BOOST_LOG_TRIVIAL(lvl)