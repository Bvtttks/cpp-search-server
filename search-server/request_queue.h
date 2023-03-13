#pragma once
#include "search_server.h"
#include <string>
#include <deque>

class RequestQueue {
public:
    explicit RequestQueue(const SearchServer& search_server) : search_server_(search_server) {
    }
    // сделаем "обёртки" для всех методов поиска, чтобы сохранять результаты для нашей статистики
    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {
        return Request_Processing_(search_server_.FindTopDocuments(raw_query, document_predicate));
    }
    
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status);
    
    std::vector<Document> AddFindRequest(const std::string& raw_query);
    int GetNoResultRequests() const;
private:
    struct QueryResult {
        QueryResult(){
            is_empty = 0;
        }
        bool is_empty;
    };
    const SearchServer& search_server_;
    std::deque<QueryResult> requests_;
    const static int min_in_day_ = 1440;
    int NoResultRequests = 0;
    
    std::vector<Document> Request_Processing_(std::vector<Document> result);
};
