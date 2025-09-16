#pragma once

// Compiler
#if defined(__clang__)
    #define TAV_COMPILER_CLANG 1
#else
    #define TAV_COMPILER_CLANG 0
#endif

#if defined(__GNUC__) && !defined(__clang__)
    #define TAV_COMPILER_GCC 1
#else
    #define TAV_COMPILER_GCC 0
#endif

#if defined(_MSC_VER)
    #define TAV_COMPILER_MSVC 1
#else
    #define TAV_COMPILER_MSVC 0
#endif

// Platform
#if defined(_WIN32)
    #define TAV_PLATFORM_WINDOWS 1
#else
    #define TAV_PLATFORM_WINDOWS 0
#endif

#if defined(__APPLE__)
    #define TAV_PLATFORM_APPLE 1
#else
    #define TAV_PLATFORM_APPLE 0
#endif

#if defined(__linux__)
    #define TAV_PLATFORM_LINUX 1
#else
    #define TAV_PLATFORM_LINUX 0
#endif

#if defined(__ANDROID__)
    #define TAV_PLATFORM_ANDROID 1
#else
    #define TAV_PLATFORM_ANDROID 0
#endif

// Architecture
#if defined(__x86_64__) || defined(_M_X64)
    #define TAV_ARCH_X64 1
#else
    #define TAV_ARCH_X64 0
#endif

#if defined(__i386__) || defined(_M_IX86)
    #define TAV_ARCH_X86 1
#else
    #define TAV_ARCH_X86 0
#endif

#if defined(__aarch64__)
    #define TAV_ARCH_ARM64 1
#else
    #define TAV_ARCH_ARM64 0
#endif

#if defined(__arm__)
    #define TAV_ARCH_ARM32 1
#else
    #define TAV_ARCH_ARM32 0
#endif

// Build
#if defined(_DEBUG)
    #define TAV_DEBUG   1
    #define TAV_RELEASE 0
#else
    #define TAV_DEBUG   0
    #define TAV_RELEASE 1
#endif
