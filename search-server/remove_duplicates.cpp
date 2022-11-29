
#include "remove_duplicates.h"

void RemoveDuplicates(SearchServer& search_server) {
    const auto documents = search_server.GetDocumentsByWords();

    std::set<int> ids_to_del;

    for (auto it = documents.begin(); it != documents.end(); std::advance(it, 1)) {
        auto origin_doc = *it;

        for (auto next_it = next(it, 1); next_it != documents.end(); std::advance(next_it, 1)) {

            auto doc = *next_it;

            if (origin_doc.second == doc.second && ids_to_del.count(doc.first) == 0) {
                ids_to_del.insert(doc.first);
            }
        }
    }

    for (const int id : ids_to_del) {
        std::cout << "Found duplicate document id " << id << "\n";
        search_server.RemoveDocument(id);
    }
}