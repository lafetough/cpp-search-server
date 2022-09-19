#include <algorithm>
#include <iostream>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <map>
#include <numeric>
#include <cmath>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result = 0;
    cin >> result;
    ReadLine();
    return result;
}

vector<string> SplitIntoWords(const string& text) {
    vector<string> words;
    string word;
    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        }
        else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }

    return words;
}

struct Document {
    int id;
    double relevance;
};

class SearchServer {
public:
    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }

    void AddDocument(int document_id, const string& document) {
        ++document_count_;

        const vector<string> words = SplitIntoWordsNoStop(document);

        map<string, double> word_to_document_id_to_count;

        for (string s : words) {
            ++word_to_document_id_to_count[s];
        }
        for (string s : words) {
            double d = word_to_document_id_to_count[s] / words.size();
            word_to_document_freqs_[s][document_id] = d;
        }

    }

    vector<Document> FindTopDocuments(const string& raw_query) const {
        const set<string> query_words = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query_words);

        sort(matched_documents.begin(), matched_documents.end(),
            [](const Document& lhs, const Document& rhs) {
                return lhs.relevance > rhs.relevance;
            });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }

private:

    int document_count_ = 0;

    map<string, map<int, double>> word_to_document_freqs_;

    set<string> stop_words_;

    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }


    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

    set<string> ParseQuery(const string& text) const {
        set<string> query_words;
        for (const string& word : SplitIntoWordsNoStop(text)) {
            query_words.insert(word);
        }
        return query_words;
    }

    vector<Document> FindAllDocuments(const set<string>& query_words) const {
        vector<Document> matched_documents;
        map<int, double > relevance = MatchDocument(word_to_document_freqs_, query_words);

        for (auto a : relevance) {
            if (a.second > 0) {
                matched_documents.push_back({ a.first, a.second });
            }
        }



        return matched_documents;
    }

    map<int, double> MatchDocument(const  map<string, map<int, double>>& content, const set<string>& query_words) const {
        if (query_words.empty()) {
            return { {} };
        }
        set<string> stop_words;

        for (string s : query_words) {
            if (s[0] == '-') {
                s.erase(remove(s.begin(), s.end(), '-'), s.end());

                stop_words.insert(s);
            }
        }

        map<string, map<int, double>> copy_to_manipulate = content;

        map<string, double> IDF;

        set<int> to_del;

        for (string s : stop_words) {
            if (copy_to_manipulate.count(s) > 0) {
                for (const auto& a : copy_to_manipulate.at(s)) {
                    to_del.insert(a.first);
                }
            }
        }

        for (string s : query_words) {
            if (copy_to_manipulate.count(s) > 0) {
                int num_of_doc_with_word;

                num_of_doc_with_word = static_cast<int>(copy_to_manipulate.at(s).size());

                IDF[s] = static_cast<double>(log(static_cast<double>(document_count_) / static_cast<double>(num_of_doc_with_word)));


            }
        }

        vector<double> to_calc;
        map<int, double> output;



        int max_id = 0;

        for (const auto& divide_map : copy_to_manipulate) {
            for (const auto& ID_and_Rel : divide_map.second) {
                if (ID_and_Rel.first > max_id) {
                    max_id = ID_and_Rel.first;

                }
            }
        }


        for (int i = 0; i <= max_id; ++i) {
            for (const auto& divide_map : copy_to_manipulate) {
                if (query_words.count(divide_map.first) > 0 && divide_map.second.count(i) > 0) {
                    double IDF_TF_of_word = divide_map.second.at(i) * IDF.at(divide_map.first);

                    to_calc.push_back(IDF_TF_of_word);
                }
            }

            double relevance = 0;

            for (double every_IDF_TF : to_calc) {
                relevance += every_IDF_TF;
            }

            output[i] = relevance;

            to_calc.clear();

        }


        for (int i : to_del) {
            output.erase(i);
        }

        return output;
    }
};

SearchServer CreateSearchServer() {
    SearchServer search_server;
    search_server.SetStopWords(ReadLine());

    const int document_count = ReadLineWithNumber();
    for (int document_id = 0; document_id < document_count; ++document_id) {
        search_server.AddDocument(document_id, ReadLine());
    }

    return search_server;
}

int main() {
    const SearchServer search_server = CreateSearchServer();

    const string query = ReadLine();
    for (const auto& [document_id, relevance] : search_server.FindTopDocuments(query)) {
        cout << "{ document_id = "s << document_id << ", "
            << "relevance = "s << relevance << " }"s << endl;
    }
}