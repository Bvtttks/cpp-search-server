#include "remove_duplicates.h"
#include <iostream>
#include <map>
#include <set>
using namespace std;

void RemoveDuplicates(SearchServer& search_server) {
    set<int> id_to_remove;
    set<map<string, double>> freqs_to_id;
    for(const int id : search_server) {
        if (freqs_to_id.count(search_server.GetWordFrequencies(id))) {
            id_to_remove.insert(id);
            cout << "Found duplicate document id " << id << endl;
        } else {
            freqs_to_id.insert(search_server.GetWordFrequencies(id));
        }
    }
    
    for(int id : id_to_remove) {
        search_server.RemoveDocument(id);
    }
}
