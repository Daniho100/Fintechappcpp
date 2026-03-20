#ifndef BANK_H
#define BANK_H

#include "user.h"
#include "account.h"
#include "loan.h"
#include <map>
#include <vector>
#include <fstream>
#include <nlohmann/json.hpp>
#include <mutex>
#include <memory>
#include <string>
#include <thread>
#include <random>

extern void DebugLog(const std::string& msg);

using json = nlohmann::json;

class Bank {
public:
    bool RegisterUser(const std::string& username, const std::string& password) {
        std::lock_guard<std::mutex> lock(mutex_);

        if (users_.count(username)) return false;

        users_[username] = std::make_unique<User>(username, password);
        accounts_[username] = std::make_unique<Account>();

        std::string acc_num;
        do {
            acc_num = GenerateAccountNumber();
        } while (account_number_to_username_.count(acc_num));

        accounts_[username]->SetAccountNumber(acc_num);
        account_number_to_username_[acc_num] = username;

        loans_.try_emplace(username);

        AsyncSaveToFile();
        return true;
    }

    User* Login(const std::string& username, const std::string& password) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = users_.find(username);
        if (it != users_.end() && it->second->Authenticate(password)) {
            return it->second.get();
        }
        return nullptr;
    }

    Account* GetAccount(const std::string& username) {
        auto it = accounts_.find(username);
        return (it != accounts_.end()) ? it->second.get() : nullptr;
    }

    bool PayBill(const std::string& username, const std::string& billType, double amount) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto acc = GetAccount(username);
        if (!acc || amount <= 0) return false;

        if (acc->Withdraw(amount)) {
            acc->AddTransaction("Bill Payment (" + billType + ")", -amount);
            AsyncSaveToFile();
            return true;
        }
        return false;
    }

    void AddLoan(const std::string& username, double principal, double rate, int days) {
        std::lock_guard<std::mutex> lock(mutex_);
        loans_.try_emplace(username);
        loans_[username].push_back(std::make_unique<Loan>(principal, rate, days));

        if (auto acc = GetAccount(username)) {
            acc->Deposit(principal);
            acc->AddTransaction("Loan Received", principal);
        }
        AsyncSaveToFile();
    }

    std::vector<Loan*> GetLoans(const std::string& username) {
        std::vector<Loan*> result;
        auto it = loans_.find(username);
        if (it != loans_.end()) {
            for (auto& l : it->second) result.push_back(l.get());
        }
        return result;
    }

    bool Transfer(const std::string& from_username, const std::string& to_account_number, double amount) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto from_acc = GetAccount(from_username);
        if (!from_acc || amount <= 0) return false;

        auto it = account_number_to_username_.find(to_account_number);
        if (it == account_number_to_username_.end()) return false;

        std::string to_username = it->second;
        if (to_username == from_username) return false;

        auto to_acc = GetAccount(to_username);
        if (!to_acc) return false;

        if (from_acc->Withdraw(amount)) {
            to_acc->Deposit(amount);
            from_acc->AddTransaction("Transfer Out", -amount, to_account_number);
            to_acc->AddTransaction("Transfer In", amount, from_acc->GetAccountNumber());
            AsyncSaveToFile();
            return true;
        }
        return false;
    }

    void LoadFromFile(const std::string& filename = "bank_data.json") {
        std::lock_guard<std::mutex> lock(mutex_);

        std::ifstream file(filename);
        if (!file.is_open()) return;

        json j;
        file >> j;

        if (j.contains("users")) {
            for (auto& [uname, data] : j["users"].items()) {
                users_[uname] = std::make_unique<User>(uname, data["password_hash"], data["salt"]);
            }
        }

        if (j.contains("accounts")) {
            for (auto& [uname, data] : j["accounts"].items()) {
                accounts_[uname] = std::make_unique<Account>();
                accounts_[uname]->LoadFromJson(data);
                std::string num = accounts_[uname]->GetAccountNumber();
                if (!num.empty()) account_number_to_username_[num] = uname;
            }
        }

        if (j.contains("loans")) {
            for (auto& [uname, arr] : j["loans"].items()) {
                loans_.try_emplace(uname);
                for (auto& item : arr) {
                    auto loan = std::make_unique<Loan>(0, 0, 1);
                    loan->LoadFromJson(item);
                    loans_[uname].push_back(std::move(loan));
                }
            }
        }
    }

    void SaveToFileSync(const std::string& filename = "bank_data.json") {
        std::lock_guard<std::mutex> lock(mutex_);

        json j;

        for (auto& [uname, user] : users_) {
            j["users"][uname] = {
                {"password_hash", user->GetPasswordHash()},
                {"salt", user->GetSalt()}
            };
        }

        for (auto& [uname, acc] : accounts_) {
            j["accounts"][uname] = acc->ToJson();
        }

        for (auto& [uname, vec] : loans_) {
            json arr = json::array();
            for (auto& loan : vec) arr.push_back(loan->ToJson());
            j["loans"][uname] = arr;
        }

        std::ofstream file(filename);
        file << j.dump(4);
    }

private:
    std::map<std::string, std::unique_ptr<User>> users_;
    std::map<std::string, std::unique_ptr<Account>> accounts_;
    std::map<std::string, std::vector<std::unique_ptr<Loan>>> loans_;
    std::map<std::string, std::string> account_number_to_username_;

    std::mutex mutex_;

    std::string GenerateAccountNumber() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<int> dis(100000, 999999);
        return "ACC-" + std::to_string(dis(gen));
    }

    void AsyncSaveToFile(const std::string& filename = "bank_data.json") {
        std::thread([this, filename]() { SaveToFileSync(filename); }).detach();
    }
};

extern Bank g_bank;

#endif