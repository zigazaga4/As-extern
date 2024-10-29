#pragma once

#include "Vector3.h"
#include "Vector4.h"
#include "Vector2.h"

struct viewMatrix {
    float matrix[16];
};

bool worldToScreen(const Vector3& pos, Vector2& screen, const viewMatrix& matrix, int windowWidth, int windowHeight);