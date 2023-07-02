#pragma once
#include "string_processing.h"
#include "concurrent_map.h"
#include "document.h"
#include <string_view>
#include <execution>
#include <algorithm>
#include <string>
#include <vector>
#include <deque>
#include <cmath>
#include <set>
#include <map>

const int MAX_RESULT_DOCUMENT_COUNT = 5;

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

class SearchServer {
public:
    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words);
    
    explicit SearchServer(const std::string_view& stop_words_text)
        : SearchServer(SplitIntoWords(stop_words_text))  // Invoke delegating constructor from string container
    {
    }
    
    explicit SearchServer(const std::string& stop_words_text)
        : SearchServer(SplitIntoWords(stop_words_text))  // Invoke delegating constructor from string container
    {
    }

    void AddDocument(int document_id, const std::string_view document, DocumentStatus status,
                     const std::vector<int>& ratings);
    
    void RemoveDocument(int document_id);
    
    template <typename ExecutionPolicy>
    void RemoveDocument(ExecutionPolicy&& policy, int document_id) {
        if (!all_ids_.count(document_id))
            return;
        std::vector<std::string_view> words;
        words.reserve(id_to_w_freqs_.at(document_id).size());

        std::transform(id_to_w_freqs_.at(document_id).begin(), id_to_w_freqs_.at(document_id).end(),
                       std::back_inserter(words),
                       [](const auto& pair) {
                           return &(pair.first);
                       });

        std::for_each(policy, words.begin(), words.end(), [&](const std::string_view str) {
            word_to_document_freqs_.erase(str);
        });

        id_to_w_freqs_.erase(document_id);
        documents_.erase(document_id);
        all_ids_.erase(document_id);
    }
    
    std::vector<Document> FindTopDocuments(const std::string_view raw_query, DocumentStatus status) const;

    std::vector<Document> FindTopDocuments(const std::string_view raw_query) const;

    template <typename ExecutionPolicy>
    inline std::vector<Document> FindTopDocuments(ExecutionPolicy&& policy, std::string_view raw_query, DocumentStatus status = DocumentStatus::ACTUAL) const {
        return SearchServer::FindTopDocuments(policy, raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
            return document_status == status;
        });
    }
    
    template <typename ExecutionPolicy, typename DocumentPredicate>
    inline std::vector<Document> FindTopDocuments(ExecutionPolicy&& policy, std::string_view raw_query, DocumentPredicate document_predicate) const {
        if(raw_query.empty()) {
            throw std::invalid_argument(" "s);
        }
        if (std::is_same_v<std::decay_t<ExecutionPolicy>, std::execution::sequenced_policy>) {
            return FindTopDocuments(raw_query, document_predicate);
        }
            bool need_sorting = true;
            const Query query = ParseQuery(raw_query, need_sorting);
            auto matched_documents = FindAllDocuments(policy, query, document_predicate);
            std::sort(std::execution::par, matched_documents.begin(), matched_documents.end(),
                [](const Document& lhs, const Document& rhs) {
                    if (std::abs(lhs.relevance - rhs.relevance) < 1e-6) {
                        return lhs.rating > rhs.rating;
                    }
                    else {
                        return lhs.relevance > rhs.relevance;
                    }
                });
            if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
                matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
            }
            return matched_documents;
    }
    
    template <typename DocumentPredicate>
    std::vector<Document> FindTopDocuments(const std::string_view raw_query, DocumentPredicate document_predicate) const {
        auto query = ParseQuery(raw_query);
        
        std::sort(query.minus_words.begin(), query.minus_words.end());
        auto last = std::unique(query.minus_words.begin(), query.minus_words.end());
        query.minus_words.erase(last, query.minus_words.end());
        
        std::sort(query.plus_words.begin(), query.plus_words.end());
        last = std::unique(query.plus_words.begin(), query.plus_words.end());
        query.plus_words.erase(last, query.plus_words.end());
        
        auto matched_documents = FindAllDocuments(query, document_predicate);
        
        sort(matched_documents.begin(), matched_documents.end(),
             [](const Document& lhs, const Document& rhs) {
                 if (std::abs(lhs.relevance - rhs.relevance) < 1e-6) {
                     return lhs.rating > rhs.rating;
                 } else {
                     return lhs.relevance > rhs.relevance;
                 }
             });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }

    int GetDocumentCount() const;

    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const std::string_view raw_query, int document_id) const;
    
    template <typename ExecutionPolicy>
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(const ExecutionPolicy policy, const std::string_view raw_query, int document_id) const {
        if (std::is_same_v<ExecutionPolicy, std::execution::sequenced_policy>) {
            return MatchDocument(raw_query, document_id);
        }
        
        const auto query = ParseQuery(raw_query);
        std::vector<std::string_view> matched_words;
        
        //Проверка на минус-слова. Если есть хоть одно- возврат пустого вектора
        if(std::any_of(policy, query.minus_words.begin(), query.minus_words.end(), [document_id, this] (std::string_view word){
            return word_to_document_freqs_.count(word) && word_to_document_freqs_.at(word).count(document_id);
        }))
            return {matched_words, documents_.at(document_id).status};
        
        matched_words.resize(query.plus_words.size());
        auto end_of_matched_words = std::copy_if(policy, query.plus_words.begin(), query.plus_words.end(), matched_words.begin(), [document_id, this] (std::string_view word){
            return word_to_document_freqs_.count(word) && word_to_document_freqs_.at(word).count(document_id);
        });
        
        std::sort(policy, matched_words.begin(), end_of_matched_words);
        auto iter = std::unique(policy, matched_words.begin(), end_of_matched_words);
        matched_words.erase(iter, matched_words.end());
        
        return {matched_words, documents_.at(document_id).status};
    }
    
    auto begin() {
        return all_ids_.begin();
    }
    
    auto end() {
        return all_ids_.end();
    }
    
    const std::map<std::string_view, double>& GetWordFrequencies(int document_id) const;

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };
    std::deque<std::string> storage;
    const std::set<std::string, std::less<>> stop_words_;
    std::map<std::string_view, std::map<int, double>> word_to_document_freqs_;
    std::map<int, DocumentData> documents_;
    std::set<int> all_ids_;
    std::map<int, std::map<std::string_view, double>> id_to_w_freqs_;
        
    bool IsStopWord(const std::string_view word) const;
    
    static bool IsValidWord(const std::string_view word);
    
    template <typename ExecutionPolicy>
    bool IsValidWord(ExecutionPolicy&& policy, const std::string_view word) {
        // A valid word must not contain special characters
        return none_of(policy, word.begin(), word.end(), [](char c) {
            return c >= '\0' && c < ' ';
        });
    }
    
    std::vector<std::string_view> SplitIntoWordsNoStop(const std::string_view text) const;

    static int ComputeAverageRating(const std::vector<int>& ratings);

    struct QueryWord {
        std::string_view data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(const std::string_view text) const;

    struct Query {
        std::vector<std::string_view> plus_words;
        std::vector<std::string_view> minus_words;
    };

    Query ParseQuery(const std::string_view text, bool is_sort = 0) const;
    
    /*
     template <typename ExecutionPolicy>
     SearchServer::Query ParseQuery(ExecutionPolicy&& policy, const std::string& text) const {
         Query result;
         for (const std::string& word : SplitIntoWords(policy, text)) {
             const auto query_word = ParseQueryWord(word);
             if (!query_word.is_stop) {
                 if (query_word.is_minus) {
                     result.minus_words.push_back(query_word.data);
                 } else {
                     result.plus_words.push_back(query_word.data);
                 }
             }
         }
         return result;
     }
     */
    
    // Existence required
    double ComputeWordInverseDocumentFreq(const std::string_view word) const {
        return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
    }

    template <typename DocumentPredicate>
    std::vector<Document> FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const;
    
    template <typename ExecutionPolicy, typename DocumentPredicate>
    inline std::vector<Document> FindAllDocuments(ExecutionPolicy&& policy, const Query& query, DocumentPredicate document_predicate) const;
};

template <typename StringContainer>
SearchServer::SearchServer(const StringContainer& stop_words)
    : stop_words_(MakeUniqueNonEmptyStrings(stop_words))
{
    if (!std::all_of(stop_words_.begin(), stop_words_.end(), [this](const std::string_view word) {
            return IsValidWord(word);
        })) {
        throw std::invalid_argument("Some of stop words are invalid");
    }
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const {
    std::map<int, double> document_to_relevance; //мапа результата
    
    for (const std::string_view word : query.plus_words) { // проходим все плюс слова запроса
        if (word_to_document_freqs_.count(word) == 0) { // если ни в 1 доке не было такого слова
            continue;
        }
        const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
        for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
            const auto& document_data = documents_.at(document_id);
            if (document_predicate(document_id, document_data.status, document_data.rating)) {
                document_to_relevance[document_id] += term_freq * inverse_document_freq;
            }
        }
    }

    for (const std::string_view word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
            document_to_relevance.erase(document_id);
        }
    }

    std::vector<Document> matched_documents;
    for (const auto [document_id, relevance] : document_to_relevance) {
        matched_documents.push_back(
            {document_id, relevance, documents_.at(document_id).rating});
    }
    return matched_documents;
}

template <typename ExecutionPolicy, typename DocumentPredicate>
inline std::vector<Document> SearchServer::FindAllDocuments(ExecutionPolicy&& policy, const Query& query, DocumentPredicate document_predicate) const {
    ConcurrentMap<int, double> document_to_relevance(100);
    std::for_each(policy, query.plus_words.begin(), query.plus_words.end(), [&](const auto& word) {
        if (word_to_document_freqs_.count(word) != 0) {
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                const auto& document_data = documents_.at(document_id);
                if (document_predicate(document_id, document_data.status, document_data.rating)) {
                    document_to_relevance[document_id].ref_to_value += term_freq * inverse_document_freq;
                }
            }
        }
    });
    std::for_each(policy, query.minus_words.begin(), query.minus_words.end(), [&](const auto& word) {
        if (word_to_document_freqs_.count(word) != 0) {
            for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }
    });
    std::vector<Document> matched_documents;
    for (const auto [document_id, relevance] : document_to_relevance.BuildOrdinaryMap()) {
        matched_documents.push_back({document_id, relevance, documents_.at(document_id).rating});
    }
    return matched_documents;
}