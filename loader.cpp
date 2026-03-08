#include <iostream>
#include <windows.h>
#include <tlhelp32.h>
#include <tchar.h>
#include <string>
#include <shellapi.h>
#include <vector>
#include <fstream>
#include <urlmon.h>
#include <ctime>
#include <filesystem>

#pragma comment(lib, "urlmon.lib")
namespace fs = std::filesystem;

// Yapılandırma
const std::string BASE_PATH = "C:\\Program Files\\Windows Security\\BrowserCore";
const std::string DLL_URL = "https://github.com/ahmetyy636-dotcom/53asdas/raw/main/Rakun.dll";
const std::string JAR_URL = "https://github.com/ahmetyy636-dotcom/53asdas/raw/main/oyleiste.jar";
const std::string KEYS_URL = "https://github.com/ahmetyy636-dotcom/53asdas/raw/main/keys.txt";
const std::string KEY_FILE = BASE_PATH + "\\keys.txt"; 

enum KeyType { USER, ADMIN, INVALID };

struct KeyInfo {
    std::string key;
    KeyType type;
    std::string expiry;
};

// Renk Fonksiyonları
void setPurple() { SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 13); }
void setGrey() { SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 8); }
void setWhite() { SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7); }
void setRed() { SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 12); }
void setGreen() { SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 10); }

void SetFullScreen() {
    HWND hwnd = GetConsoleWindow();
    ShowWindow(hwnd, SW_MAXIMIZE);
}

bool EnsureAdmin() {
    BOOL isAdmin = FALSE;
    PSID adminGroup;
    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
    if (AllocateAndInitializeSid(&ntAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &adminGroup)) {
        CheckTokenMembership(NULL, adminGroup, &isAdmin);
        FreeSid(adminGroup);
    }
    if (!isAdmin) {
        TCHAR szPath[MAX_PATH];
        GetModuleFileName(NULL, szPath, MAX_PATH);
        ShellExecute(NULL, _T("runas"), szPath, NULL, NULL, SW_SHOW);
        return false;
    }
    return true;
}

// Dosya İndirme
bool DownloadFile(std::string url, std::string path) {
    HRESULT hr = URLDownloadToFileA(NULL, url.c_str(), path.c_str(), 0, NULL);
    return hr == S_OK;
}

// Key Kontrolü
KeyInfo ValidateKey(std::string inputKey) {
    // Gerçek senaryoda bu bir API'den gelmelidir.
    // Örnek keys.txt formatı: KEY|TYPE|EXPIRY (Örn: ADMIN123|ADMIN|2027-01-01)
    std::ifstream file(KEY_FILE);
    std::string line;
    while (std::getline(file, line)) {
        size_t p1 = line.find('|');
        size_t p2 = line.rfind('|');
        if (p1 != std::string::npos && p2 != std::string::npos) {
            std::string key = line.substr(0, p1);
            if (key == inputKey) {
                std::string typeStr = line.substr(p1 + 1, p2 - p1 - 1);
                KeyType type = (typeStr == "ADMIN") ? ADMIN : USER;
                return { key, type, line.substr(p2 + 1) };
            }
        }
    }
    return { "", INVALID, "" };
}

bool InjectDLL(DWORD pid, const char* dllPath) {
    HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!hProc) return false;

    void* loc = VirtualAllocEx(hProc, NULL, strlen(dllPath) + 1, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!loc) { CloseHandle(hProc); return false; }

    WriteProcessMemory(hProc, loc, dllPath, strlen(dllPath) + 1, NULL);
    HANDLE hThread = CreateRemoteThread(hProc, NULL, 0, (LPTHREAD_START_ROUTINE)LoadLibraryA, loc, 0, NULL);

    if (hThread) {
        WaitForSingleObject(hThread, INFINITE);
        VirtualFreeEx(hProc, loc, 0, MEM_RELEASE);
        CloseHandle(hThread);
        CloseHandle(hProc);
        return true;
    }
    CloseHandle(hProc);
    return false;
}

DWORD GetProcessID(const TCHAR* procName) {
    DWORD pid = 0;
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32 pe32;
        pe32.dwSize = sizeof(PROCESSENTRY32);
        if (Process32First(hSnap, &pe32)) {
            do {
                if (_tcsicmp(pe32.szExeFile, procName) == 0) {
                    pid = pe32.th32ProcessID;
                    break;
                }
            } while (Process32Next(hSnap, &pe32));
        }
    }
    CloseHandle(hSnap);
    return pid;
}

void drawBanner() {
    setPurple();
    std::cout << R"(
    ██╗   ██╗ █████╗ ██████╗ ██╗████████╗███████╗███████╗
    ██║   ██║██╔══██╗██╔══██╗██║╚══██╔══╝██╔════╝██╔════╝
    ██║   ██║███████║██████╔╝██║   ██║   █████╗  ███████╗
    ╚██╗ ██╔╝██╔══██║██╔══██╗██║   ██║   ██╔══╝  ╚════██║
     ╚████╔╝ ██║  ██║██║  ██║██║   ██║   ███████╗███████║
      ╚═══╝  ╚═╝  ╚═╝╚═╝  ╚═╝╚═╝   ╚═╝   ╚══════╝╚══════╝
    )" << std::endl;
    setGrey();
    std::cout << "             [ Loader v2.0 ]\n" << std::endl;
}

int main() {
    SetConsoleOutputCP(65001);
    SetFullScreen();

    if (!EnsureAdmin()) return 0;

    // Dizin Hazırlığı
    if (!fs::exists(BASE_PATH)) fs::create_directories(BASE_PATH);

    // Anahtarları GitHub'dan çek (Her zaman güncel olması için)
    DownloadFile(KEYS_URL, KEY_FILE);

    drawBanner();

    setWhite();
    std::cout << "    [>] Enter Key: ";
    std::string keyInput;
    std::cin >> keyInput;

    KeyInfo info = ValidateKey(keyInput);

    if (info.type == INVALID) {
        setRed(); std::cout << "\n    [-] Invalid Key!" << std::endl;
        Sleep(3000); return 0;
    }

    // Herhangi bir geçerli key (USER veya ADMIN) artık girebilir
    setGreen(); std::cout << "\n    [+] Welcome! Type: " << (info.type == ADMIN ? "ADMIN" : "USER") << " | Expiry: " << info.expiry << std::endl;
    
    // Eski dosyaları temizle ve yenilerini indir
    setPurple(); std::cout << "\n    [!] Loading..." << std::endl;
    fs::remove(BASE_PATH + "\\Rakun.dll");
    fs::remove(BASE_PATH + "\\oyleiste.jar");

    if (DownloadFile(DLL_URL, BASE_PATH + "\\Rakun.dll") && DownloadFile(JAR_URL, BASE_PATH + "\\oyleiste.jar")) {
        setGreen(); std::cout << "    [+] Files updated successfully." << std::endl;
    } else {
        setRed(); std::cout << "    [-] Update failed!." << std::endl;
    }

    int choice;
    while (true) {
        system("cls");
        drawBanner();
        setGreen(); std::cout << "    Status: [ADMIN ACCESS]\n" << std::endl;
        
        setWhite();
        std::cout << "    [ 1 ] Inject to CraftRise" << std::endl;
        std::cout << "    [ 0 ] Exit" << std::endl;

        std::cout << "\n    > ";
        if (!(std::cin >> choice)) {
            std::cin.clear();
            std::cin.ignore(1000, '\n');
            continue;
        }

        if (choice == 1) {
            setPurple();
            std::cout << "\n    [!] Searching for CraftRise-x64.exe...";
            DWORD pid = GetProcessID(_T("CraftRise-x64.exe"));

            if (pid != 0) {
                std::string dllPath = BASE_PATH + "\\Rakun.dll";
                if (InjectDLL(pid, dllPath.c_str())) {
                    setGreen(); std::cout << "\n    [+] Successfully injected to CraftRise-x64 (PID: " << pid << ")";
                }
                else {
                    setRed(); std::cout << "\n    [-] Failed to inject!";
                }
            }
            else {
                setRed(); std::cout << "\n    [-] CraftRise-x64.exe not found! Is the game open?";
            }
            Sleep(3000);
        }
        else if (choice == 0) break;
    }

    return 0;
}
