#pragma once

#include <android/log.h>

#define COLOR_WHITE "\033[1;37;1m"
#define COLOR_RED "\033[1;31m"
#define COLOR_GREEN "\033[1;32m"
#define COLOR_YELLOW "\033[1;33m"
#define COLOR_RESET "\033[m"

#define LOG_LEVEL_DEBUG 3
#define LOG_LEVEL_INFO 4
#define LOG_LEVEL_WARN 5
#define LOG_LEVEL_ERROR 6

#ifndef LOG_LEVEL
#define LOG_LEVEL LOG_LEVEL_DEBUG
#endif

#ifdef __LINUX__
#define LOG_COMMMON(name, color, fmt, ...)                                                                            \
    do                                                                                                                \
    {                                                                                                                 \
        printf(color "[%-5s] %s (%s #%d) " fmt COLOR_RESET "\n", name, __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__); \
    } while (0)
#endif

#ifdef __ANDROID__
#define LOG_COMMMON(level, name, color, fmt, ...) \
    do                                                                                                                \
    {                                                                                                                 \
        __android_log_print(level, "native", "[%-5s] %s:%d (%s) " fmt, name, __FILE__, __LINE__, __FUNCTION__, ##__VA_ARGS__); \
    } while (0)
#endif

#if LOG_LEVEL <= LOG_LEVEL_DEBUG
#define LOG_DEBUG(fmt, ...) LOG_COMMMON(ANDROID_LOG_DEBUG, "debug", COLOR_WHITE, fmt, ##__VA_ARGS__)
#else
#define LOG_DEBUG(fmt, ...)
#endif

#if LOG_LEVEL <= LOG_LEVEL_INFO
#define LOG_INFO(fmt, ...) LOG_COMMMON(ANDROID_LOG_INFO, "info", COLOR_GREEN, fmt, ##__VA_ARGS__)
#else
#define LOG_INFO(fmt, ...)
#endif

#if LOG_LEVEL <= LOG_LEVEL_WARN
#define LOG_WARN(fmt, ...) LOG_COMMMON(ANDROID_LOG_WARN, "warn", COLOR_YELLOW, fmt, ##__VA_ARGS__)
#else
#define LOG_WARN(fmt, ...)
#endif

#if LOG_LEVEL <= LOG_LEVEL_ERROR
#define LOG_ERROR(fmt, ...) LOG_COMMMON(ANDROID_LOG_ERROR, "error", COLOR_RED, fmt, ##__VA_ARGS__)
#else
#define LOG_ERROR(fmt, ...)
#endif

