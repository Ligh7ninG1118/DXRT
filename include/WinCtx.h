#pragma once

#include "stdafx.h"

class DXRenderer;

class WinCtx
{
public:
	static int Run(DXRenderer* pRenderer, HINSTANCE hInstance, int nCmdShow);
	static HWND GetHwnd() { return mHWnd; };

protected:
	static LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
	static HWND mHWnd;

};

