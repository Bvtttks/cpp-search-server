#include "request_queue.h"
#include <string>
#include <deque>
using namespace std;

template <typename DocumentPredicate>
vector<Document> RequestQueue::AddFindRequest(const string& raw_query, DocumentPredicate document_predicate) {
    return Request_Processing_(search_server_.FindTopDocuments(raw_query, document_predicate));
}

vector<Document> RequestQueue::AddFindRequest(const string& raw_query, DocumentStatus status) {
    return Request_Processing_(search_server_.FindTopDocuments(raw_query, status));
}
vector<Document> RequestQueue::AddFindRequest(const string& raw_query) {
    return Request_Processing_(search_server_.FindTopDocuments(raw_query));
}
int RequestQueue::GetNoResultRequests() const {
    return NoResultRequests;
}

vector<Document> RequestQueue::Request_Processing_(vector<Document> result) {
    if (requests_.size() >= min_in_day_) {
        if (requests_.front().is_empty == 1)
            --NoResultRequests;
        requests_.pop_front();
    }
    QueryResult temp;
    if (result.empty()) {
        temp.is_empty = 1;
        ++NoResultRequests;
    }
    requests_.push_back(temp);
    return result;
}
