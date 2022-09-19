#include <iostream>
#include <string>

using namespace std;

int main() {

	int num_of_three = 0;

	for (int i = 0; i <= 1000; ++i) {
		string s = to_string(i);
		for (char c : s) {
			if (c == '3') {
				++num_of_three;
			}
		}
	}
	cout << num_of_three;

}
// Решите загадку: Сколько чисел от 1 до 1000 содержат как минимум одну цифру 3?
// Напишите ответ здесь:

// Закомитьте изменения и отправьте их в свой репозиторий.

