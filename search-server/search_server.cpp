#include "search_server.h"
#include "process_queries.h"
#include <execution>
#include <string>
#include <vector>
#include <set>
#include <map>
using namespace std;

void SearchServer::AddDocument(int document_id, const string_view document, DocumentStatus status, const vector<int>& ratings) {
    if ((document_id < 0) || (documents_.count(document_id) > 0))
        throw invalid_argument("Invalid document_id"s);
    storage.emplace_back(document);
    const auto words = SplitIntoWordsNoStop(storage.back());
    
    const double inv_word_count = 1.0 / words.size();
    for (const string_view word : words)
        word_to_document_freqs_[word][document_id] += inv_word_count;
    for (const string_view word : words) {
        SearchServer::id_to_w_freqs_[document_id].insert({word, word_to_document_freqs_[word][document_id]});
    }
    documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status});
    all_ids_.insert(document_id);
}

void SearchServer::RemoveDocument(int document_id) {
    if (!all_ids_.count(document_id))
        return;
    set<string_view> this_doc_words;
    for (auto [key, value] : GetWordFrequencies(document_id))
        word_to_document_freqs_.at(key).erase(document_id);
    id_to_w_freqs_.erase(document_id);
    documents_.erase(document_id);
    all_ids_.erase(document_id);
}

vector<Document> SearchServer::FindTopDocuments(const string_view raw_query, DocumentStatus status) const {
    return FindTopDocuments(raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
            return document_status == status;
        });
}

vector<Document> SearchServer::FindTopDocuments(const string_view raw_query) const {
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

int SearchServer::GetDocumentCount() const {
    return static_cast<int>(documents_.size());
}

tuple<vector<string_view>, DocumentStatus> SearchServer::MatchDocument(const string_view raw_query, int document_id) const {
    auto query = ParseQuery(raw_query);
    vector<string_view> matched_words;
    
    std::sort(query.minus_words.begin(), query.minus_words.end());
    auto iter1 = std::unique(query.minus_words.begin(), query.minus_words.end());
    query.minus_words.erase(iter1, query.minus_words.end());

    std::sort(query.plus_words.begin(), query.plus_words.end());
    auto iter2 = std::unique(query.plus_words.begin(), query.plus_words.end());
    query.plus_words.erase(iter2, query.plus_words.end());
    
    for (const string_view word : query.minus_words) {
        if (word_to_document_freqs_.count(word) && word_to_document_freqs_.at(word).count(document_id))
            return {matched_words, documents_.at(document_id).status};
    }
    
    for (string_view word : query.plus_words) {
        if (word_to_document_freqs_.count(word) && word_to_document_freqs_.at(word).count(document_id))
            matched_words.push_back(word);
    }
    
    return {matched_words, documents_.at(document_id).status};
}

const map<string_view, double>& SearchServer::GetWordFrequencies(int document_id) const {
    static map<string_view, double> empty_map;
    if(all_ids_.count(document_id))
        return SearchServer::id_to_w_freqs_.at(document_id);
    return empty_map;
}

bool SearchServer::IsStopWord(const string_view word) const {
    return stop_words_.count(word) > 0;
}

/*
bool SearchServer::IsValidWord(const string& word) {
    // A valid word must not contain special characters
    return none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
    });
}
*/

bool SearchServer::IsValidWord(const string_view word) {
    // A valid word must not contain special characters
    return none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
    });
}

vector<string_view> SearchServer::SplitIntoWordsNoStop(const string_view text) const {
    vector<string_view> words;
    for (const string_view word : SplitIntoWords(text)) {
        if (!IsValidWord(word)) {
            throw invalid_argument("Word "s + string{word} + " is invalid"s);
        }
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}

int SearchServer::ComputeAverageRating(const vector<int>& ratings) {
    if (ratings.empty()) {
        return 0;
    }
    int rating_sum = 0;
    for (const int rating : ratings) {
        rating_sum += rating;
    }
    return rating_sum / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(const string_view text) const {
    if (text.empty()) {
        throw invalid_argument("Query word is empty"s);
    }
    string_view word = text;
    bool is_minus = false;
    if (word[0] == '-') {
        is_minus = true;
        word.remove_prefix(1);
    }
    if (word.empty() || word[0] == '-' || !IsValidWord(word)) {
        throw invalid_argument("Query word "s + string{text} + " is invalid");
    }

    return {word, is_minus, IsStopWord(word)};
}

SearchServer::Query SearchServer::ParseQuery(const string_view text, bool is_sort) const {
    Query result;
    for (const string_view word : SplitIntoWords(text)) {
        const auto query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            if (query_word.is_minus) {
                result.minus_words.push_back(query_word.data);
            } else {
                result.plus_words.push_back(query_word.data);
            }
        }
    }
    if (!is_sort) {
        return result;
    }
    std::sort(result.minus_words.begin(), result.minus_words.end());
    auto unique_minus_words = std::unique(result.minus_words.begin(), result.minus_words.end());
    result.minus_words.erase(unique_minus_words, result.minus_words.end());
     std::sort(result.plus_words.begin(), result.plus_words.end());
    auto unique_plus_words = std::unique(result.plus_words.begin(), result.plus_words.end());
    result.plus_words.erase(unique_plus_words, result.plus_words.end());
    return result;
}