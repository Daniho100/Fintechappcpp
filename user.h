#ifndef USER_H
#define USER_H

#include <string>
#include <random>
#include <vector>
#include <sstream>
#include <iomanip>
#include <cstdio>
#include <windows.h>
#include <bcrypt.h>

class User {
public:
    User(const std::string& username, const std::string& password)
        : username_(username),
          password_hash_(""),
          salt_("") 
    {
        salt_ = GenerateSalt();
        password_hash_ = HashPassword(password, salt_);
    }

    // LOAD from JSON
    User(const std::string& username, const std::string& p_hash, const std::string& slt)
        : username_(username),
          password_hash_(p_hash),
          salt_(slt) {}

    bool Authenticate(const std::string& password) const {
        return HashPassword(password, salt_) == password_hash_;
    }

    const std::string& GetUsername() const { return username_; }
    const std::string& GetPasswordHash() const { return password_hash_; }
    const std::string& GetSalt() const { return salt_; }

private:
    std::string username_;
    std::string password_hash_;
    std::string salt_;

    static std::string GenerateSalt() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 255);

        std::string salt;
        salt.reserve(32);

        for (int i = 0; i < 16; ++i) {
            uint8_t byte = static_cast<uint8_t>(dis(gen));
            char hex[3];
            sprintf(hex, "%02x", byte);
            salt += hex;
        }

        return salt;
    }

    static std::string HashPassword(const std::string& password, const std::string& salt) {
        BCRYPT_ALG_HANDLE hAlg = nullptr;

        NTSTATUS status = BCryptOpenAlgorithmProvider(
            &hAlg,
            BCRYPT_SHA256_ALGORITHM,
            nullptr,
            0
        );

        if (!BCRYPT_SUCCESS(status)) return "";

        std::vector<BYTE> passBytes(password.begin(), password.end());

        std::vector<BYTE> saltBytes;
        saltBytes.reserve(salt.length() / 2);

        for (size_t i = 0; i < salt.length(); i += 2) {
            unsigned int val = 0;
            sscanf(salt.substr(i, 2).c_str(), "%02x", &val);
            saltBytes.push_back(static_cast<BYTE>(val));
        }

        std::vector<BYTE> derived(32, 0); 

        status = BCryptDeriveKeyPBKDF2(
            hAlg,
            passBytes.data(), static_cast<ULONG>(passBytes.size()),
            saltBytes.data(), static_cast<ULONG>(saltBytes.size()),
            1000,  // iterations
            derived.data(), 32,
            0
        );

        BCryptCloseAlgorithmProvider(hAlg, 0);

        if (!BCRYPT_SUCCESS(status)) return "";

        std::ostringstream oss;
        oss << std::hex << std::setfill('0');

        for (BYTE b : derived) {
            oss << std::setw(2) << static_cast<int>(b);
        }

        return oss.str();
    }
};

#endif 