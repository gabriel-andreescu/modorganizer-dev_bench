#pragma once
#include <cstdio>
#include <exception>
#include <functional>
#include <initializer_list>

namespace Tests {
// Returns the first nonzero check result, or 0.
inline int First(std::initializer_list<std::function<int()>> a_checks) {
    for (const auto& check : a_checks) {
        if (const auto code = check(); code != 0) {
            return code;
        }
    }
    return 0;
}

// Runs a test program and reports escaping exceptions as failures.
inline int Run(const std::function<int()>& a_program) noexcept {
    try {
        return a_program();
    } catch (const std::exception& error) {
        std::fputs(error.what(), stderr);
        return 100;
    } catch (...) {
        return 101;
    }
}
}
