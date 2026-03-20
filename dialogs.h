#ifndef DIALOGS_H
#define DIALOGS_H

#include <windows.h>
#include <string>
#include <fstream>
#include <commctrl.h>
#include <cstdio>
#include "bank.h"
#include "resource.h"

#define WM_REFRESH_DASHBOARD (WM_APP + 5)

extern void DebugLog(const std::string& msg);
extern std::string g_logged_user;
extern Bank g_bank;

// ======================
// Helper functions
// ======================
std::string GetDlgItemText(HWND hDlg, int nID) {
    char buffer[256] = {0};
    GetDlgItemTextA(hDlg, nID, buffer, sizeof(buffer));
    return buffer;
}

double SanitizeDouble(const std::string& str) {
    try { return std::stod(str); } catch (...) { return -1.0; }
}

int SanitizeInt(const std::string& str) {
    try { return std::stoi(str); } catch (...) { return -1; }
}

// ======================
// Register & Login (unchanged)
// ======================
INT_PTR CALLBACK RegisterDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_INITDIALOG:
        DebugLog("RegisterDlgProc: Dialog opened.");
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK: {
            std::string username = GetDlgItemText(hDlg, IDC_USERNAME);
            std::string password = GetDlgItemText(hDlg, IDC_PASSWORD);
            DebugLog("RegisterDlgProc: IDOK pressed. Username='" + username + "'");

            bool success = g_bank.RegisterUser(username, password);
            DebugLog(success ? "RegisterDlgProc: Registration successful." : "RegisterDlgProc: Registration failed.");

            if (success) {
                g_logged_user = username;
                MessageBoxA(hDlg, "Registered successfully!\nYou can now login.", "Success", MB_OK);
                EndDialog(hDlg, IDOK);
            } else {
                MessageBoxA(hDlg, "Username already taken.", "Error", MB_OK | MB_ICONERROR);
            }
            return TRUE;
        }
        case IDCANCEL:
            EndDialog(hDlg, IDCANCEL);
            return TRUE;
        }
    }
    return FALSE;
}

INT_PTR CALLBACK LoginDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_INITDIALOG:
        DebugLog("LoginDlgProc: Dialog opened.");
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case ID_LOGIN: {
            std::string user = GetDlgItemText(hDlg, IDC_USERNAME);
            std::string pass = GetDlgItemText(hDlg, IDC_PASSWORD);
            DebugLog("LoginDlgProc: Attempt login for " + user);

            User* u = g_bank.Login(user, pass);
            if (u != nullptr) {
                g_logged_user = user;
                EndDialog(hDlg, IDOK);
            } else {
                MessageBoxA(hDlg, "Invalid credentials.", "Error", MB_OK | MB_ICONERROR);
            }
            return TRUE;
        }
        case ID_REGISTER:
            DialogBoxA(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_REGISTER_DIALOG), hDlg, RegisterDlgProc);
            return TRUE;
        case IDCANCEL:
            EndDialog(hDlg, IDCANCEL);
            return TRUE;
        }
    }
    return FALSE;
}

// ======================
// Transaction Dialogs 
// ======================
INT_PTR CALLBACK DepositDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_INITDIALOG:
            DebugLog("DepositDlgProc: Dialog opened.");
            return TRUE;

        case WM_COMMAND:
            DebugLog("DepositDlgProc: Button ID = " + std::to_string(LOWORD(wParam)));

            if (LOWORD(wParam) == IDOK || LOWORD(wParam) == 1) {
                std::string amt_str = GetDlgItemText(hDlg, IDC_AMOUNT);
                double amt = SanitizeDouble(amt_str);

                if (amt <= 0) {
                    MessageBoxA(hDlg, "Invalid amount.", "Error", MB_OK | MB_ICONERROR);
                    return TRUE;
                }

                if (auto acc = g_bank.GetAccount(g_logged_user)) {
                    acc->Deposit(amt);
                    DebugLog("DepositDlgProc: Deposit successful");
                    MessageBoxA(hDlg, "Deposited!", "Success", MB_OK);
                }
                EndDialog(hDlg, IDOK);
                return TRUE;
            }
            if (LOWORD(wParam) == IDCANCEL) {
                DebugLog("DepositDlgProc: Cancel pressed.");
                EndDialog(hDlg, IDCANCEL);
                return TRUE;
            }
            break;
    }
    return FALSE;
}

INT_PTR CALLBACK WithdrawDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_INITDIALOG:
            DebugLog("WithdrawDlgProc: Dialog opened.");
            return TRUE;

        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK || LOWORD(wParam) == 1) {
                std::string amt_str = GetDlgItemText(hDlg, IDC_AMOUNT);
                double amt = SanitizeDouble(amt_str);

                if (amt <= 0) {
                    MessageBoxA(hDlg, "Invalid amount.", "Error", MB_OK | MB_ICONERROR);
                    return TRUE;
                }

                if (auto acc = g_bank.GetAccount(g_logged_user)) {
                    if (acc->Withdraw(amt)) {
                        MessageBoxA(hDlg, "Withdrawn!", "Success", MB_OK);
                    } else {
                        MessageBoxA(hDlg, "Insufficient funds.", "Error", MB_OK | MB_ICONERROR);
                    }
                }
                EndDialog(hDlg, IDOK);
                return TRUE;
            }
            if (LOWORD(wParam) == IDCANCEL) {
                EndDialog(hDlg, IDCANCEL);
                return TRUE;
            }
            break;
    }
    return FALSE;
}

INT_PTR CALLBACK BillDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_INITDIALOG: {
            DebugLog("BillDlgProc: Dialog opened.");
            HWND hCombo = GetDlgItem(hDlg, IDC_BILL_TYPE);
            SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)"Electricity");
            SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)"Water");
            SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)"Airtime");
            SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)"Data");
            SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)"Betting");
            SendMessage(hCombo, CB_SETCURSEL, 0, 0);
            return TRUE;
        }

        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK || LOWORD(wParam) == 1) {
                std::string amt_str = GetDlgItemText(hDlg, IDC_AMOUNT);
                double amt = SanitizeDouble(amt_str);

                HWND hCombo = GetDlgItem(hDlg, IDC_BILL_TYPE);
                int sel = (int)SendMessage(hCombo, CB_GETCURSEL, 0, 0);
                char type[32] = {0};
                SendMessage(hCombo, CB_GETLBTEXT, sel, (LPARAM)type);

                if (amt <= 0) {
                    MessageBoxA(hDlg, "Invalid amount.", "Error", MB_OK | MB_ICONERROR);
                    return TRUE;
                }

                if (g_bank.PayBill(g_logged_user, type, amt)) {
                    MessageBoxA(hDlg, "Bill paid!", "Success", MB_OK);
                } else {
                    MessageBoxA(hDlg, "Insufficient funds or error.", "Error", MB_OK | MB_ICONERROR);
                }
                EndDialog(hDlg, IDOK);
                return TRUE;
            }
            if (LOWORD(wParam) == IDCANCEL) {
                EndDialog(hDlg, IDCANCEL);
                return TRUE;
            }
            break;
    }
    return FALSE;
}


INT_PTR CALLBACK SaveDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_INITDIALOG:
        DebugLog("SaveDlgProc: Dialog opened.");
        return TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == 1) {
            std::string amt_str = GetDlgItemText(hDlg, IDC_AMOUNT);
            double amt = SanitizeDouble(amt_str);

            if (amt > 0) {
                if (auto acc = g_bank.GetAccount(g_logged_user)) {
                    acc->Save(amt);
                    MessageBoxA(hDlg, "Amount moved to savings!", "Success", MB_OK);
                }
            } else {
                MessageBoxA(hDlg, "Invalid amount.", "Error", MB_OK | MB_ICONERROR);
            }
            EndDialog(hDlg, IDOK);
            return TRUE;
        }
        if (LOWORD(wParam) == IDCANCEL) {
            EndDialog(hDlg, IDCANCEL);
            return TRUE;
        }
    }
    return FALSE;
}

INT_PTR CALLBACK FixedSavingDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_INITDIALOG:
        DebugLog("FixedSavingDlgProc: Dialog opened.");
        return TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == 1) {
            double amt  = SanitizeDouble(GetDlgItemText(hDlg, IDC_AMOUNT));
            double rate = SanitizeDouble(GetDlgItemText(hDlg, IDC_INTEREST_RATE));
            int    days = SanitizeInt(GetDlgItemText(hDlg, IDC_DURATION_DAYS));

            if (amt > 0 && rate > 0 && days > 0) {
                if (auto acc = g_bank.GetAccount(g_logged_user)) {
                    acc->CreateFixedSaving(amt, rate, days);
                    MessageBoxA(hDlg, "Fixed saving created!", "Success", MB_OK);
                }
            } else {
                MessageBoxA(hDlg, "Invalid values!", "Error", MB_OK | MB_ICONERROR);
            }
            EndDialog(hDlg, IDOK);
            return TRUE;
        }
        if (LOWORD(wParam) == IDCANCEL) {
            EndDialog(hDlg, IDCANCEL);
            return TRUE;
        }
    }
    return FALSE;
}

INT_PTR CALLBACK LoanDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_INITDIALOG:
        DebugLog("LoanDlgProc: Dialog opened.");
        return TRUE;

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == 1) {
            double principal = SanitizeDouble(GetDlgItemText(hDlg, IDC_AMOUNT));
            double rate      = SanitizeDouble(GetDlgItemText(hDlg, IDC_INTEREST_RATE));
            int    days      = SanitizeInt(GetDlgItemText(hDlg, IDC_DURATION_DAYS));

            if (principal > 0 && rate > 0 && days > 0) {
                g_bank.AddLoan(g_logged_user, principal, rate, days);
                MessageBoxA(hDlg, "Loan approved!\nFunds added to balance.", "Success", MB_OK);
            } else {
                MessageBoxA(hDlg, "Invalid values!", "Error", MB_OK | MB_ICONERROR);
            }
            EndDialog(hDlg, IDOK);
            return TRUE;
        }
        if (LOWORD(wParam) == IDCANCEL) {
            EndDialog(hDlg, IDCANCEL);
            return TRUE;
        }
    }
    return FALSE;
}

INT_PTR CALLBACK TransferDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_INITDIALOG: {
        DebugLog("TransferDlgProc: Dialog opened.");
        if (auto acc = g_bank.GetAccount(g_logged_user)) {
            SetDlgItemTextA(hDlg, IDC_SENDER_ACCOUNT, acc->GetAccountNumber().c_str());
        }
        return TRUE;
    }

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == 1) {
            std::string recipient = GetDlgItemText(hDlg, IDC_RECIPIENT_ACCOUNT);
            double amt = SanitizeDouble(GetDlgItemText(hDlg, IDC_TRANSFER_AMOUNT));

            if (amt > 0 && !recipient.empty()) {
                if (g_bank.Transfer(g_logged_user, recipient, amt)) {
                    MessageBoxA(hDlg, "Transfer successful!", "Success", MB_OK);
                } else {
                    MessageBoxA(hDlg, "Transfer failed.\nInvalid account number or insufficient funds.", "Error", MB_OK | MB_ICONERROR);
                }
            } else {
                MessageBoxA(hDlg, "Invalid amount or account number.", "Error", MB_OK | MB_ICONERROR);
            }
            EndDialog(hDlg, IDOK);
            return TRUE;
        }
        if (LOWORD(wParam) == IDCANCEL) {
            EndDialog(hDlg, IDCANCEL);
            return TRUE;
        }
    }
    return FALSE;
}



// ======================
// View Loans
// ======================
INT_PTR CALLBACK ViewLoansDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_INITDIALOG: {
        DebugLog("ViewLoansDlgProc: Opening loans list");
        HWND hList = GetDlgItem(hDlg, IDC_LOAN_LIST);
        ListView_SetExtendedListViewStyle(hList, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

        LVCOLUMN lvc = {};
        lvc.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_FMT;
        lvc.fmt = LVCFMT_LEFT;
        lvc.cx = 140; lvc.pszText = const_cast<LPSTR>("Due Date");     ListView_InsertColumn(hList, 0, &lvc);
        lvc.cx = 100; lvc.pszText = const_cast<LPSTR>("Remaining");    ListView_InsertColumn(hList, 1, &lvc);
        lvc.cx = 80;  lvc.pszText = const_cast<LPSTR>("Status");       ListView_InsertColumn(hList, 2, &lvc);

        auto loans = g_bank.GetLoans(g_logged_user);
        int i = 0;
        for (auto* loan : loans) {
            LVITEM lvi = {};
            lvi.mask = LVIF_TEXT;
            lvi.iItem = i;
            lvi.pszText = const_cast<LPSTR>(loan->GetDueDateStr().c_str());
            ListView_InsertItem(hList, &lvi);

            char buf[32];
            std::snprintf(buf, sizeof(buf), "%.2f", loan->GetRemaining());
            ListView_SetItemText(hList, i, 1, buf);
            ListView_SetItemText(hList, i, 2, const_cast<LPSTR>(loan->IsOverdue() ? "OVERDUE" : "Active"));
            i++;
        }
        return TRUE;
    }
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK) {
            EndDialog(hDlg, IDOK);
            return TRUE;
        }
    }
    return FALSE;
}

// ======================
// View Fixed Savings
// ======================
INT_PTR CALLBACK ViewFixedDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_INITDIALOG: {
        DebugLog("ViewFixedDlgProc: Opening fixed savings list");
        HWND hList = GetDlgItem(hDlg, IDC_FIXED_LIST);
        ListView_SetExtendedListViewStyle(hList, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

        LVCOLUMN lvc = {};
        lvc.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_FMT;
        lvc.fmt = LVCFMT_LEFT;
        lvc.cx = 90;  lvc.pszText = const_cast<LPSTR>("Amount");   ListView_InsertColumn(hList, 0, &lvc);
        lvc.cx = 70;  lvc.pszText = const_cast<LPSTR>("Rate");     ListView_InsertColumn(hList, 1, &lvc);
        lvc.cx = 60;  lvc.pszText = const_cast<LPSTR>("Days");     ListView_InsertColumn(hList, 2, &lvc);
        lvc.cx = 100; lvc.pszText = const_cast<LPSTR>("Interest"); ListView_InsertColumn(hList, 3, &lvc);
        lvc.cx = 80;  lvc.pszText = const_cast<LPSTR>("Mature?");  ListView_InsertColumn(hList, 4, &lvc);

        if (auto acc = g_bank.GetAccount(g_logged_user)) {
            const auto& fs_list = acc->GetFixedSavings();
            int i = 0;
            for (const auto& fs : fs_list) {
                LVITEM lvi = {};
                lvi.mask = LVIF_TEXT;
                lvi.iItem = i;

                char buf[32];
                std::snprintf(buf, sizeof(buf), "%.2f", fs.amount);
                lvi.pszText = buf;
                ListView_InsertItem(hList, &lvi);

                std::snprintf(buf, sizeof(buf), "%.2f", fs.interest_rate);
                ListView_SetItemText(hList, i, 1, buf);
                std::snprintf(buf, sizeof(buf), "%d", fs.duration_days);
                ListView_SetItemText(hList, i, 2, buf);
                std::snprintf(buf, sizeof(buf), "%.2f", fs.calculate_interest());
                ListView_SetItemText(hList, i, 3, buf);
                ListView_SetItemText(hList, i, 4, const_cast<LPSTR>(fs.is_mature() ? "Yes" : "No"));
                i++;
            }
        }
        return TRUE;
    }
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK) {
            EndDialog(hDlg, IDOK);
            return TRUE;
        }
    }
    return FALSE;
}

// ======================
// View Transaction History
// ======================
INT_PTR CALLBACK ViewHistoryDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_INITDIALOG: {
        DebugLog("ViewHistoryDlgProc: Opening history");
        HWND hList = GetDlgItem(hDlg, IDC_HISTORY_LIST);
        ListView_SetExtendedListViewStyle(hList, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

        LVCOLUMN lvc = {};
        lvc.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_FMT;
        lvc.fmt = LVCFMT_LEFT;
        lvc.cx = 110; lvc.pszText = const_cast<LPSTR>("Date");      ListView_InsertColumn(hList, 0, &lvc);
        lvc.cx = 100; lvc.pszText = const_cast<LPSTR>("Type");      ListView_InsertColumn(hList, 1, &lvc);
        lvc.cx = 80;  lvc.pszText = const_cast<LPSTR>("Amount");    ListView_InsertColumn(hList, 2, &lvc);
        lvc.cx = 120; lvc.pszText = const_cast<LPSTR>("Other");     ListView_InsertColumn(hList, 3, &lvc);

        if (auto acc = g_bank.GetAccount(g_logged_user)) {
            const auto& hist = acc->GetTransactions();
            int i = 0;
            for (const auto& tx : hist) {
                LVITEM lvi = {};
                lvi.mask = LVIF_TEXT;
                lvi.iItem = i;
                lvi.pszText = const_cast<LPSTR>(tx.timestamp.c_str());
                ListView_InsertItem(hList, &lvi);

                ListView_SetItemText(hList, i, 1, const_cast<LPSTR>(tx.type.c_str()));
                char buf[32];
                std::snprintf(buf, sizeof(buf), "%.2f", tx.amount);
                ListView_SetItemText(hList, i, 2, buf);
                ListView_SetItemText(hList, i, 3, const_cast<LPSTR>(tx.other_party.c_str()));
                i++;
            }
        }
        return TRUE;
    }
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK) {
            EndDialog(hDlg, IDOK);
            return TRUE;
        }
    }
    return FALSE;
}


// ======================
// Main Dashboard
// ======================
INT_PTR CALLBACK MainWindowDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_INITDIALOG: {
        DebugLog("MainWindowDlgProc: Dashboard opened for " + g_logged_user);

        if (auto acc = g_bank.GetAccount(g_logged_user)) {
            acc->AccrueInterest();

            char buf[64];
            std::snprintf(buf, sizeof(buf), "Balance: %.2f", acc->GetBalance());
            SetDlgItemTextA(hDlg, IDC_BALANCE_LABEL, buf);

            std::snprintf(buf, sizeof(buf), "Savings: %.2f", acc->GetSavings());
            SetDlgItemTextA(hDlg, IDC_SAVINGS_LABEL, buf);

            SetDlgItemTextA(hDlg, IDC_ACCOUNT_NUMBER, acc->GetAccountNumber().c_str());
        }
        return TRUE;
    }

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case ID_DEPOSIT:    DialogBoxA(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_DEPOSIT_DIALOG), hDlg, DepositDlgProc); PostMessage(hDlg, WM_REFRESH_DASHBOARD, 0, 0); break;
        case ID_WITHDRAW:   DialogBoxA(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_WITHDRAW_DIALOG), hDlg, WithdrawDlgProc); PostMessage(hDlg, WM_REFRESH_DASHBOARD, 0, 0); break;
        case ID_PAY_BILL:   DialogBoxA(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_BILL_DIALOG), hDlg, BillDlgProc); PostMessage(hDlg, WM_REFRESH_DASHBOARD, 0, 0); break;
        case ID_SAVE:       DialogBoxA(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_SAVE_DIALOG), hDlg, SaveDlgProc); PostMessage(hDlg, WM_REFRESH_DASHBOARD, 0, 0); break;
        case ID_FIXED_SAVE: DialogBoxA(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_FIXED_SAVE_DIALOG), hDlg, FixedSavingDlgProc); PostMessage(hDlg, WM_REFRESH_DASHBOARD, 0, 0); break;
        case ID_TAKE_LOAN:  DialogBoxA(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_LOAN_DIALOG), hDlg, LoanDlgProc); PostMessage(hDlg, WM_REFRESH_DASHBOARD, 0, 0); break;
        case ID_TRANSFER:   DialogBoxA(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_TRANSFER_DIALOG), hDlg, TransferDlgProc); PostMessage(hDlg, WM_REFRESH_DASHBOARD, 0, 0); break;

        case ID_VIEW_LOANS: DialogBoxA(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_VIEW_LOANS_DIALOG), hDlg, ViewLoansDlgProc); break;
        case ID_VIEW_FIXED: DialogBoxA(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_VIEW_FIXED_DIALOG), hDlg, ViewFixedDlgProc); break;
        case ID_VIEW_HISTORY: DialogBoxA(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_HISTORY_DIALOG), hDlg, ViewHistoryDlgProc); break;

         case ID_LOGOUT:
            DebugLog("Logging out user: " + g_logged_user);
            g_logged_user.clear();
            EndDialog(hDlg, 0);
            DialogBoxA(
                GetModuleHandle(NULL),
                MAKEINTRESOURCE(IDD_LOGIN_DIALOG), 
                NULL,
                LoginDlgProc
            );
            return TRUE;
        }
        return TRUE;



    case WM_REFRESH_DASHBOARD:
        if (auto acc = g_bank.GetAccount(g_logged_user)) {
            acc->AccrueInterest();
            char buf[64];
            std::snprintf(buf, sizeof(buf), "Balance: %.2f", acc->GetBalance());
            SetDlgItemTextA(hDlg, IDC_BALANCE_LABEL, buf);
            std::snprintf(buf, sizeof(buf), "Savings: %.2f", acc->GetSavings());
            SetDlgItemTextA(hDlg, IDC_SAVINGS_LABEL, buf);
        }
        return TRUE;
    }
    return FALSE;
}

#endif 