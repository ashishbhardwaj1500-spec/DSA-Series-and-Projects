#include <iostream>
#include <vector>
using namespace std;

int main() {
    int n = 5;
    int arr[5] = {56,-34,-54,34,65};
    int reset;
    int currsum = 0;
    int maxsum = INT_MIN;
    int sum = 0;
    for (int st = 0; st<n; st++) {
        currsum += arr[st];
        maxsum = max(currsum,maxsum);
        if (currsum < 0) {
            currsum = 0;
        }
    }
    cout << "MAximum Subarray Sum: " << maxsum;
    return 0;
}