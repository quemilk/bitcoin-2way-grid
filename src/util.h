#pragma once

#include "type.h"
#include <vector>

std::string calcHmacSHA256(const std::string& msg, const std::string& decoded_key);

std::string toTimeStr(uint64_t time_msec);

int generateRadomInt(int min_value, int max_value);

std::string generateRandomString(size_t length);

void trimString(string& str);
void splitString(const string& v, std::vector<string>& out, char delim, size_t max_seg = -1);

inline std::string toString(OrderSide side) {
    return (side == OrderSide::Buy) ? "buy" : "sell";
}