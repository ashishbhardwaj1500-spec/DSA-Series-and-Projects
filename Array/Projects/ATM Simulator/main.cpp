
#include <iostream>
#include <fstream>
#include <iomanip>
#include <limits>
#include "credential.h"

using namespace std;

const string FILE_NAME = "balance.txt";

// Load balance from file
double loadBalance() {
    ifstream in(FILE_NAME);

    double balance = 0.0;

    if (in.is_open()) {
        if (!(in >> balance) || balance < 0) {
            cout << "Invalid balance file!\n";
            return 0.0;
        }
    }

    return balance;
}

// Save balance permanently
bool saveBalance(double balance) {
    ofstream out(FILE_NAME);

    if (!out.is_open()) {
        return false;
    }

    out << fixed << setprecision(2) << balance;

    return out.good();
}

// Password authentication
bool authenticate() {
    int attempts = 0;
    int passcode;

    while (attempts < 3) {

        cout << "Enter Password: ";

        if (!(cin >> passcode)) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Invalid input!\n";
            continue;
        }

        if (passcode == pin) {
            cout << "Login Successful!\n";
            return true;
        }

        attempts++;

        cout << "Wrong Password!\n";
        cout << 3 - attempts << " attempts remaining.\n";
    }

    cout << "Card Blocked!\n";
    return false;
}

// Check balance
void checkBalance(double balance) {
    cout << fixed << setprecision(2);
    cout << "Current Balance: " << balance << endl;
}

// Deposit money
void deposit(double& balance) {

    double amount;

    cout << "Deposit Amount: ";

    if (!(cin >> amount)) {
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "Invalid input!\n";
        return;
    }

    if (amount <= 0) {
        cout << "Invalid deposit amount!\n";
        return;
    }

    double newBalance = balance + amount;

    if (saveBalance(newBalance)) {
        balance = newBalance;

        cout << "Deposit Successful!\n";
        checkBalance(balance);
    }
    else {
        cout << "Error saving balance!\n";
    }
}

int main() {

    double balance = loadBalance();

    cout << "\n===== BANKING SYSTEM =====\n";
    cout << "1. Check Balance\n";
    cout << "2. Deposit Money\n";

    int choice;

    cout << "Choice: ";

    if (!(cin >> choice)) {
        cout << "Invalid choice!\n";
        return 1;
    }

    if (choice != 1 && choice != 2) {
        cout << "Invalid option!\n";
        return 1;
    }

    if (!authenticate()) {
        return 0;
    }

    switch (choice) {

        case 1:
            checkBalance(balance);
            break;

        case 2:
            deposit(balance);
            break;
    }

    return 0;
}
