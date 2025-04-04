#pragma once

#include <cstdint>
#include <iostream>
#include <vector>

struct Ciptrace {
    Ciptrace(char* path);
    std::vector<std::vector<int64_t>> timeframes;
};
std::ostream& operator<<(std::ostream& os, const Ciptrace &trace);
