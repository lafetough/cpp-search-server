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

void TestRelevanceOut() {
    const int doc_id = 42;
    const string content = "пушистый кот пушистый хвост"s;
    const vector<int> ratings = { 7, 2, 7 };

    const int doc_id2 = 53;
    const string content2 = "белый кот и модный ошейник"s;
    const vector<int> ratings2 = { 8, -3 };

    const int doc_id3 = 63;
    const string content3 = "ухоженный пёс выразительные глаза"s;
    const vector<int> ratings3 = { 5, -12, 2, 1 };


    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, ratings2);
        server.AddDocument(doc_id3, content3, DocumentStatus::ACTUAL, ratings3);

        auto doc = server.FindTopDocuments("пушистый ухоженный кот"s);

        double relevance = doc.front().relevance;
        for (const auto& a : doc) {
            ASSERT(a.relevance <= relevance);
            relevance = a.relevance;
        }
    }
}

void TestRatingDoc() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };

    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        ASSERT(server.FindTopDocuments("cat in the city"s).at(0).rating == (1 + 2 + 3) / 3);
    }
}

void TestCostomPredicat() {

    const int doc_id = 1;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };

    const int doc_id2 = 3;
    const string content2 = "fghjjdfg fasd"s;
    const vector<int> ratings2 = { 1, 2, 3 };

    {
        SearchServer server;
        server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
        server.AddDocument(doc_id2, content2, DocumentStatus::ACTUAL, ratings2);

        auto doc = server.FindTopDocuments("cat in the city"s, [](int document_id, DocumentStatus status, int rating) { return document_id == 1; });

        ASSERT_EQUAL(doc.size(), 1);
        ASSERT_EQUAL(doc[0].id, 1);

    }

}

void TestStatusFind() {

    const int doc_id1 = 42;
    const string content1 = "cat in the city"s;
    const vector<int> ratings1 = { 1, 2, 3 };
    DocumentStatus status1 = DocumentStatus::ACTUAL;

    const int doc_id2 = 52;
    const string content2 = "the city"s;
    const vector<int> ratings2 = { 1, 2, 3 };
    DocumentStatus status2 = DocumentStatus::BANNED;

    const int doc_id3 = 62;
    const string content3 = "cat in the house"s;
    const vector<int> ratings3 = { 1, 2, 3 };
    DocumentStatus status3 = DocumentStatus::ACTUAL;

    {
        SearchServer server;
        server.AddDocument(doc_id1, content1, status1, ratings1);
        server.AddDocument(doc_id2, content2, status2, ratings2);
        server.AddDocument(doc_id3, content3, status3, ratings3);

        auto doc = server.FindTopDocuments("cat in the city"s, DocumentStatus::ACTUAL);

        ASSERT_EQUAL(doc.size(), 2);
        ASSERT_EQUAL(doc[0].id, 42);
        ASSERT_EQUAL(doc[1].id, 62);
    }

}

void TestRelevCompute() {

    SearchServer server;
    server.SetStopWords("и в на"s);
    server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, { 8, -3 });
    server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });
    //server.AddDocument(3, "ухоженный скворец евгений"s, DocumentStatus::BANNED, { 9 });

    auto doc = server.FindTopDocuments("пушистый ухоженный кот");

    vector<double> relevance_actual;

    for (const auto& a : doc) {
        relevance_actual.push_back(a.relevance);
    }
    vector<double> relevance_result = {
        (log(server.GetDocumentCount() * 1.0 / 1) * (2.0 / 4) + log(server.GetDocumentCount() * 1.0 / 2) * (1.0 / 4)),
        (log(server.GetDocumentCount() * 1.0 / 1) * (1.0 / 4)) ,
        (log(server.GetDocumentCount() * 1.0 / 2) * (1.0 / 4))
    };

    ASSERT(relevance_actual == relevance_result);
}

// Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() {
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(MatchTestDoc);
    RUN_TEST(TestRelevanceOut);
    RUN_TEST(TestRatingDoc);
    RUN_TEST(TestCostomPredicat);
    RUN_TEST(TestStatusFind);
    RUN_TEST(TestRelevCompute);
}


int main() {
    TestSearchServer();
    cout << "Search server testing finished"s << endl;
}