#pragma once
#include "search_server.h"
#include "document.h"
#include <string>
#include <deque>
#include <vector>


class RequestQueue {
public:
    explicit RequestQueue(const SearchServer& search_server);

    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate);

    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status);

    std::vector<Document> AddFindRequest(const std::string& raw_query);

    int GetNoResultRequests() const;

private:
    int time_in_general_ = 0;
    struct QueryResult {
        int add_time;
        std::vector<Document> content;
    };
    std::deque<QueryResult> requests_;
    const static int min_in_day_ = 1440;
    const SearchServer& search_server_;
    int no_result_num_ = 0;

    void AddResut(const std::vector<Document> content);
};

template <typename DocumentPredicate>
std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {
    const std::vector<Document> result = search_server_.FindTopDocuments(raw_query, document_predicate);
    AddResut(result);
    return result;

}