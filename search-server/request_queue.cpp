#include "request_queue.h"


RequestQueue::RequestQueue(const SearchServer& search_server)
    :search_server_(search_server)
{
}

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentStatus status) {
    const std::vector<Document> result = search_server_.FindTopDocuments(raw_query, status);
    AddResut(result);
    return result;
}

std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query) {
    const std::vector<Document> result = search_server_.FindTopDocuments(raw_query);
    AddResut(result);
    return result;
}

int RequestQueue::GetNoResultRequests() const {
    return no_result_num_;
}

void RequestQueue::AddResut(const std::vector<Document> content) {

    ++time_in_general_;

    if (!requests_.empty()) {
        while (min_in_day_ <= time_in_general_ - requests_.front().add_time) {
            --no_result_num_;
            requests_.pop_front();
        }
    }

    requests_.push_back({ time_in_general_, content });
    if (content.empty()) {
        ++no_result_num_;
    }

}