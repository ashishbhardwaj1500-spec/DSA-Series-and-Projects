#include <iostream>
using namespace std;

int main() {
    int arr[10] = {42, 17, 89, 5, 63, 28, 91, 34, 76, 12};
    int sum = 0;
    int average = 0;
    for (int i = 0; i < 10; i++) {
        sum += arr[i];


    }
    average = (sum)/(sizeof(arr)/sizeof(int));
    cout << "Sum: " << sum << endl;
    cout << "Average: " << average << endl;

    return 0;
}
