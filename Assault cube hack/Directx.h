// Directx.h
#pragma once
#include <Windows.h>
#include <d2d1.h>
#include <vector>
#include "Memory reader.h"
#include "Utils.h"
#include "Globals.h" // Include the Globals header for global variables

// Function declarations
void InitD3D(HWND hWnd);
void InitD2D();
void RenderFrame();
void CleanD3D();
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
void CreateOverlayWindow(HINSTANCE hInstance, HWND hGameWnd, const viewMatrix& viewMatrix, const std::vector<Player>& players, int screenWidth, int screenHeight, float cameraYaw, float cameraPitch);
void UpdateOverlayWindowSize(HWND hGameWnd);
Vector3 AdjustForCamera(const Vector3& position, float cameraYaw, float cameraPitch);