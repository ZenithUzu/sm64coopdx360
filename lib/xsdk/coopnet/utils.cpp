#ifdef OSX_BUILD
#include <mach-o/dyld.h>
#endif

#include <stdexcept>
#include <string>
#include <ctime>
#include <fstream>
#include "socket.hpp"

#if defined(__APPLE__)
// for _NSGetExecutablePath
#include <mach-o/dyld.h>
#endif

// Convert a domain name to an in_addr using gethostbyname
in_addr_t GetAddrFromDomain(const std::string& domain) {
#ifdef __XSDK__
    // xSDK resolves names through getaddrinfo only.
    struct addrinfo hints = {};
    struct addrinfo* result = nullptr;
    hints.ai_family = AF_INET;
    if (getaddrinfo(domain.c_str(), nullptr, &hints, &result) != 0 || !result) {
        return htonl(INADDR_LOOPBACK);
    }
    in_addr_t addr = ((struct sockaddr_in*)result->ai_addr)->sin_addr.s_addr;
    freeaddrinfo(result);
    return addr;
#else
    struct hostent* he = gethostbyname(domain.c_str());
    if (he == nullptr) {
        he = gethostbyname("127.0.0.1");
    }
    auto addr_list = reinterpret_cast<in_addr**>(he->h_addr_list);
    return addr_list[0]->s_addr;
#endif
}

static void _clock_gettime(struct timespec* clock_time) {
#if !defined _POSIX_MONOTONIC_CLOCK || _POSIX_MONOTONIC_CLOCK < 0
    clock_gettime(CLOCK_REALTIME, clock_time);
#elif _POSIX_MONOTONIC_CLOCK > 0
    clock_gettime(CLOCK_MONOTONIC, clock_time);
#else
    if (clock_gettime(CLOCK_MONOTONIC, clock_time)) {
        clock_gettime(CLOCK_REALTIME, clock_time);
    }
#endif
}

static uint64_t clock_elapsed_ns(void) {
    static bool sClockInitialized = false;
    static uint64_t clock_start_ns;
    if (!sClockInitialized) {
        struct timespec clock_start = { 0 };
        _clock_gettime(&clock_start);
        clock_start_ns = ((uint64_t)clock_start.tv_sec) * 1000000000 + ((uint64_t)clock_start.tv_nsec);
        sClockInitialized = true;
    }

    struct timespec clock_current = { 0 };
    _clock_gettime(&clock_current);

    uint64_t clock_current_ns = ((uint64_t)clock_current.tv_sec) * 1000000000 + ((uint64_t)clock_current.tv_nsec);
    return (clock_current_ns - clock_start_ns);
}

float clock_elapsed(void) {
    return (clock_elapsed_ns() / 1000000000.0f);
}

std::string getExecutablePath() {
    char path[0xFF];
#if defined(_WIN32)
    if (GetModuleFileNameA(nullptr, path, MAX_PATH) != 0) {
        return std::string(path);
    }
#elif defined(__linux__)
    ssize_t len = readlink("/proc/self/exe", path, sizeof(path) - 1);
    if (len != -1) {
        path[len] = '\0';
        return std::string(path);
    }
#elif defined(__APPLE__)
    uint32_t size = sizeof(path);
    if (_NSGetExecutablePath(path, &size) == 0) {
        return std::string(path);
    }
#elif defined(__XSDK__)
    (void)path;
    return "game:/sm64coopdx.xex";
#endif
    return "";
}

static std::string readFileData(const std::string &filepath) {
    if (filepath == "") { return ""; }
    std::ifstream file(filepath, std::ios::binary | std::ios::ate);
    if (!file) throw std::runtime_error("Cannot open file.");
    std::streamsize fileSize = file.tellg();
    file.seekg(0, std::ios::beg);
    std::string data(fileSize, '\0');
    if (!file.read(&data[0], fileSize)) throw std::runtime_error("Cannot read file data.");
    return data;
}

std::size_t hashFile(const std::string &filepath = getExecutablePath()) {
#ifdef __XSDK__
    // The whole XEX in memory, on a console that may be short of it: a hash isn't worth stopping the game.
    try {
        const std::string data = readFileData(filepath);
        return data == "" ? 0 : std::hash<std::string>{}(data);
    } catch (...) {
        return 0;
    }
#else
    const std::string data = readFileData(filepath);
    if (data == "") { return 0; }
    return std::hash<std::string>{}(data);
#endif
}
