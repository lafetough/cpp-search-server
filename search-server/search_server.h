#pragma once
#include "document.h"
#include <set>
#include <map>
#include <string>
#include <vector>
#include <algorithm>
#include "string_processing.h"
#include <execution>
#include <deque>
#include <type_traits>
#include <mutex>
#include "concurrent_map.h"

const int MAX_RESULT_DOCUMENT_COUNT = 5;

const double EPSILON = 1e-6;

class SearchServer {
public:

    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words);

    explicit SearchServer(const std::string& stop_words_text);

    explicit SearchServer(std::string_view stop_words_text);

    void AddDocument(int document_id, const std::string_view document, DocumentStatus status,
        const std::vector<int>& ratings);

    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(const std::string_view raw_query,
        DocumentPredicate document_predicate) const;
    std::vector<Document> FindTopDocuments(const std::string_view raw_query, DocumentStatus status) const;
    std::vector<Document> FindTopDocuments(const std::string_view raw_query) const;
    template <typename DocumentPredicate, typename ExePolicy>
    std::vector<Document> FindTopDocuments(const ExePolicy& policy, const std::string_view raw_query,
        DocumentPredicate document_predicate) const;
    template<typename ExePolicy>
    std::vector<Document> FindTopDocuments(const ExePolicy& policy, const std::string_view raw_query, DocumentStatus status) const;
    template<typename ExePolicy>
    std::vector<Document> FindTopDocuments(const ExePolicy& policy, const std::string_view raw_query) const;

    int GetDocumentCount() const;

    void RemoveDocument(int document_id);
    void RemoveDocument(const std::execution::sequenced_policy, int documnet_id);
    void RemoveDocument(const std::execution::parallel_policy, int document_id);

    std::set<int>::const_iterator begin() const;
    std::set<int>::const_iterator end() const;

    const std::map<std::string_view, double>& GetWordFrequencies(int document_id) const;

    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::string_view raw_query,
        int document_id) const;
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::execution::sequenced_policy, const std::string_view raw_query,
        int document_id) const;
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::execution::parallel_policy, const std::string_view& raw_query,
        int document_id) const;


private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };
    const std::set<std::string, std::less<>> stop_words_;
    std::deque<std::string> documents_strings_;
    std::map<std::string_view, std::map<int, double>> word_to_id_freqs_;
    std::map<int, std::map<std::string_view, double>> id_to_words_freq_;
    std::map<int, DocumentData> documents_;
    std::set<int> document_ids_;

    bool IsStopWord(const std::string_view word) const;

    static bool IsValidWord(const std::string_view word);

    std::vector<std::string_view> SplitIntoWordsNoStop(const std::string_view text) const;

    static int ComputeAverageRating(const std::vector<int>& ratings);

    struct QueryWord {
        std::string_view data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(const std::string_view text) const;

    struct Query {
        std::vector<std::string> plus_words;
        std::vector<std::string> minus_words;
    };

    Query ParseQuery(const std::string_view text) const;
    Query ParseQuery(const std::execution::parallel_policy, const std::string_view& text) const;

    double ComputeWordInverseDocumentFreq(const std::string_view word) const;

    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const Query& query,
        DocumentPredicate document_predicate) const;
    template <typename ExePolicy, typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const ExePolicy& policy, const Query& query,
        DocumentPredicate document_predicate) const;



};




//TEMPLATES --------------------------------------------------------------------------------------------------------------------------------------------------------------------


template <typename StringContainer>
SearchServer::SearchServer(const StringContainer& stop_words)
    : stop_words_(MakeUniqueNonEmptyStrings(stop_words))  // Extract non-empty stop words
{
    using namespace std;

    if (!all_of(stop_words_.begin(), stop_words_.end(), IsValidWord)) {
        throw std::invalid_argument("Some of stop words are invalid"s);
    }
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(const std::string_view raw_query,
    DocumentPredicate document_predicate) const {
    return FindTopDocuments(std::execution::seq, raw_query, document_predicate);
}

template <typename DocumentPredicate, typename ExePolicy>
std::vector<Document> SearchServer::FindTopDocuments(const ExePolicy& policy, const std::string_view raw_query,
    DocumentPredicate document_predicate) const {

    auto query = ParseQuery(std::execution::par, raw_query);

    std::sort(
        policy,
        query.minus_words.begin(),
        query.minus_words.end()
    );
    query.minus_words.erase(
        std::unique(policy, query.minus_words.begin(), query.minus_words.end()),
        query.minus_words.end()
    );

    std::sort(
        policy,
        query.plus_words.begin(),
        query.plus_words.end()
    );
    query.plus_words.erase(
        std::unique(policy, query.plus_words.begin(), query.plus_words.end()),
        query.plus_words.end()
    );

    auto matched_documents = FindAllDocuments(policy, query, document_predicate);

    std::sort(
        policy,
        matched_documents.begin(),
        matched_documents.end(),
        [](const Document& lhs, const Document& rhs) {
            return std::abs(lhs.relevance - rhs.relevance) < EPSILON
                ? lhs.rating > rhs.rating
                : lhs.relevance > rhs.relevance;
        }
    );

    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }

    return matched_documents;

}
template<typename ExePolicy>
std::vector<Document> SearchServer::FindTopDocuments(const ExePolicy& policy, const std::string_view raw_query, DocumentStatus status) const {
    return FindTopDocuments(policy, raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
        return document_status == status;});

}
template<typename ExePolicy>
std::vector<Document> SearchServer::FindTopDocuments(const ExePolicy& policy, const std::string_view raw_query) const {
    return FindTopDocuments(policy, raw_query, DocumentStatus::ACTUAL);
}


template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const Query& query,
    DocumentPredicate document_predicate) const {
    return FindAllDocuments(std::execution::seq, query, document_predicate);
}

template <typename ExePolicy, typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const ExePolicy& policy, const Query& query,
    DocumentPredicate document_predicate) const {

    ConcurrentMap<int, double> document_to_relevance(150);

    std::for_each(
        policy,
        query.plus_words.begin(),
        query.plus_words.end(),
        [&](const std::string_view& word) {
            if (word_to_id_freqs_.count(word) == 0) {
                return;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            std::for_each(
                policy,
                word_to_id_freqs_.at(word).begin(),
                word_to_id_freqs_.at(word).end(),
                [&](const auto& document) {
                    const auto& document_data = documents_.at(document.first);
                    if (document_predicate(document.first, document_data.status, document_data.rating)) {
                        document_to_relevance[document.first].ref_to_value += document.second * inverse_document_freq;
                    }
                }
            );
        }
    );
    std::for_each(
        policy,
        query.minus_words.begin(),
        query.minus_words.end(),
        [&](const std::string_view word) {
            if (word_to_id_freqs_.count(word) == 0) {
                return;
            }
            for (const auto [document_id, _] : word_to_id_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }
    );

    const auto builded = std::move(document_to_relevance.BuildOrdinaryMap());
    std::vector<Document> matched_documents;
    matched_documents.reserve(builded.size());
    std::mutex locker;

    std::for_each(
        policy,
        builded.begin(),
        builded.end(),
        [&](const auto& content) {
            Document* ptr;
            {
                std::lock_guard g(locker);
                ptr = &matched_documents.emplace_back();
            }
            *ptr = { content.first, content.second, documents_.at(content.first).rating };
        }
    );

    return matched_documents;
}