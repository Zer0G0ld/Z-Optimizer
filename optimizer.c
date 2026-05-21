// Define que o alvo é o Windows Vista ou superior
#define _WIN32_WINNT 0x0600
#define WINVER 0x0600

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <shlobj.h>
#include <shlwapi.h>  
#include <tlhelp32.h>
#include <psapi.h>     
#include <time.h>
#include <winsvc.h>

#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "psapi.lib")

// ==================== DEFINIÇÕES DO KERNEL ====================
#ifndef SystemMemoryListInformation
#define SystemMemoryListInformation 80
#endif

#ifndef STATUS_PRIVILEGE_NOT_HELD
#define STATUS_PRIVILEGE_NOT_HELD 0xC0000061L
#endif

typedef enum _SYSTEM_MEMORY_LIST_COMMAND {
    MemoryPurgeStandbyList = 4,
    MemoryPurgeLowPriorityStandbyList = 5,
    MemoryPurgeAllStandbyList = 7
} SYSTEM_MEMORY_LIST_COMMAND;

// ==================== ESTRUTURAS DE DADOS ====================
typedef struct {
    const char* internalName;
    const char* friendlyName;
    DWORD startupType;
} WindowsServiceTarget;

typedef struct {
    HKEY hRootKey;
    const char* subKey;
    const char* valueName;
    DWORD valueType;
    DWORD dwValue;
    const char* friendlyName;
} RegistryTarget;

// ==================== FUNÇÕES DE LOG ====================
void LogError(const char* operation, DWORD errorCode) {
    char* errorMsg = NULL;
    FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                   NULL, errorCode, 0, (LPSTR)&errorMsg, 0, NULL);
    printf("  [ERRO] %s: %s (Código: %lu)\n", operation, errorMsg, errorCode);
    LocalFree(errorMsg);
}

// ==================== FUNÇÕES DE PRIVILÉGIO ====================
BOOL EnablePrivilege(LPCSTR privilegeName) {
    HANDLE hToken;
    TOKEN_PRIVILEGES tp;
    LUID luid;
    
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
        return FALSE;
    }
    
    if (!LookupPrivilegeValueA(NULL, privilegeName, &luid)) {
        CloseHandle(hToken);
        return FALSE;
    }
    
    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    
    BOOL result = AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), NULL, NULL);
    DWORD err = GetLastError();
    CloseHandle(hToken);
    
    return result && (err != ERROR_NOT_ALL_ASSIGNED);
}

// ==================== WAIT FOR PROCESS EXIT ====================
BOOL WaitForProcessExit(const char* processName, DWORD timeoutMs) {
    DWORD startTime = GetTickCount();
    
    while ((GetTickCount() - startTime) < timeoutMs) {
        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnapshot == INVALID_HANDLE_VALUE) {
            return FALSE;
        }
        
        PROCESSENTRY32 pe;
        pe.dwSize = sizeof(PROCESSENTRY32);
        BOOL found = FALSE;
        
        if (Process32First(hSnapshot, &pe)) {
            do {
                if (stricmp(pe.szExeFile, processName) == 0) {
                    found = TRUE;
                    break;
                }
            } while (Process32Next(hSnapshot, &pe));
        }
        
        CloseHandle(hSnapshot);
        
        if (!found) {
            return TRUE;
        }
        
        Sleep(100);
    }
    
    return FALSE;
}

// ==================== PURGA DA STANDBY LIST ====================
void PurgeStandbyList() {
    printf("\n## Liberando Standby List (Cache do Sistema)...\n");
    printf("  [*] Solicitando purga do cache de arquivos do Kernel...\n");
    
    EnablePrivilege("SeProfileSingleProcessPrivilege");
    
    HMODULE hNtdll = GetModuleHandleA("ntdll.dll");
    if (!hNtdll) {
        LogError("GetModuleHandleA (ntdll.dll)", GetLastError());
        return;
    }
    
    typedef LONG (WINAPI *pfnNtSetSystemInformation)(
        DWORD SystemInformationClass,
        PVOID SystemInformation,
        ULONG SystemInformationLength
    );
    
    pfnNtSetSystemInformation NtSetSystemInformation =
        (pfnNtSetSystemInformation)GetProcAddress(hNtdll, "NtSetSystemInformation");
    
    if (!NtSetSystemInformation) {
        LogError("GetProcAddress (NtSetSystemInformation)", GetLastError());
        return;
    }
    
    SYSTEM_MEMORY_LIST_COMMAND commands[] = {
        MemoryPurgeAllStandbyList,
        MemoryPurgeStandbyList,
        MemoryPurgeLowPriorityStandbyList
    };
    
    const char* cmdNames[] = {"PurgeAll (Win10+)", "PurgeStandby (Win8+)", "PurgeLowPriority"};
    BOOL purged = FALSE;
    
    for (int i = 0; i < 3; i++) {
        LONG status = NtSetSystemInformation(
            SystemMemoryListInformation,
            &commands[i],
            sizeof(commands[i])
        );
        
        if (status == 0) {
            printf("  [OK] Standby List purgada! (Metodo: %s)\n", cmdNames[i]);
            purged = TRUE;
            break;
        } else if (status == STATUS_PRIVILEGE_NOT_HELD) {
            printf("  [ERRO] Privilegios insuficientes para %s\n", cmdNames[i]);
        } else {
            printf("  [INFO] %s retornou: 0x%08lX\n", cmdNames[i], status);
        }
    }
    
    if (!purged) {
        printf("  [FALHA] Nenhum metodo de purga funcionou\n");
    }
}

// ==================== OTIMIZAÇÃO DE SERVIÇOS ====================
void OptimizeWindowsServices() {
    printf("\n## Otimizando Servicos e Cortando Telemetria...\n");
    
    WindowsServiceTarget targets[] = {
        {"DiagTrack", "Telemetria", SERVICE_DISABLED},
        {"dmwappushservice", "Push Router", SERVICE_DISABLED},
        {"SysMain", "SuperFetch", SERVICE_DEMAND_START},
        {"WSearch", "Windows Search", SERVICE_DEMAND_START}
    };
    int totalServices = sizeof(targets) / sizeof(targets[0]);
    
    SC_HANDLE hSCM = OpenSCManagerA(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    if (!hSCM) {
        LogError("OpenSCManagerA", GetLastError());
        return;
    }
    
    int optimizedCount = 0;
    
    for (int i = 0; i < totalServices; i++) {
        SC_HANDLE hService = OpenServiceA(hSCM, targets[i].internalName, 
                                         SERVICE_CHANGE_CONFIG | SERVICE_STOP);
        
        if (!hService) {
            if (GetLastError() != ERROR_SERVICE_DOES_NOT_EXIST) {
                printf("  [INFO] Servico %s nao encontrado\n", targets[i].internalName);
            }
            continue;
        }
        
        if (ChangeServiceConfigA(hService, SERVICE_NO_CHANGE, targets[i].startupType,
                                 SERVICE_NO_CHANGE, NULL, NULL, NULL, NULL, NULL, NULL, NULL)) {
            const char* actionStr = (targets[i].startupType == SERVICE_DISABLED) ? "Desativado" : "Manual";
            printf("  [OK] %s -> %s\n", targets[i].friendlyName, actionStr);
            optimizedCount++;
            
            SERVICE_STATUS status;
            ControlService(hService, SERVICE_CONTROL_STOP, &status);
        } else {
            printf("  [AVISO] Falha ao alterar: %s\n", targets[i].friendlyName);
        }
        
        CloseHandle(hService);
    }
    
    CloseHandle(hSCM);
    printf("  [OK] %d servicos reconfigurados\n", optimizedCount);
}

// ==================== OTIMIZAÇÃO DO REGISTRO ====================
void OptimizeWindowsRegistry() {
    printf("\n## Aplicando Politicas de Privacidade...\n");
    
    RegistryTarget targets[] = {
        {HKEY_CURRENT_USER, 
         "Software\\Policies\\Microsoft\\Windows\\Explorer", 
         "DisableSearchBoxSuggestions", REG_DWORD, 1, 
         "Desativar buscas do Bing"},
        
        {HKEY_LOCAL_MACHINE, 
         "Software\\Policies\\Microsoft\\Windows\\DataCollection", 
         "AllowTelemetry", REG_DWORD, 0, 
         "Desativar Telemetria"},
        
        {HKEY_LOCAL_MACHINE, 
         "Software\\Policies\\Microsoft\\Windows Defender\\Spynet", 
         "SubmitSamplesConsent", REG_DWORD, 2, 
         "Desativar envio ao Spynet"}
    };
    
    int totalTargets = sizeof(targets) / sizeof(targets[0]);
    int optimizedCount = 0;
    
    for (int i = 0; i < totalTargets; i++) {
        HKEY hKey;
        LONG status;
        
        status = RegCreateKeyExA(
            targets[i].hRootKey,
            targets[i].subKey,
            0,
            NULL,
            REG_OPTION_NON_VOLATILE,
            KEY_SET_VALUE,
            NULL,
            &hKey,
            NULL
        );
        
        if (status == ERROR_SUCCESS) {
            status = RegSetValueExA(
                hKey,
                targets[i].valueName,
                0,
                targets[i].valueType,
                (const BYTE*)&targets[i].dwValue,
                sizeof(DWORD)
            );
            
            if (status == ERROR_SUCCESS) {
                printf("  [OK] %s\n", targets[i].friendlyName);
                optimizedCount++;
            } else {
                printf("  [AVISO] Falha em %s (Erro: %ld)\n", targets[i].friendlyName, status);
            }
            
            RegCloseKey(hKey);
        }
    }
    
    printf("  [OK] %d modificacoes aplicadas\n", optimizedCount);
}

// ==================== LIMPEZA DE WORKING SET ====================
void OptimizeMemoryUsage() {
    printf("\n## Trimming de Working Set em processos...\n");
    
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        LogError("CreateToolhelp32Snapshot", GetLastError());
        return;
    }
    
    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(PROCESSENTRY32);
    
    const char* criticalProcesses[] = {
        "System", "Registry", "csrss.exe", "lsass.exe",
        "winlogon.exe", "services.exe", "svchost.exe"
    };
    
    int optimized = 0;
    int denied = 0;
    
    if (Process32First(hSnapshot, &pe)) {
        do {
            BOOL isCritical = FALSE;
            for (int i = 0; i < sizeof(criticalProcesses)/sizeof(criticalProcesses[0]); i++) {
                if (stricmp(pe.szExeFile, criticalProcesses[i]) == 0) {
                    isCritical = TRUE;
                    break;
                }
            }
            
            if (isCritical) continue;
            
            HANDLE hProcess = OpenProcess(PROCESS_SET_QUOTA | PROCESS_QUERY_INFORMATION,
                                         FALSE, pe.th32ProcessID);
            if (hProcess) {
                if (EmptyWorkingSet(hProcess)) {
                    optimized++;
                } else {
                    if (GetLastError() == ERROR_ACCESS_DENIED) denied++;
                }
                CloseHandle(hProcess);
            }
        } while (Process32Next(hSnapshot, &pe));
    }
    
    CloseHandle(hSnapshot);
    
    if (optimized > 0) {
        printf("  [OK] %d processos otimizados\n", optimized);
    }
    if (denied > 0) {
        printf("  [AVISO] %d processos com acesso negado\n", denied);
    }
}

// ==================== CLEAN FUNCTIONS ====================
void CleanBrowserCaches() {
    printf("\n## Limpando caches de navegadores...\n");
    
    const char* browserPaths[] = {
        "%localappdata%\\Google\\Chrome\\User Data\\Default\\Cache",
        "%localappdata%\\Microsoft\\Edge\\User Data\\Default\\Cache",
        "%appdata%\\Mozilla\\Firefox\\Profiles\\*\\cache2"
    };
    
    for (size_t i = 0; i < sizeof(browserPaths)/sizeof(browserPaths[0]); i++) {
        char cmd[512];
        snprintf(cmd, sizeof(cmd), "if exist \"%s\" (del /f /q /s \"%s\\*\" >nul 2>&1)", 
                 browserPaths[i], browserPaths[i]);
        system(cmd);
    }
    printf("  [OK] Caches de navegadores limpos\n");
}

void CleanThumbnailCache() {
    printf("\n## Resetando cache de miniaturas...\n");
    
    system("taskkill /f /im explorer.exe >nul 2>&1");
    
    if (!WaitForProcessExit("explorer.exe", 5000)) {
        printf("  [AVISO] Explorer nao terminou em 5 segundos\n");
    }
    
    char cmd[512];
    snprintf(cmd, sizeof(cmd),
             "del /f /q \"%s\\AppData\\Local\\Microsoft\\Windows\\Explorer\\thumbcache_*.db\" >nul 2>&1",
             getenv("USERPROFILE"));
    system(cmd);
    
    system("start explorer.exe");
    Sleep(500);
    printf("  [OK] Cache de miniaturas resetado\n");
}

void SmartPrefetchCleanup() {
    printf("\n## Limpeza inteligente de Prefetch (>30 dias)...\n");
    
    char windowsPath[MAX_PATH];
    char searchPath[MAX_PATH];
    
    if (!GetWindowsDirectoryA(windowsPath, MAX_PATH)) {
        LogError("GetWindowsDirectory", GetLastError());
        return;
    }
    
    PathCombineA(searchPath, windowsPath, "Prefetch\\*.pf");
    
    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA(searchPath, &findData);
    
    if (hFind == INVALID_HANDLE_VALUE) {
        printf("  [INFO] Nenhum arquivo .pf encontrado\n");
        return;
    }
    
    FILETIME ftNow;
    GetSystemTimeAsFileTime(&ftNow);
    ULARGE_INTEGER ullNow;
    ullNow.LowPart = ftNow.dwLowDateTime;
    ullNow.HighPart = ftNow.dwHighDateTime;
    
    int deleted = 0;
    
    do {
        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
        
        ULARGE_INTEGER ullFile;
        ullFile.LowPart = findData.ftLastWriteTime.dwLowDateTime;
        ullFile.HighPart = findData.ftLastWriteTime.dwHighDateTime;
        
        // Protecao contra underflow (relogio do sistema alterado)
        if (ullNow.QuadPart > ullFile.QuadPart) {
            ULONGLONG diffTicks = ullNow.QuadPart - ullFile.QuadPart;
            ULONGLONG diffDays = diffTicks / 864000000000ULL;  // 1 dia em ticks
            
            if (diffDays > 30) {
                char fullPath[MAX_PATH * 2];
                PathCombineA(fullPath, windowsPath, "Prefetch");
                PathAppendA(fullPath, findData.cFileName);
                
                if (DeleteFileA(fullPath)) {
                    deleted++;
                }
            }
        }
    } while (FindNextFileA(hFind, &findData));
    
    FindClose(hFind);
    printf("  [OK] %d arquivos .pf antigos removidos\n", deleted);
}

void CleanWindowsCaches() {
    printf("\n## Limpando caches do Windows...\n");
    
    system("net stop wuauserv >nul 2>&1");
    system("del /f /q /s %systemroot%\\SoftwareDistribution\\Download\\* >nul 2>&1");
    system("net start wuauserv >nul 2>&1");
    
    system("del /f /q /s %systemroot%\\Temp\\* >nul 2>&1");
    system("del /f /q /s %temp%\\* >nul 2>&1");
    system("del /f /q /s %systemroot%\\Logs\\CBS\\*.cab >nul 2>&1");
    
    printf("  [OK] Caches do Windows limpos\n");
}

void RunDiagnostics() {
    printf("\n╔════════════════════════════════════════╗\n");
    printf("║         DIAGNOSTICO DO SISTEMA         ║\n");
    printf("╚════════════════════════════════════════╝\n");
    
    MEMORYSTATUSEX mem;
    mem.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&mem);
    
    printf("  RAM Total:      %.2f GB\n", (double)mem.ullTotalPhys / (1024*1024*1024));
    printf("  RAM Disponivel: %.2f GB (%.1f%%)\n",
           (double)mem.ullAvailPhys / (1024*1024*1024),
           (double)mem.ullAvailPhys / mem.ullTotalPhys * 100);
    
    char sysPath[MAX_PATH];
    GetSystemDirectoryA(sysPath, MAX_PATH);
    PathStripToRootA(sysPath);
    
    ULARGE_INTEGER freeBytes, totalBytes;
    if (GetDiskFreeSpaceExA(sysPath, &freeBytes, &totalBytes, NULL)) {
        printf("  Disco %s:        %.1f GB livres de %.1f GB\n",
               sysPath,
               (double)freeBytes.QuadPart / (1024*1024*1024),
               (double)totalBytes.QuadPart / (1024*1024*1024));
    }
}

void RunPostOptimizationDiagnostics() {
    printf("\n╔════════════════════════════════════════╗\n");
    printf("║      DIAGNOSTICO POS-OTIMIZACAO        ║\n");
    printf("╚════════════════════════════════════════╝\n");
    
    MEMORYSTATUSEX mem;
    mem.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&mem);
    
    double totalGB = (double)mem.ullTotalPhys / (1024*1024*1024);
    double availableGB = (double)mem.ullAvailPhys / (1024*1024*1024);
    double percent = (double)mem.ullAvailPhys / mem.ullTotalPhys * 100;
    
    printf("  RAM Disponivel AGORA: %.2f GB (%.1f%% de %.2f GB)\n",
           availableGB, percent, totalGB);
    
    if (percent > 25) {
        printf("  ✅ Memoria em nivel excelente!\n");
    } else if (percent > 15) {
        printf("  ⚠️  Memoria em nivel aceitavel\n");
    } else {
        printf("  🔴 CRITICO: Reinicie e execute novamente\n");
    }
}

BOOL IsUserAdmin() {
    BOOL isAdmin = FALSE;
    PSID adminGroup = NULL;
    SID_IDENTIFIER_AUTHORITY ntAuth = SECURITY_NT_AUTHORITY;
    
    if (AllocateAndInitializeSid(&ntAuth, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                 DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &adminGroup)) {
        CheckTokenMembership(NULL, adminGroup, &isAdmin);
        FreeSid(adminGroup);
    }
    return isAdmin;
}

// ==================== MAIN ====================
int main() {
    SetConsoleOutputCP(CP_UTF8);
    
    printf("╔══════════════════════════════════════════════════════════╗\n");
    printf("║    Z-OPTIMIZER v3.2.2 - HARDENED EDITION                 ║\n");
    printf("║    Kernel Purge + Services + Registry + Anti-Underflow   ║\n");
    printf("╚══════════════════════════════════════════════════════════╝\n");
    
    if (!IsUserAdmin()) {
        printf("\n❌ ERRO: Execute como Administrador!\n");
        printf("   Clique com botao direito -> Executar como administrador\n\n");
        system("pause");
        return 1;
    }
    
    RunDiagnostics();
    
    printf("\n▶ Iniciando otimizacao...\n");
    time_t start = time(NULL);
    
    CleanWindowsCaches();
    CleanBrowserCaches();
    CleanThumbnailCache();
    SmartPrefetchCleanup();
    OptimizeMemoryUsage();
    OptimizeWindowsServices();
    OptimizeWindowsRegistry();
    PurgeStandbyList();
    
    system("ipconfig /flushdns >nul 2>&1");
    
    time_t end = time(NULL);
    
    RunPostOptimizationDiagnostics();
    
    printf("\n╔══════════════════════════════════════════════════════════╗\n");
    printf("║  ✅ OTIMIZACAO CONCLUIDA em %ld segundos                   ║\n", (long)(end-start));
    printf("║  RECOMENDACAO: Reiniciar o sistema para aplicar tudo      ║\n");
    printf("╚══════════════════════════════════════════════════════════╝\n");
    
    system("pause");
    return 0;
}
