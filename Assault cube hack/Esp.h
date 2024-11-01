#pragma once
#include <d2d1_1.h>
#include <vector>
#include "Memory reader.h"
#include "Directx.h"
#include "Utils.h"
#include "Globals.h"

void RenderESP(ID2D1DeviceContext* pRenderTarget, ID2D1SolidColorBrush* pBrush, const viewMatrix& viewMatrix, const std::vector<Player>& players, int screenWidth, int screenHeight, float cameraYaw, float cameraPitch);
