#pragma once

#include <atomic>
#include <vector>
#include <mutex>
#include "Utils.h"
#include "Memory reader.h"

extern viewMatrix g_viewMatrix;
extern std::vector<Player> g_players;
extern float g_cameraYaw;
extern float g_cameraPitch;
extern int g_screenWidth;
extern int g_screenHeight;
extern std::mutex memoryMutex;
extern std::atomic<bool> memoryReaderRunning;
