#include "remove_duplicates.h"
#include <iostream>
#include <map>
#include <set>
using namespace std;

// заводим set, где лежат set<string>- set<set<string>>
// когда видим новый документ, формируем из его слов set<string>.
// если такого в главном сете не нашлось, добавляем, если нашлось- удаляем документ

void RemoveDuplicates(SearchServer& search_server) {
    set<int> id_to_remove;
    set<set<string>> all_words;
    for(const int id : search_server) {
        set<string> this_doc_words;
        for (auto [key, value] : search_server.GetWordFrequencies(id))
            this_doc_words.insert(key);

        if (all_words.count(this_doc_words)) {
            id_to_remove.insert(id);
            cout << "Found duplicate document id " << id << endl;
        } else {
            all_words.insert(this_doc_words);
        }
    }

    for(int id : id_to_remove) {
        search_server.RemoveDocument(id);
    }
}
