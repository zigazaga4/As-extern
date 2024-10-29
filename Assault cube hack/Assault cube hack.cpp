#include <Windows.h>
#include <iostream>
#include "Directx.h"
#include "Memory reader.h"
#include "Utils.h"
#include <conio.h> // For _getch()

int main() {
    const wchar_t* procName = L"ac_client.exe";
    DWORD procId = GetProcId(procName);

    if (procId == 0) {
        std::cerr << "AssaultCube process not found." << std::endl;
        return 1;
    }

    HANDLE hProcess = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, procId);
    if (!hProcess) {
        std::cerr << "Failed to open process. Error code: " << GetLastError() << std::endl;
        return 1;
    }

    uintptr_t moduleBase = GetModuleBaseAddress(procId, procName);
    if (moduleBase == 0) {
        std::cerr << "Failed to get module base address." << std::endl;
        CloseHandle(hProcess);
        return 1;
    }

    std::cout << "Process ID: " << procId << std::endl;
    std::cout << "Module Base Address: " << std::hex << moduleBase << std::dec << std::endl;

    viewMatrix viewMatrix;
    GetViewMatrix(hProcess, viewMatrix);

    std::vector<Player> players = GetPlayerData(hProcess, moduleBase);

    // Read camera angles
    float cameraYaw = GetCameraYaw(hProcess, moduleBase);
    float cameraPitch = GetCameraPitch(hProcess, moduleBase);

    std::vector<Entity> entities;
    ReadEntities(hProcess, moduleBase, entities);

    std::cout << "View matrix, camera angles, player data, and entities retrieved." << std::endl;

    // Close handle to the process
    CloseHandle(hProcess);

    // Create overlay window and render ESP
    HWND hGameWnd = FindWindow(NULL, L"AssaultCube"); // Make sure to replace with the correct game window title
    if (!hGameWnd) {
        std::cerr << "Game window not found." << std::endl;
        return 1;
    }

    // Get game window dimensions
    RECT clientRect;
    GetClientRect(hGameWnd, &clientRect);
    int screenWidth = clientRect.right - clientRect.left;
    int screenHeight = clientRect.bottom - clientRect.top;

    std::cout << "Screen Width: " << screenWidth << ", Screen Height: " << screenHeight << std::endl;

    CreateOverlayWindow(GetModuleHandle(NULL), hGameWnd, viewMatrix, players, screenWidth, screenHeight, cameraYaw, cameraPitch);

    std::cout << "Press any key to exit..." << std::endl;
    _getch(); // Wait for a key press
    return 0;
}