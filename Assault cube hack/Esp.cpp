#include "ESP.h"
#include "Utils.h"
#include <d2d1_1.h>
#include <vector>
#include "Memory reader.h"
#include <iostream>
#include "Vector2.h"

void RenderESP(ID2D1DeviceContext* pRenderTarget, ID2D1SolidColorBrush* pBrush, const viewMatrix& viewMatrix, const std::vector<Player>& players, int screenWidth, int screenHeight, float cameraYaw, float cameraPitch) {
    int mainPlayerTeam = -1;
    if (!players.empty()) {
        mainPlayerTeam = players[0].team;
    }

    for (const auto& player : players) {
        if (!player.isAlive) continue;

        Vector2 screenPosHead;
        Vector2 screenPosFeet;

        if (worldToScreen(player.head, screenPosHead, viewMatrix, screenWidth, screenHeight) &&
            worldToScreen(player.feet, screenPosFeet, viewMatrix, screenWidth, screenHeight)) {

            float boxHeight = screenPosFeet.y - screenPosHead.y;
            float boxWidth = boxHeight / 2.0f;

            if (boxHeight > 0 && boxWidth > 0) {
                D2D1_RECT_F rect = D2D1::RectF(screenPosHead.x - boxWidth / 2, screenPosHead.y, screenPosHead.x + boxWidth / 2, screenPosFeet.y);

                if (player.team == mainPlayerTeam) {
                    pBrush->SetColor(D2D1::ColorF(D2D1::ColorF::Blue));
                }
                else {
                    pBrush->SetColor(D2D1::ColorF(D2D1::ColorF::Red));
                }

                pRenderTarget->DrawRectangle(rect, pBrush, 2.0f);
            }
        }
    }
}