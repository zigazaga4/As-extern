#include "DirectX.h"
#include "Globals.h"
#include <d3d11.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <wrl/client.h>
#include "ESP.h"
#include <iostream>
#include "Memory reader.h"
#include <vector>
#include <windows.h>
#include <d2d1.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

using namespace Microsoft::WRL;

extern std::vector<Player> g_players;
extern viewMatrix g_viewMatrix;
extern float g_cameraYaw;
extern float g_cameraPitch;
extern int g_screenWidth;
extern int g_screenHeight;
extern std::mutex memoryMutex;
extern std::atomic<bool> memoryReaderRunning;

IDXGISwapChain* swapchain = nullptr;
ID3D11Device* dev = nullptr;
ID3D11DeviceContext* devcon = nullptr;
ComPtr<ID3D11RenderTargetView> backbuffer;
ComPtr<ID2D1Factory1> pFactory;
ComPtr<ID2D1DeviceContext> pRenderTarget;
ComPtr<ID2D1SolidColorBrush> pBrush;
ComPtr<IDXGISurface> pBackBuffer;
HWND hOverlayWnd = nullptr;

void InitD3D(HWND hWnd) {
    DXGI_SWAP_CHAIN_DESC scd = {};
    scd.BufferCount = 2; // Double buffering
    scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    scd.BufferDesc.Width = g_screenWidth;
    scd.BufferDesc.Height = g_screenHeight;
    scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    scd.OutputWindow = hWnd;
    scd.SampleDesc.Count = 1;
    scd.Windowed = TRUE;
    scd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD; // Discard for better performance
    scd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        D3D11_CREATE_DEVICE_BGRA_SUPPORT,
        nullptr,
        0,
        D3D11_SDK_VERSION,
        &scd,
        &swapchain,
        &dev,
        nullptr,
        &devcon
    );

    if (FAILED(hr)) {
        return;
    }

    ComPtr<ID3D11Texture2D> pBackBufferTexture;
    hr = swapchain->GetBuffer(0, IID_PPV_ARGS(&pBackBufferTexture));
    if (FAILED(hr)) {
        return;
    }

    hr = dev->CreateRenderTargetView(pBackBufferTexture.Get(), nullptr, backbuffer.GetAddressOf());
    if (FAILED(hr)) {
        return;
    }

    devcon->OMSetRenderTargets(1, backbuffer.GetAddressOf(), nullptr);

    D3D11_VIEWPORT viewport = {};
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    viewport.Width = static_cast<FLOAT>(g_screenWidth);
    viewport.Height = static_cast<FLOAT>(g_screenHeight);

    devcon->RSSetViewports(1, &viewport);

    InitD2D();
}

void InitD2D() {
    HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, pFactory.GetAddressOf());
    if (FAILED(hr)) {
        return;
    }

    hr = swapchain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    if (FAILED(hr)) {
        return;
    }

    D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
        D2D1_RENDER_TARGET_TYPE_DEFAULT,
        D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED),
        0, 0
    );

    ComPtr<ID2D1RenderTarget> pRT;
    hr = pFactory->CreateDxgiSurfaceRenderTarget(pBackBuffer.Get(), &props, &pRT);
    if (FAILED(hr)) {
        return;
    }

    hr = pRT.As(&pRenderTarget);
    if (FAILED(hr)) {
        return;
    }

    hr = pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Red), pBrush.GetAddressOf());
    if (FAILED(hr)) {
        return;
    }
}

void RenderFrame() {
    const float clearColor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    devcon->ClearRenderTargetView(backbuffer.Get(), clearColor);

    if (pRenderTarget) {
        pRenderTarget->BeginDraw();
        pRenderTarget->Clear(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.0f));  // Transparent color

        // Call RenderESP to draw player ESP
        {
            std::lock_guard<std::mutex> lock(memoryMutex);
            RenderESP(pRenderTarget.Get(), pBrush.Get(), g_viewMatrix, g_players, g_screenWidth, g_screenHeight, g_cameraYaw, g_cameraPitch);
        }

        HRESULT hr = pRenderTarget->EndDraw();
    }

    swapchain->Present(0, 0); // Disable vsync for higher FPS
}

void CleanD3D() {
    swapchain->SetFullscreenState(FALSE, NULL);
    swapchain->Release();
    dev->Release();
    devcon->Release();
}

LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_PAINT:
        RenderFrame();
        ValidateRect(hWnd, NULL);
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, message, wParam, lParam);
}

void CreateOverlayWindow(HINSTANCE hInstance, HWND hGameWnd, const viewMatrix& viewMatrix, const std::vector<Player>& players, int screenWidth, int screenHeight, float cameraYaw, float cameraPitch) {
    g_viewMatrix = viewMatrix;
    g_players = players;
    g_screenWidth = screenWidth;
    g_screenHeight = screenHeight;
    g_cameraYaw = cameraYaw;
    g_cameraPitch = cameraPitch;

    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
    wc.lpszClassName = L"Overlay";

    RegisterClassEx(&wc);

    RECT rect;
    GetWindowRect(hGameWnd, &rect);

    hOverlayWnd = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TRANSPARENT,
        wc.lpszClassName,
        L"Overlay",
        WS_POPUP,
        rect.left, rect.top,
        screenWidth, screenHeight,
        NULL, NULL, hInstance, NULL
    );

    SetLayeredWindowAttributes(hOverlayWnd, 0, 255, LWA_ALPHA);
    SetLayeredWindowAttributes(hOverlayWnd, RGB(0, 0, 0), 0, LWA_COLORKEY);

    InitD3D(hOverlayWnd);

    ShowWindow(hOverlayWnd, SW_SHOW);

    MSG msg = {};
    DWORD procId = GetProcId(L"ac_client.exe");
    HANDLE hProcess = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, procId);
    uintptr_t moduleBase = GetModuleBaseAddress(procId, L"ac_client.exe");

    StartMemoryReaderThread(hProcess, moduleBase);  // Start the memory reader thread

    // Increase the priority of the current thread to ensure smoother rendering
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

    while (msg.message != WM_QUIT) {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        RenderFrame(); // Continuously render the frame
    }

    StopMemoryReaderThread();  // Stop the memory reader thread

    CloseHandle(hProcess);
    CleanD3D();
}
