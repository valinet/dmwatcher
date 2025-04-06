// Copyright 2025 VALINET Solutions SRL
// Author(s): Valentin Radu (valentin.radu@valinet.ro)
//
#pragma once
#ifdef _WIN32
#define MyGetLastError() GetLastError()
#else
#define MyGetLastError() errno
#endif
#define myassert(condition, message) \
    do { \
        if (! (condition)) { \
            std::cerr << "Assertion `" #condition "` failed in " << __FILE__ \
                      << " line " << __LINE__ << ": " << message \
                      << " (GetLastError/errno: " << MyGetLastError() << ")" \
                      << std::endl; \
            exit(__LINE__); \
        } \
    } while (false)
