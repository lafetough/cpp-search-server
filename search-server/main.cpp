#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "search_server.h"

using namespace std;

void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };
    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        const auto found_docs = server.FindTopDocuments("in"s);
        ASSERT_EQUAL(found_docs.size(), 1u);
        const Document& doc0 = found_docs[0];
        ASSERT_EQUAL(doc0.id, doc_id);
    }

    {
        SearchServer server;
        server.SetStopWords("in the"s);
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT_HINT(server.FindTopDocuments("in"s).empty(),
            "Stop words must be excluded from documents"s);
    }
}

void MatchTestDoc() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };

    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        tuple<vector<string>, DocumentStatus> out = { {"cat"s, "city"s, "in"s}, DocumentStatus::ACTUAL };
        ASSERT(server.MatchDocument("cat in city "s, doc_id) == out);
        out = { {}, DocumentStatus::ACTUAL };
        ASSERT(server.MatchDocument("-cat in city"s, doc_id) == out);

    }
}

void TestRelevOut() {
    const int doc_id = 42;
    const string content = "пушистый кот пушистый хвост"s;
    const vector<int> ratings = { 7, 2, 7 };

    const int doc_id2 = 53;
    const string content2 = "белый кот и модный ошейник"s;
    const vector<int> ratings2 = { 8, -3 };


    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, ratings2);

        auto doc = server.FindTopDocuments("пушистый ухоженный кот"s);

        sort(doc.begin(), doc.end(),
            [](const Document& lhs, const Document& rhs) {
                if (abs(lhs.relevance - rhs.relevance) < 1e-6) {
                    return lhs.rating > rhs.rating;
                }
                else {
                    return lhs.relevance > rhs.relevance;
                }
            });
        vector<double> d_first;

        for (const auto& a : server.FindTopDocuments("пушистый ухоженный кот"s)) {
            d_first.push_back(a.relevance);
        }

        vector<double> d_second;

        for (const auto& a : doc) {
            d_second.push_back(a.relevance);
        }

        ASSERT(d_first == d_second);

    }
}

void TestRatingDoc() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };

    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT(server.FindTopDocuments("cat in the city"s).at(0).rating == 2);
    }
}

void TestCostomPredicat() {

    const int doc_id = 1;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };

    const int doc_id2 = 1;
    const string content2 = "fghjjdfg fasd"s;
    const vector<int> ratings2 = { 1, 2, 3 };

    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, ratings2);

        ASSERT(server.FindTopDocuments("cat in the city"s, [](int document_id, DocumentStatus status, int rating) { return document_id == 1; }).size() == 1);
    }

}

void TestStatusFind() {

    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };

    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT(server.FindTopDocuments("cat in the city"s, DocumentStatus::ACTUAL).size() == 1);
    }

}

void TestRelevCompute() {

    SearchServer server;
    server.SetStopWords("и в на"s);
    server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, { 8, -3 });
    server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });
    server.AddDocument(3, "ухоженный скворец евгений"s, DocumentStatus::BANNED, { 9 });

    auto doc = server.FindTopDocuments("пушистый ухоженный кот");

    vector<double> d_actual;

    for (const auto a : doc) {

        d_actual.push_back(a.relevance);

    }

    vector<double> d_expected = { 0.86643397569993164, 0.17328679513998632, 0.17328679513998632 };

    ASSERT(d_actual == d_expected);
}

// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() {
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(MatchTestDoc);
    RUN_TEST(TestRelevOut);
    RUN_TEST(TestRatingDoc);
    RUN_TEST(TestCostomPredicat);
    RUN_TEST(TestStatusFind);
    RUN_TEST(TestRelevCompute);
    // Не забудьте вызывать остальные тесты здесь
}

// --------- Окончание модульных тестов поисковой системы -----------

int main() {
    TestSearchServer();
    // Если вы видите эту строку, значит все тесты прошли успешно
    cout << "Search server testing finished"s << endl;
}