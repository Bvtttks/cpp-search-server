#include "process_queries.h"
#include "search_server.h"
#include <execution>
#include <iostream>
#include <string>
#include <vector>
using namespace std;

void PrintDocument(const Document& document) {
    cout << "{ "s
         << "document_id = "s << document.id << ", "s
         << "relevance = "s << document.relevance << ", "s
         << "rating = "s << document.rating << " }"s << endl;
}

void Test() {
    const auto check_search = [](SearchServer search_server) {
        // добавляем документ, содержащий стоп-слова
        search_server.AddDocument(0, "кот в мешке", DocumentStatus::ACTUAL, {1, 2, 3});

        if (const auto docs = search_server.FindTopDocuments("в"); !docs.empty()) {
            std::cout << "стоп слова не должны быть найдены" << std::endl;
        }

        if (const auto docs = search_server.FindTopDocuments("кот"); docs.empty()) {
            std::cout << "не стоп-слово, присутствующее в документе, должно быть найдено"
                      << std::endl;
        }
    };

    std::string stop_words = "и в на";

    check_search(SearchServer{stop_words});
    check_search(SearchServer{vector<string>{"и", "в", "на"}});
    check_search(SearchServer{set<string>{"и", "в", "на"}});
}

int main() {
    Test();
    return 0;
}