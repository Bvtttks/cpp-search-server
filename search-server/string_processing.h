#pragma once
#include <execution>
#include <string>
#include <vector>
#include <set>

std::vector<std::string_view> SplitIntoWords(const std::string_view s);
    
template <typename StringContainer>
std::set<std::string, std::less<>> MakeUniqueNonEmptyStrings(const StringContainer& strings) {
    std::set<std::string, std::less<>> non_empty_strings;
    for (const auto str : strings) {
        if (!str.empty())
            non_empty_strings.insert(std::string{str});
    }
    return non_empty_strings;
}
