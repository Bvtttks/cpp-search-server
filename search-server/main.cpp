// Решите загадку: Сколько чисел от 1 до 1000 содержат как минимум одну цифру 3?
// Напишите ответ здесь:
#include <string>
#include <iostream>

using namespace std;

int main() {
    int count = 0;
    string line;
    for(int i = 2; i < 994; ++i){
        line = to_string(i);
        for(char c : line) {
            if (c == '3')
                ++count;
        }
    }
    cout << count << endl;
    return 0;
}

// Закомитьте изменения и отправьте их в свой репозиторий.
