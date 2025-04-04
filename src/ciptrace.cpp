#include "ciptrace.hpp"

#include <algorithm>
#include <cassert>
#include <fstream>
#include <sstream>
#include <iostream>

Ciptrace::Ciptrace(char* path) {
    std::string line;
    std::ifstream stream(path);
    while (std::getline(stream, line)) {
        std::stringstream sstream(line);
        int64_t timeframe_value;
        std::string separator;
        sstream >> timeframe_value;
        assert(timeframe_value == timeframes.size());
        sstream >> separator;
        assert(separator == "=");
        unsigned char logic;
        std::vector<int64_t> timeframe;
        int64_t var { 1 };
        while (sstream >> logic && (logic == 'X' || logic == '0' || logic == '1')) {
            if (logic == '0')
                timeframe.push_back(-var);
            else if (logic == '1')
                timeframe.push_back(var);
            var++;
        }
        std::sort(std::begin(timeframe), std::end(timeframe));
        timeframes.push_back(timeframe);
    }
}

std::ostream& operator<<(std::ostream& os, const Ciptrace &trace) {
    for (size_t index { 0u }; index < trace.timeframes.size(); ++index) {
        os << "v" << index;
        for (auto& value : trace.timeframes[index])
            os << " " << value;
        os << " 0";
    }
    return os;
}
