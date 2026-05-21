// Define que o alvo é o Windows Vista ou superior para liberar o GetTickCount64 no MinGW
#define _WIN32_WINNT 0x0600 

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <shlobj.h>
#include <shlwapi.h>  
#include <tlhelp32.h>
#include <psapi.h>     
#include <time.h>

#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "psapi.lib")

void LogError(const char* operation, DWORD errorCode) {
    char* errorMsg = NULL;
    FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                   NULL, errorCode, 0, (LPSTR)&errorMsg, 0, NULL);
    printf("  [ERRO] %s: %s (Código: %lu)\n", operation, errorMsg, errorCode);
    LocalFree(errorMsg);
}

BOOL IsUserAdmin() {
    BOOL isAdmin = FALSE;
    PSID adminGroup = NULL;
    SID_IDENTIFIER_AUTHORITY ntAuth = SECURITY_NT_AUTHORITY;
    
    // CORREÇÃO: Alterado de DOMAIN_ALIAS_ADMINS para DOMAIN_ALIAS_RID_ADMINS
    if (AllocateAndInitializeSid(&ntAuth, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                 DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &adminGroup)) {
        CheckTokenMembership(NULL, adminGroup, &isAdmin);
        FreeSid(adminGroup);
    }
    return isAdmin;
}

void CleanBrowserCaches() {
    printf("\n## Limpando caches de navegadores...\n");
    
    const char* browserPaths[] = {
        "%localappdata%\\Google\\Chrome\\User Data\\Default\\Cache",
        "%localappdata%\\Microsoft\\Edge\\User Data\\Default\\Cache",
        "%appdata%\\Mozilla\\Firefox\\Profiles\\*\\cache2",
        "%localappdata%\\BraveSoftware\\Brave-Browser\\User Data\\Default\\Cache"
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
    Sleep(1000); 
    
    char cmd[512];
    snprintf(cmd, sizeof(cmd), 
             "del /f /q \"%s\\AppData\\Local\\Microsoft\\Windows\\Explorer\\thumbcache_*.db\" >nul 2>&1",
             getenv("USERPROFILE"));
    system(cmd);
    
    // CORREÇÃO: Ajustado o literal de string para ANSI e alocado buffer de saída correto
    char pathBuffer[MAX_PATH];
    SHGetFolderPathAndSubDirA(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, 
                             "Microsoft\\Windows\\Explorer", pathBuffer);
    
    system("start explorer.exe");
    Sleep(500);
    printf("  [OK] Cache de miniaturas resetado (Explorer reiniciado)\n");
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
        printf("  [INFO] Nenhum arquivo .pf encontrado (pasta vazia ou sem acesso)\n");
        return;
    }
    
    SYSTEMTIME stNow;
    GetSystemTime(&stNow);
    int deleted = 0;
    
    do {
        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
        
        FILETIME ftLocal;
        FileTimeToLocalFileTime(&findData.ftLastWriteTime, &ftLocal);
        
        SYSTEMTIME stFile;
        FileTimeToSystemTime(&ftLocal, &stFile);
        
        int daysDiff = (stNow.wYear - stFile.wYear) * 365 + 
                       (stNow.wMonth - stFile.wMonth) * 30 + 
                       (stNow.wDay - stFile.wDay);
        
        if (daysDiff > 30) {
            char fullPath[MAX_PATH * 2];
            PathCombineA(fullPath, windowsPath, "Prefetch");
            PathAppendA(fullPath, findData.cFileName);
            
            if (DeleteFileA(fullPath)) {
                deleted++;
            }
        }
    } while (FindNextFileA(hFind, &findData));
    
    FindClose(hFind);
    printf("  [OK] %d arquivos .pf antigos removidos\n", deleted);
}

void OptimizeMemoryUsage() {
    printf("\n## Trimming de Working Set em processos não-críticos...\n");
    
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        LogError("CreateToolhelp32Snapshot", GetLastError());
        return;
    }
    
    PROCESSENTRY32W pe;
    pe.dwSize = sizeof(PROCESSENTRY32W);
    
    const wchar_t* criticalProcesses[] = {
        L"System", L"Registry", L"csrss.exe", L"lsass.exe", 
        L"winlogon.exe", L"services.exe", L"svchost.exe"
    };
    
    int optimized = 0;
    
    if (Process32FirstW(hSnapshot, &pe)) {
        do {
            BOOL isCritical = FALSE;
            for (size_t i = 0; i < sizeof(criticalProcesses)/sizeof(criticalProcesses[0]); i++) {
                if (_wcsicmp(pe.szExeFile, criticalProcesses[i]) == 0) {
                    isCritical = TRUE;
                    break;
                }
            }
            
            if (isCritical) continue;
            
            HANDLE hProcess = OpenProcess(PROCESS_SET_QUOTA, FALSE, pe.th32ProcessID);
            if (hProcess) {
                if (EmptyWorkingSet(hProcess)) {
                    optimized++;
                }
                CloseHandle(hProcess);
            }
        } while (Process32NextW(hSnapshot, &pe));
    }
    
    CloseHandle(hSnapshot);
    printf("  [OK] Working set otimizado para %d processos\n", optimized);
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
    printf("║         DIAGNÓSTICO DO SISTEMA         ║\n");
    printf("╚════════════════════════════════════════╝\n");
    
    MEMORYSTATUSEX mem;
    mem.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&mem);
    
    printf("  RAM Total:      %.2f GB\n", (double)mem.ullTotalPhys / (1024*1024*1024));
    printf("  RAM Disponível: %.2f GB (%.1f%%)\n", 
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
    
    // CORREÇÃO: Agora reconhecido pelo compilador por conta do _WIN32_WINNT
    ULONGLONG uptimeMS = GetTickCount64();
    DWORD uptimeHours = (DWORD)(uptimeMS / 1000 / 3600);
    if (uptimeHours > 48) {
        printf("\n  [!] AVISO: Sistema ligado há %d horas. Reiniciar pode ajudar!\n", uptimeHours);
    }
}

int main() {
    SetConsoleOutputCP(CP_UTF8);
    
    printf("╔══════════════════════════════════════════════════════════╗\n");
    printf("║      Z-OPTIMIZER v3.0.1 - PRODUCTION READY (MinGW)       ║\n");
    printf("║      Otimizador de Sistema em C para Windows             ║\n");
    printf("╚══════════════════════════════════════════════════════════╝\n");
    
    if (!IsUserAdmin()) {
        printf("\n❌ ERRO: Execute como Administrador!\n");
        printf("   Clique com botão direito -> Executar como administrador\n\n");
        system("pause");
        return 1;
    }
    
    RunDiagnostics();
    
    printf("\n▶ Iniciando otimização...\n");
    time_t start = time(NULL);
    
    CleanWindowsCaches();
    CleanBrowserCaches();
    CleanThumbnailCache();
    SmartPrefetchCleanup();
    OptimizeMemoryUsage();
    
    system("ipconfig /flushdns >nul 2>&1");
    
    time_t end = time(NULL);
    
    printf("\n╔══════════════════════════════════════════════════════════╗\n");
    printf("║  ✅ OTIMIZAÇÃO CONCLUÍDA em %ld segundos                    ║\n", (long)(end-start));
    printf("║  Recomendação: Reiniciar o sistema para aplicar tudo       ║\n");
    printf("╚══════════════════════════════════════════════════════════╝\n");
    
    system("pause");
    return 0;
}
