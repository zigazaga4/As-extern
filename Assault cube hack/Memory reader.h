#pragma once
#include <Windows.h>
#include <TlHelp32.h>
#include <vector>
#include <array>
#include <mutex>  // Add this include for the mutex
#include <atomic>  // Add this include for atomic

#include "Utils.h"

struct Player {
    Vector3 position;  // Player position
    Vector3 head;      // Player head position
    Vector3 feet;      // Player feet position
    int health = 0;    // Player health, initialize to 0
    bool isAlive = false; // Player alive status, initialize to false
    int team = -1;     // Player team, initialize to an invalid team (-1)
};

struct Entity {
    uintptr_t base = 0;
    char name[20] = {};
    int health = 0;
    Vector3 position_head;
    Vector3 position_feet;
    int team = 0;
};

DWORD GetProcId(const wchar_t* procName);
uintptr_t GetModuleBaseAddress(DWORD procId, const wchar_t* modName);

template <typename T>
T ReadMemory(HANDLE hProcess, uintptr_t address);

float GetCameraYaw(HANDLE hProcess, uintptr_t moduleBase);
float GetCameraPitch(HANDLE hProcess, uintptr_t moduleBase);

uintptr_t ReadPointer(HANDLE hProcess, uintptr_t address);
void GetViewMatrix(HANDLE hProcess, viewMatrix& viewMatrix);
std::vector<Player> GetPlayerData(HANDLE hProcess, uintptr_t moduleBase);
void ReadPlayerData(HANDLE hProcess, uintptr_t entityListBase, int playerIndex, std::vector<Player>& players);
void ReadEntities(HANDLE hProcess, uintptr_t moduleBase, std::vector<Entity>& entities);

void StartMemoryReaderThread(HANDLE hProcess, uintptr_t moduleBase);
void StopMemoryReaderThread();
