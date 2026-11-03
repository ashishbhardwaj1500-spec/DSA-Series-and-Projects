#include <iostream>
#include <vector>
using namespace std;

int main() {
    vector<char> vec = {'G', 'k', 'A', 'z', 'P', 'm', 'R', 'b', 'X', 'q'};
    cout << vec.size() << endl;
    vec.push_back('\0');
    cout << vec.size() << endl;
    vec.pop_back();
    cout <<vec.front() <<endl;
    cout <<vec.back() <<endl;
    cout << vec.at(4) << endl;
    cout << vec.size() << endl;
    for (char i: vec) {
        cout << i << endl;
    }

    vector<int> ve;
    ve.push_back(1);
    ve.push_back(2);
    ve.push_back(3);
    ve.push_back(4);


    cout <<vec.size() <<endl;//5
    cout << vec.capacity() << endl;//8

}