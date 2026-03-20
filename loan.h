#ifndef LOAN_H
#define LOAN_H

#include <chrono>
#include <string>
#include <nlohmann/json.hpp>
#include <ctime>

using json = nlohmann::json;

class Loan {
public:
    Loan(double principal, double interest_rate, int duration_days)
        : principal_(principal),
          interest_rate_(interest_rate),
          due_date_(std::chrono::system_clock::now() + std::chrono::hours(24 * duration_days)),
          repayment_amount_(principal * (1 + interest_rate)),
          remaining_(repayment_amount_),
          is_overdue_(false) {}

    bool IsDue() const {
        return std::chrono::system_clock::now() >= due_date_;
    }

    void CheckOverdue() {
        if (IsDue() && remaining_ > 0) {
            is_overdue_ = true;
            remaining_ *= (1 + 0.01); // 1% penalty
        }
    }

    bool Repay(double amount) {
        if (amount > 0 && remaining_ > 0) {
            remaining_ -= amount;
            if (remaining_ <= 0) {
                remaining_ = 0;
                return true;
            }
        }
        return false;
    }

    double GetRemaining() const { return remaining_; }

    std::string GetDueDateStr() const {
        auto tt = std::chrono::system_clock::to_time_t(due_date_);
        std::tm* ptm = std::localtime(&tt);
        char buffer[32];
        std::strftime(buffer, 32, "%Y-%m-%d %H:%M", ptm);
        return buffer;
    }

    bool IsOverdue() const { return is_overdue_; }

    json ToJson() const {
        return {
            {"principal", principal_},
            {"interest_rate", interest_rate_},
            {"remaining", remaining_},
            {"repayment_amount", repayment_amount_},
            {"due_date", (long long)std::chrono::system_clock::to_time_t(due_date_)},
            {"is_overdue", is_overdue_}
        };
    }

    void LoadFromJson(const json& j) {
        principal_ = j.at("principal").get<double>();
        interest_rate_ = j.at("interest_rate").get<double>();
        remaining_ = j.at("remaining").get<double>();
        repayment_amount_ = j.at("repayment_amount").get<double>();
        std::time_t t = j.at("due_date").get<long long>();
        due_date_ = std::chrono::system_clock::from_time_t(t);
        is_overdue_ = j.at("is_overdue").get<bool>();
    }

private:
    double principal_;
    double interest_rate_;
    std::chrono::system_clock::time_point due_date_;
    double repayment_amount_;
    double remaining_;
    bool is_overdue_;
};

#endif