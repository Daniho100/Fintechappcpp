#include "bank.h"

// Define the global bank instance
Bank g_bank;

struct BankInitializer {
    BankInitializer() {
        g_bank.LoadFromFile();
    }
};

BankInitializer _bank_init;