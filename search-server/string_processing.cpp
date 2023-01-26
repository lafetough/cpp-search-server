#include "string_processing.h"


std::vector<std::string_view> SplitIntoWords(const std::string_view text) {
    std::vector<std::string_view> words;

    size_t start = text.find_first_not_of(' ');
    while (start != std::string_view::npos) {
        size_t end = text.find(' ', start);
        words.push_back(text.substr(start, end - start));
        start = text.find_first_not_of(' ', end);
    }
    return words;

}
