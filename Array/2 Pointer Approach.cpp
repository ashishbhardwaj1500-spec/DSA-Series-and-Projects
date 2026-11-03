#include <iostream>
#include <utility>

using namespace std;

void pointer_swap(int arr[], int sz) {
    int start = 0;
    int end = sz - 1;

    while (start <= end) {
        swap(arr[start], arr[end]);
        start++;
        end--;
    }
}

int main() {
    int arr[10] = {42, 17, 89, 5, 63, 28, 91, 34, 76, 12};

    int sz = 10;

    pointer_swap(arr, sz);

    for (int i = 0; i < sz; i++) {
        cout << arr[i] << " ";
    }

    cout << '\n';

    return 0;
}