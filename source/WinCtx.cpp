#include "WinCtx.h"
#include "DXRenderer.h"

HWND WinCtx::mHWnd = nullptr;

int WinCtx::Run(DXRenderer* pRenderer, HINSTANCE hInstance, int nCmdShow)
{
    int argc;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    pRenderer->ParseCommandLineArgs(argv, argc);
    LocalFree(argv);

    WNDCLASSEX wndClass = { 0 };
    wndClass.cbSize = sizeof(WNDCLASSEX);
    wndClass.style = CS_HREDRAW | CS_VREDRAW;
    wndClass.lpfnWndProc = WindowProc;
    wndClass.hInstance = hInstance;
    wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wndClass.lpszClassName = L"DXRTClass";
    RegisterClassEx(&wndClass);

    RECT wndRect = { 0, 0, pRenderer->GetWidth(), pRenderer->GetHeight()};
    AdjustWindowRect(&wndRect, WS_OVERLAPPEDWINDOW, FALSE);

    mHWnd = CreateWindow(
        wndClass.lpszClassName,
        pRenderer->GetTitle(),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        wndRect.right - wndRect.left,
        wndRect.bottom - wndRect.top,
        nullptr,
        nullptr,
        hInstance,
        pRenderer
    );

    pRenderer->OnInit();

    SetProcessDPIAware();
    ShowWindow(mHWnd, nCmdShow);

    MSG msg = {};
    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    pRenderer->OnDestroy();

    return static_cast<char>(msg.wParam);
}

LRESULT WinCtx::WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    DXRenderer* pRenderer = reinterpret_cast<DXRenderer*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

    switch (uMsg)
    {
    case WM_CREATE:
        {
            LPCREATESTRUCT pCreateStruct = reinterpret_cast<LPCREATESTRUCT>(lParam);
            SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pCreateStruct->lpCreateParams));
        }
        return 0;
    case WM_KEYUP:
        if (pRenderer)
        {
            pRenderer->OnKeyUp(static_cast<UINT8>(wParam));
        }
        return 0;
    case WM_KEYDOWN:
        if (pRenderer)
        {
            pRenderer->OnKeyDown(static_cast<UINT8>(wParam));
        }
        return 0;
    case WM_PAINT:
        if (pRenderer)
        {
            pRenderer->OnUpdate();
            pRenderer->OnRender();
        }
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}
