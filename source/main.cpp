#include "stdafx.h"
#include "WinCtx.h"
#include "DXRenderer.h"


_Use_decl_annotations_
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    DXRenderer renderer(1920, 1080, L"DXRT");
    return WinCtx::Run(&renderer, hInstance, nCmdShow);
}
