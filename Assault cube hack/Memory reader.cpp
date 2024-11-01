#include "Memory reader.h"
#include "Globals.h"
#include <iostream>
#include <Windows.h>
#include <TlHelp32.h>
#include <vector>
#include <mutex>
#include <thread>
#include <atomic>


DWORD GetProcId(const wchar_t* procName) {
    DWORD procId = 0;
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap != INVALID_HANDLE_VALUE) {
        PROCESSENTRY32 procEntry;
        procEntry.dwSize = sizeof(procEntry);
        if (Process32First(hSnap, &procEntry)) {
            do {
                if (!_wcsicmp(procEntry.szExeFile, procName)) {
                    procId = procEntry.th32ProcessID;
                    break;
                }
            } while (Process32Next(hSnap, &procEntry));
        }
    }
    CloseHandle(hSnap);
    return procId;
}

uintptr_t GetModuleBaseAddress(DWORD procId, const wchar_t* modName) {
    uintptr_t modBaseAddr = 0;
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, procId);
    if (hSnap != INVALID_HANDLE_VALUE) {
        MODULEENTRY32 modEntry;
        modEntry.dwSize = sizeof(modEntry);
        if (Module32First(hSnap, &modEntry)) {
            do {
                if (!_wcsicmp(modEntry.szModule, modName)) {
                    modBaseAddr = (uintptr_t)modEntry.modBaseAddr;
                    break;
                }
            } while (Module32Next(hSnap, &modEntry));
        }
    }
    CloseHandle(hSnap);
    return modBaseAddr;
}

template <typename T>
T ReadMemory(HANDLE hProcess, uintptr_t address) {
    T buffer;
    if (!ReadProcessMemory(hProcess, (LPCVOID)address, &buffer, sizeof(T), NULL)) {
        return 0;
    }
    return buffer;
}

uintptr_t ReadPointer(HANDLE hProcess, uintptr_t address) {
    DWORD buffer = 0;
    if (!ReadProcessMemory(hProcess, (LPCVOID)address, &buffer, sizeof(DWORD), NULL)) {
        return 0;
    }
    return static_cast<uintptr_t>(buffer);
}

void GetViewMatrix(HANDLE hProcess, viewMatrix& viewMatrix) {
    uintptr_t baseAddress = 0x0057DFD0;     for (int i = 0; i < 16; ++i) {
        viewMatrix.matrix[i] = ReadMemory<float>(hProcess, baseAddress + i * sizeof(float));
    }
}

void ReadPlayerData(HANDLE hProcess, uintptr_t entityListBase, int playerIndex, std::vector<Player>& players) {
    uintptr_t playerBasePtr = entityListBase + (playerIndex * sizeof(DWORD));     uintptr_t playerBase = ReadPointer(hProcess, playerBasePtr);

    if (playerBase == 0) {
        return;
    }

    Player player;

        player.health = ReadMemory<int>(hProcess, playerBase + 0xEC);
    player.isAlive = player.health > 0;

        player.position.x = ReadMemory<float>(hProcess, playerBase + 0x4);
    player.position.y = ReadMemory<float>(hProcess, playerBase + 0x8);
    player.position.z = ReadMemory<float>(hProcess, playerBase + 0xC);

        player.head.x = ReadMemory<float>(hProcess, playerBase + 0x4);
    player.head.y = ReadMemory<float>(hProcess, playerBase + 0x8);
    player.head.z = ReadMemory<float>(hProcess, playerBase + 0xC);

    player.feet.x = ReadMemory<float>(hProcess, playerBase + 0x4);
    player.feet.y = ReadMemory<float>(hProcess, playerBase + 0x8);
    player.feet.z = ReadMemory<float>(hProcess, playerBase + 0x30);

        player.team = ReadMemory<int>(hProcess, playerBase + 0x30C);

    if (player.isAlive) {
        players.push_back(player);
    }
}

std::vector<Player> GetPlayerData(HANDLE hProcess, uintptr_t moduleBase) {
    std::vector<Player> players;
    uintptr_t entityListBaseAddress = moduleBase + 0x0018AC04;
    uintptr_t entityListBase = ReadPointer(hProcess, entityListBaseAddress);

    if (entityListBase == 0) {
        return players;
    }

    for (int i = 0; i < 31; ++i) {
        ReadPlayerData(hProcess, entityListBase, i, players);
    }

    return players;
}

float GetCameraYaw(HANDLE hProcess, uintptr_t moduleBase) {
    uintptr_t cameraYawAddress = moduleBase + 0x0017E0A8;
    return ReadMemory<float>(hProcess, cameraYawAddress + 0x34);
}

float GetCameraPitch(HANDLE hProcess, uintptr_t moduleBase) {
    uintptr_t cameraPitchAddress = moduleBase + 0x0017E0A8;
    return ReadMemory<float>(hProcess, cameraPitchAddress + 0x38);
}

void ReadEntities(HANDLE hProcess, uintptr_t moduleBase, std::vector<Entity>& entities) {
        uintptr_t entityListBase = moduleBase + 0x0018AC04;
    uintptr_t entityBase = ReadPointer(hProcess, entityListBase);

    for (int i = 0; i < 31; ++i) {
        Entity entity;
        entity.base = entityBase + (i * sizeof(DWORD));

        entity.health = ReadMemory<int>(hProcess, entity.base + 0xEC);
        ReadProcessMemory(hProcess, (LPCVOID)(entity.base + 0x225), &entity.name, sizeof(entity.name), nullptr);
        entity.position_head.x = ReadMemory<float>(hProcess, entity.base + 0x4);
        entity.position_head.y = ReadMemory<float>(hProcess, entity.base + 0x8);
        entity.position_head.z = ReadMemory<float>(hProcess, entity.base + 0xC);
        entity.position_feet.x = ReadMemory<float>(hProcess, entity.base + 0x4);
        entity.position_feet.y = ReadMemory<float>(hProcess, entity.base + 0x8);
        entity.position_feet.z = ReadMemory<float>(hProcess, entity.base + 0x30);
        entity.team = ReadMemory<int>(hProcess, entity.base + 0x30C);

        if (entity.health > 0) {
            entities.push_back(entity);
        }
    }
}

void MemoryReaderThread(HANDLE hProcess, uintptr_t moduleBase) {
    while (memoryReaderRunning.load()) {
        {
            std::lock_guard<std::mutex> lock(memoryMutex);
            g_players = GetPlayerData(hProcess, moduleBase);
            GetViewMatrix(hProcess, g_viewMatrix);
            g_cameraYaw = GetCameraYaw(hProcess, moduleBase);
            g_cameraPitch = GetCameraPitch(hProcess, moduleBase);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void StartMemoryReaderThread(HANDLE hProcess, uintptr_t moduleBase) {
    memoryReaderRunning.store(true);
    std::thread(MemoryReaderThread, hProcess, moduleBase).detach();
}

void StopMemoryReaderThread() {
    memoryReaderRunning.store(false);
}
