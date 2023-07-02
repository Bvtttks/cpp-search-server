#include "read_input_functions.h"
#include <iostream>
#include <vector>

template<typename It>
class IteratorRange {
public:
    It begin() const {
        return begin_;
    }
    
    It end() const {
        return end_;
    }
    
    void SetBegin(It iter){
        begin_ = iter;
    }
    
    void SetEnd(It iter){
        end_ = iter;
    }
    
    int size() const {
        return distance(begin_, end_);
    }
private:
    It begin_;
    It end_;
};

template<typename It>
std::ostream& operator<< (std::ostream &out, const IteratorRange<It> &range)
{
    for(It i = range.begin(); i != range.end(); ++i){
        auto temp = *i;
        out << temp;
    }
    return out;
}