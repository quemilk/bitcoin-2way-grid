#pragma once

#include "type.h"

std::string calcHmacSHA256(const std::string& msg, const std::string& decoded_key);

std::string toTimeStr(uint64_t time_msec);

int generateRadomInt(int min_value, int max_value);

std::string generateRandomString(size_t length);