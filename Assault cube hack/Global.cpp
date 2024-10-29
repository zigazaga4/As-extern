// Globals.cpp
#include "Globals.h"

viewMatrix g_viewMatrix;
std::vector<Player> g_players;
float g_cameraYaw;
float g_cameraPitch;
int g_screenWidth;
int g_screenHeight;
std::mutex memoryMutex;
std::atomic<bool> memoryReaderRunning(false);