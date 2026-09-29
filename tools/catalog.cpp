#include "Tools/Catalog.h"
#include <cstdio>
#include <exception>
#include <fstream>
#include <iostream>
#include <span>

int main(int a_count, char** a_args) {
    try {
        const std::span arguments(a_args, static_cast<std::size_t>(a_count));
        const auto catalog = Bench::Json {{"tools", Bench::Catalog()}}.dump(2);
        if (arguments.size() > 1) {
            std::ofstream output(arguments[1], std::ios::binary);
            output << catalog << '\n';
            return output.good() ? 0 : 1;
        }
        std::cout << catalog;
        return 0;
    } catch (const std::exception& error) {
        std::fputs(error.what(), stderr);
        return 1;
    } catch (...) {
        return 1;
    }
}
