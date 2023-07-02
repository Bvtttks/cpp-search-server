#include "string_processing.h"
#include <execution>
#include <string>
#include <vector>
#include <set>
#include <iostream>
using namespace std;

std::vector<std::string_view> SplitIntoWords(const std::string_view s) {
    std::string_view str = s;
    std::vector<std::string_view> result;
    
    while (true) {
        size_t space = str.find(' ');
        result.push_back(str.substr(0, space));
        if (space == str.npos) {
            break;
        } else {
            str.remove_prefix(space + 1);
        }
    }
    return result;
}