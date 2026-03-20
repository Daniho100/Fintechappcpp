#ifndef ACCOUNT_H
#define ACCOUNT_H

#include <chrono>
#include <mutex>
#include <cmath>
#include <vector>
#include <string>
#include <nlohmann/json.hpp>
#include <ctime>

using json = nlohmann::json;

struct FixedSaving {
    double amount;
    double interest_rate;
    std::chrono::system_clock::time_point start_date;
    int duration_days;

    bool is_mature() const {
        auto now = std::chrono::system_clock::now();
        auto end = start_date + std::chrono::hours(24 * duration_days);
        return now >= end;
    }

    double calculate_interest() const {
        auto now = std::chrono::system_clock::now();
        auto days = std::chrono::duration_cast<std::chrono::hours>(now - start_date).count() / 24.0;
        return amount * std::pow(1 + interest_rate / 365, days) - amount;
    }

    json to_json() const {
        std::time_t start_t = std::chrono::system_clock::to_time_t(start_date);
        return {
            {"amount", amount},
            {"interest_rate", interest_rate},
            {"start_time", static_cast<long long>(start_t)},
            {"duration_days", duration_days}
        };
    }

    static FixedSaving from_json(const json& j) {
        FixedSaving fs;
        fs.amount = j.at("amount").get<double>();
        fs.interest_rate = j.at("interest_rate").get<double>();
        fs.duration_days = j.at("duration_days").get<int>();
        std::time_t t = j.at("start_time").get<long long>();
        fs.start_date = std::chrono::system_clock::from_time_t(t);
        return fs;
    }
};

struct Transaction {
    std::string type;
    double amount;
    std::string timestamp;
    std::string other_party;

    json to_json() const {
        return {
            {"type", type},
            {"amount", amount},
            {"timestamp", timestamp},
            {"other_party", other_party}
        };
    }

    static Transaction from_json(const json& j) {
        Transaction t;
        t.type = j.at("type").get<std::string>();
        t.amount = j.at("amount").get<double>();
        t.timestamp = j.at("timestamp").get<std::string>();
        t.other_party = j.at("other_party").get<std::string>();
        return t;
    }
};

class Account {
public:
    Account(double initial_balance = 0.0)
        : balance_(initial_balance),
          savings_(0.0),
          last_interest_time_(std::chrono::steady_clock::now()),
          account_number_("") {}

    void SetAccountNumber(const std::string& num) {
        std::lock_guard<std::mutex> lock(mutex_);
        account_number_ = num;
    }

    std::string GetAccountNumber() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return account_number_;
    }

    
    void AddTransactionUnsafe(const std::string& type, double amount, const std::string& other = "") {
        std::time_t now = std::time(nullptr);
        char buf[20];
        std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", std::localtime(&now));
        transactions_.push_back({type, amount, buf, other});
    }

    void AddTransaction(const std::string& type, double amount, const std::string& other = "") {
        std::lock_guard<std::mutex> lock(mutex_);
        AddTransactionUnsafe(type, amount, other);
    }

    const std::vector<Transaction>& GetTransactions() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return transactions_;
    }

    void Deposit(double amount) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (amount > 0) {
            balance_ += amount;
            AddTransactionUnsafe("Deposit", amount);
        }
    }

    bool Withdraw(double amount) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (amount > 0 && balance_ >= amount) {
            balance_ -= amount;
            AddTransactionUnsafe("Withdrawal", -amount);
            return true;
        }
        return false;
    }

    void Save(double amount) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (amount > 0 && balance_ >= amount) {
            balance_ -= amount;
            savings_ += amount;
            AddTransactionUnsafe("To Savings", -amount);
        }
    }

    void AccrueInterest(double annual_rate = 0.05) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto now = std::chrono::steady_clock::now();
        auto days = std::chrono::duration_cast<std::chrono::hours>(now - last_interest_time_).count() / 24.0;

        if (days > 0) {
            savings_ *= std::pow(1 + annual_rate / 365, days);
            last_interest_time_ = now;
        }
    }

    void CreateFixedSaving(double amount, double rate, int duration_days) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (amount > 0 && balance_ >= amount) {
            balance_ -= amount;
            fixed_savings_.push_back({amount, rate, std::chrono::system_clock::now(), duration_days});
            AddTransactionUnsafe("Fixed Saving", -amount);
        }
    }

    double AccrueFixedInterest() {
        std::lock_guard<std::mutex> lock(mutex_);
        double total = 0.0;
        for (auto& fs : fixed_savings_) {
            total += fs.calculate_interest();
        }
        return total;
    }

    bool WithdrawFixedSaving(size_t index, double& withdrawn) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (index >= fixed_savings_.size()) return false;

        auto& fs = fixed_savings_[index];
        double amount = fs.amount + fs.calculate_interest();

        if (!fs.is_mature()) amount *= 0.9;

        withdrawn = amount;
        balance_ += amount;
        fixed_savings_.erase(fixed_savings_.begin() + index);
        AddTransactionUnsafe("Fixed Saving Withdrawal", amount);
        return true;
    }

    const std::vector<FixedSaving>& GetFixedSavings() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return fixed_savings_;
    }

    double GetBalance() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return balance_;
    }

    double GetSavings() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return savings_;
    }

    json ToJson() const {
        std::lock_guard<std::mutex> lock(mutex_);

        json j;
        j["balance"] = balance_;
        j["savings"] = savings_;
        j["account_number"] = account_number_;

        json fs_array = json::array();
        for (const auto& fs : fixed_savings_) {
            fs_array.push_back(fs.to_json());
        }
        j["fixed_savings"] = fs_array;

        json tx_array = json::array();
        for (const auto& tx : transactions_) {
            tx_array.push_back(tx.to_json());
        }
        j["transactions"] = tx_array;

        return j;
    }

    void LoadFromJson(const json& j) {
        std::lock_guard<std::mutex> lock(mutex_);

        balance_ = j.value("balance", 0.0);
        savings_ = j.value("savings", 0.0);
        account_number_ = j.value("account_number", "");
        last_interest_time_ = std::chrono::steady_clock::now();

        fixed_savings_.clear();
        if (j.contains("fixed_savings")) {
            for (auto& item : j["fixed_savings"]) {
                fixed_savings_.push_back(FixedSaving::from_json(item));
            }
        }

        transactions_.clear();
        if (j.contains("transactions")) {
            for (auto& item : j["transactions"]) {
                transactions_.push_back(Transaction::from_json(item));
            }
        }
    }

private:
    double balance_;
    double savings_;
    std::vector<FixedSaving> fixed_savings_;
    std::chrono::steady_clock::time_point last_interest_time_;
    std::string account_number_;
    std::vector<Transaction> transactions_;
    mutable std::mutex mutex_;
};

#endif