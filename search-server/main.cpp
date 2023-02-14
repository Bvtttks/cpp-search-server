#include <algorithm>
#include <iostream>
#include <set>
#include <map>
#include <string>
#include <utility>
#include <vector>
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
        } else {
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
        const vector<string> words = SplitIntoWordsNoStop(document);
        const double weight = 1.0 / words.size();
        for(const string& str : words)
            word_to_document_freqs_[str][document_id] += weight;
        ++document_count_;
    }
    
    vector<Document> FindTopDocuments(const string& raw_query) const {
        const Query query_words = ParseQuery(raw_query);
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
    struct Query {
        set<string> plus;
        set<string> minus;
    };
    
    map<string, map<int, double>> word_to_document_freqs_;
    set<string> stop_words_;
    int document_count_ = 0;
    
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
    
    // Ручные тесты и тесты Практикума говорят о работоспособности минус-слов в данной версии программы,
    // из-за особенностей парсинга в минус словах всегда остаётся '-' нулевым символом, и по нему они очень легко определяются(стр. 108)
    Query ParseQuery(const string& text) const {
        Query query;
        for (string& word : SplitIntoWordsNoStop(text)) {
            if(word[0] == '-') {
                word.erase(0, 1);
                query.minus.insert(word);
            }
            else
                query.plus.insert(word);
        }
        return query;
    }
    
    vector<Document> FindAllDocuments(const Query& query_words) const {
        map<int, double> document_to_relevance; // id дока и его релевантность
        double IDF;
        for(const string& str : query_words.plus) { // цикл по плюс словам запроса
            if(word_to_document_freqs_.count(str)) { // если слово есть в документах
                IDF = log((1.0 * document_count_ / word_to_document_freqs_.at(str).size())); // подсчёт IDF +слова запроса
                    for(const auto& [key, TF]: word_to_document_freqs_.at(str)) // смотрим TF этого слова для каждого дока
                        document_to_relevance[key] += IDF * TF; // записываем релевантность этого дока
            }
        }
        for(string str : query_words.minus) {
            if (word_to_document_freqs_.count(str)) {
                for(const auto& [ID, relevance]: word_to_document_freqs_.at(str))
                    document_to_relevance.erase(ID);
            }
        }
        vector<Document> matched_documents;
        for(auto element : document_to_relevance)
            matched_documents.push_back({element.first, element.second});
        return matched_documents;
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
