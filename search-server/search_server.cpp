#include "search_server.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <ostream>
#include <numeric>
#include <string_view>


SearchServer::SearchServer(const std::string& stop_words_text)
    : SearchServer(
        SplitIntoWords(stop_words_text))  // Invoke delegating constructor from string container
{
}

SearchServer::SearchServer(std::string_view stop_words_text)
    :SearchServer(SplitIntoWords(stop_words_text))
{
}

void SearchServer::AddDocument(int document_id, const std::string_view document, DocumentStatus status,
    const std::vector<int>& ratings) {

    using namespace std;

    if ((document_id < 0) || (documents_.find(document_id) != documents_.end())) {
        throw invalid_argument("Invalid document_id"s);
    }

    documents_strings_.push_back(static_cast<std::string>((document)));

    const auto words = SplitIntoWordsNoStop(documents_strings_.back());

    const double inv_word_count = 1.0 / words.size();
    for (const string_view& word : words) {
        word_to_id_freqs_[word][document_id] += inv_word_count;
        id_to_words_freq_[document_id][word] += inv_word_count;

    }
    documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
    document_ids_.insert(document_id);
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query, DocumentStatus status) const {
    return FindTopDocuments(
        raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
            return document_status == status;
        });
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query) const {
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}



const std::map<std::string_view, double>& SearchServer::GetWordFrequencies(int document_id) const {

    static const std::map<std::string_view, double> empty_map;

    if (!id_to_words_freq_.count(document_id)) {
        return empty_map;
    }

    return id_to_words_freq_.at(document_id);
}

void SearchServer::RemoveDocument(int document_id) {

    if (!documents_.count(document_id)) {
        return;
    }
    documents_.erase(document_id);
    document_ids_.erase(document_id);

    std::for_each(
        std::execution::seq,
        id_to_words_freq_[document_id].begin(),
        id_to_words_freq_[document_id].end(),
        [&](const std::pair<std::string_view, double>& word) {
            word_to_id_freqs_[word.first].erase(document_id);
        }
    );

    id_to_words_freq_.erase(document_id);

}

void SearchServer::RemoveDocument(const std::execution::sequenced_policy, int document_id) {
    RemoveDocument(document_id);
}

void SearchServer::RemoveDocument(const std::execution::parallel_policy, int document_id) {

    if (!document_ids_.count(document_id)) {
        return;
    }
    std::vector<std::pair<std::string_view, double>> temp;

    temp.reserve(id_to_words_freq_[document_id].size());
    temp.insert(temp.begin(), id_to_words_freq_[document_id].begin(), id_to_words_freq_[document_id].end());

    documents_.erase(documents_.find(document_id));
    document_ids_.erase(document_ids_.find(document_id));
    id_to_words_freq_.erase(id_to_words_freq_.find(document_id));


    std::for_each(
        std::execution::par_unseq,
        temp.begin(),
        temp.end(),
        [&](const std::pair<std::string_view, double>& word) {
            word_to_id_freqs_[word.first].erase(document_id);
        });
}

int SearchServer::GetDocumentCount() const {
    return static_cast<int>(documents_.size());
}


std::set<int>::const_iterator SearchServer::begin() const {
    return document_ids_.begin();
}

std::set<int>::const_iterator SearchServer::end() const {
    return document_ids_.end();
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::string_view raw_query,
    int document_id) const {
    const auto query = ParseQuery(raw_query);

    bool is_any_minus_words = false;
    for (const std::string_view word : query.minus_words) {
        if (word_to_id_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_id_freqs_.at(word).count(document_id)) {
            is_any_minus_words = true;
        }
    }
    if (is_any_minus_words) {
        return { {}, documents_.at(document_id).status };
    }

    std::vector<std::string_view> matched_words;
    for (const std::string_view word : query.plus_words) {
        if (word_to_id_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_id_freqs_.at(word).count(document_id)) {
            matched_words.push_back(word_to_id_freqs_.find(word)->first);
        }
    }
    
   
    
    return { matched_words, documents_.at(document_id).status };
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::execution::sequenced_policy, const std::string_view raw_query,int document_id) const {

    return MatchDocument(raw_query, document_id);
}

std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(const std::execution::parallel_policy, const std::string_view& raw_query,
    int document_id) const {

    const Query parsed_query = ParseQuery(std::execution::par, raw_query);

    bool is_any_minus_words = std::any_of(
        std::execution::par_unseq,
        parsed_query.minus_words.begin(),
        parsed_query.minus_words.end(),
        [&](const std::string_view& word) {

            const auto it = word_to_id_freqs_.find(word);

            return (it != word_to_id_freqs_.end() &&
                it->second.find(document_id) != it->second.end());
        }
    );
    
    if (is_any_minus_words) {
        return { {}, documents_.at(document_id).status};
    }

    std::vector<std::string_view> matched_documents(parsed_query.plus_words.size());

    std::transform(
        std::execution::par_unseq,
        parsed_query.plus_words.begin(),
        parsed_query.plus_words.end(),
        matched_documents.begin(),
        [&](const std::string_view& word) {

            using namespace std::string_view_literals;

            const auto it = word_to_id_freqs_.find(word); 

            return (it != word_to_id_freqs_.end() &&
                it->second.find(document_id) != it->second.end())
                ? it->first
                : ""sv;
        }
    );

    std::sort(
        std::execution::par_unseq,
        matched_documents.begin(),
        matched_documents.end(),
        [](const std::string_view lhs, const std::string_view rhs) {
            return rhs < lhs;
        }
    );

    matched_documents.erase(
        std::unique(
            std::execution::par_unseq,
            matched_documents.begin(),
            matched_documents.end()
        ) - 1,
        matched_documents.end()
    );

    return { matched_documents, documents_.at(document_id).status };

}


bool SearchServer::IsStopWord(const std::string_view word) const {
    return stop_words_.find(word) != stop_words_.end();
}

bool SearchServer::IsValidWord(const std::string_view word) {
    // A valid word must not contain special characters
    return std::none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
        });
}

std::vector<std::string_view> SearchServer::SplitIntoWordsNoStop(const std::string_view text) const {

    using namespace std;

    const auto splitted = SplitIntoWords(text);

    std::vector<std::string_view> words;
    words.reserve(splitted.size());
    for (const std::string_view& word : splitted) {
        if (!IsValidWord(word)) {
            throw std::invalid_argument("Word "s + std::string(word) + " is invalid"s);
        }
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}

int SearchServer::ComputeAverageRating(const std::vector<int>& ratings) {
    if (ratings.empty()) {
        return 0;
    }
    int rating_sum = std::accumulate(ratings.begin(), ratings.end(), 0);

    return rating_sum / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(const std::string_view text) const {

    using namespace std;

    if (text.empty()) {
        throw std::invalid_argument("Query word is empty"s);
    }
    std::string_view word;
    bool is_minus = false;
    if (text[0] == '-') {
        is_minus = true;
        word = text.substr(1);
    }
    else {
        word = text;
    }
    if (word.empty() || word[0] == '-' || !IsValidWord(word)) {
        throw std::invalid_argument("Query word "s + std::string(text) + " is invalid");
    }

    return { word, is_minus, IsStopWord(word) };
}

SearchServer::Query SearchServer::ParseQuery(const std::string_view text) const {
    SearchServer::Query result;
    
    for (const std::string_view word : SplitIntoWords(text)) {
        const auto query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                result.minus_words.push_back(std::string(query_word.data));
            }
            else {
                result.plus_words.push_back(std::string(query_word.data));
            }
        }
    }

    std::sort(
        std::execution::seq,
        result.minus_words.begin(),
        result.minus_words.end()
    );
    result.minus_words.erase(
        std::unique(std::execution::seq, result.minus_words.begin(), result.minus_words.end()),
        result.minus_words.end()
    );

    std::sort(
        std::execution::seq,
        result.plus_words.begin(),
        result.plus_words.end()
    );
    result.plus_words.erase(
        std::unique(std::execution::seq, result.plus_words.begin(), result.plus_words.end()),
        result.plus_words.end()
    );

    return result;

}

SearchServer::Query SearchServer::ParseQuery(const std::execution::parallel_policy ,const std::string_view& text) const {

    SearchServer::Query result;

    const auto splitted = SplitIntoWords(text);

    result.minus_words.reserve(splitted.size());
    result.plus_words.reserve(splitted.size());


    for (const std::string_view& word : splitted) {
        const auto query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            query_word.is_minus
                ? result.minus_words.push_back(static_cast<std::string>((query_word.data)))
                : result.plus_words.push_back(static_cast<std::string>((query_word.data)));
        }
    }
    return result;

}


double SearchServer::ComputeWordInverseDocumentFreq(const std::string_view word) const {
    return log(GetDocumentCount() * 1.0 / word_to_id_freqs_.at(word).size());
}