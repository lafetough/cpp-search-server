
#include "remove_duplicates.h"

void RemoveDuplicates(SearchServer& search_server) {

    std::map<std::set<std::string_view>, int> words_docs;
    std::set<int> ids_to_del;

    for (const int id : search_server) {
        const auto temp = search_server.GetWordFrequencies(id);
        std::set<std::string_view> set_to_push;
        for (const auto& words : temp) {
            set_to_push.insert(words.first);
        }
        if (words_docs.count(set_to_push)) {
            ids_to_del.insert(id);
            continue;
        }
        words_docs[set_to_push] = id;
    }

    for (const auto id : ids_to_del) {
        std::cout << "Found duplicate document id " << id << "\n";
        search_server.RemoveDocument(id);
    }

}