#include "process_queries.h"
#include <execution>
#include "log_duration.h"
#include <numeric>


std::vector<std::vector<Document>> ProcessQueries(
    const SearchServer& search_server,
    const std::vector<std::string>& queries) {

    if (queries.empty()) {
        return {}; 
    }

    std::vector<std::vector<Document>> result(queries.size());

    std::transform(std::execution::par, queries.begin(), queries.end(), result.begin(), [&search_server](const std::string& s) noexcept {
        return search_server.FindTopDocuments(s);
    });

    return result;
}


std::list<Document> ProcessQueriesJoined(
    const SearchServer& search_server,
    const std::vector<std::string>& queries) {

    const auto top_documents = ProcessQueries(search_server, queries);

    std::list<Document> result;

    for (const auto a : top_documents) {

        std::list<Document> temp(a.size());

        std::transform(std::execution::par, a.begin(), a.end(), temp.begin(), [](const Document doc) { return doc;});

        result.insert(result.end(), temp.begin(), temp.end()); 
    }

    return result;

}