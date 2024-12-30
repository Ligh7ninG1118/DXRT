#include "stdafx.h"


LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{

    switch (uMsg)
    {

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return 0;
}


int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    WNDCLASSEX wndClass = { 0 };
    wndClass.cbSize = sizeof(WNDCLASSEX);
    wndClass.style = CS_HREDRAW | CS_VREDRAW;
    wndClass.lpfnWndProc = WindowProc;
    wndClass.hInstance = hInstance;
    wndClass.hCursor = LoadCursor(hInstance, IDC_ARROW);
    wndClass.lpszClassName = L"DXRT";
    RegisterClassEx(&wndClass);

    RECT wndRect = { 0, 0, 1920, 1080 };
    AdjustWindowRect(&wndRect, WS_OVERLAPPEDWINDOW, false);

    HWND hWnd = CreateWindow(
        wndClass.lpszClassName,
        L"DXRT",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        wndRect.right - wndRect.left,
        wndRect.bottom - wndRect.top,
        nullptr,
        nullptr,
        hInstance,
        nullptr
    );

    ShowWindow(hWnd, nCmdShow);

    MSG msg = {};
    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {

        }
    }

    return 0;
}
