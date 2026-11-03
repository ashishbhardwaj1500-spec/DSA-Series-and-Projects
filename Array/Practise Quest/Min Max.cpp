#include <iostream>
using namespace std;

int main() {
    int arr[10] = {34,45,32,54,65,22,54,56,23,87};
    int sz = sizeof(arr)/sizeof(int);
    int max = INT_MIN;
    int min = INT_MAX;
    for (int i = 0; i < sz; i++) {
       if (max < arr[i]) {
           max = arr[i];
       }
       else if (min >= arr[i]) {
            min = arr[i];
        }
    }

    cout << max <<endl;
    cout << min <<endl;

}