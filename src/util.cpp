#include "util.h"
#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <random>
#include <array>


std::string calcHmacSHA256(const std::string& msg, const std::string& decoded_key) {
    std::array<unsigned char, EVP_MAX_MD_SIZE> hash;
    unsigned int hashLen;

    HMAC(
        EVP_sha256(),
        decoded_key.data(),
        static_cast<int>(decoded_key.size()),
        reinterpret_cast<unsigned char const*>(msg.data()),
        static_cast<int>(msg.size()),
        hash.data(),
        &hashLen
    );

    return std::string{ reinterpret_cast<char const*>(hash.data()), hashLen };
}

std::string toTimeStr(uint64_t time_msec) {
    struct tm tstruct;
    char buf[80];
    time_t now = time_msec / 1000;
    tstruct = *localtime(&now);
    strftime(buf, _countof(buf), "%m-%d %H:%M:%S", &tstruct);

    return buf;
}

int generateRadomInt(int min_value, int max_value) {
    // genenrate random uuid of device
    static std::random_device r;
    static std::seed_seq seed{ r(), r(), r(), r(), r(), r(), r(), r() };
    static std::mt19937_64 eng(seed);

    std::uniform_int_distribution<int> dist{ min_value, max_value };
    return dist(eng);
}

std::string generateRandomString(size_t length) {
    static const std::string k_alpha = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZjJ";

    std::string s;
    s.reserve(length);
    for (size_t i = 0; i < length; ++i)
        s.push_back(k_alpha[generateRadomInt(0, (int)(k_alpha.size() - 1))]);
    return s;
}
