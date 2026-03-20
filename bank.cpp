#include "bank.h"


struct BankInitializer {
    BankInitializer() {
        g_bank.LoadFromFile();
    }
};

BankInitializer _bank_init;