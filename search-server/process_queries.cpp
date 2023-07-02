#include "process_queries.h"
#include <execution>

using namespace std;

std::vector<Document> ProcessQueriesJoined(const SearchServer& search_server, const std::vector<std::string>& queries) {
    std::vector<Document> all_search_results;
    for(auto vec : ProcessQueries(search_server, queries))
        all_search_results.insert(all_search_results.end(), vec.begin(), vec.end());
    return all_search_results;
}

vector<vector<Document>> ProcessQueries(const SearchServer& search_server, const vector<string>& queries) {
    vector<vector<Document>> search_result(queries.size());
    
    transform(std::execution::par, queries.begin(), queries.end(), search_result.begin(),
              [&search_server](const string& query) {return search_server.FindTopDocuments(query);});
    
    return search_result;
}