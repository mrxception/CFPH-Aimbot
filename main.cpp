#include "main.h"

float Smoothing = 4.0f;          
int FOV_Range = 85;              
float Horizontal_Correction = 2.0f; float Vertical_Correction = 0.0f;   
void DrawCircle(IDirect3DDevice9* pDevice, float x, float y, float radius, int sides, D3DCOLOR color)
{
    float Angle = (3.14159265f * 2.0f) / sides;
    float _Cos = cos(Angle);
    float _Sin = sin(Angle);
    float x1 = radius, y1 = 0, x2, y2;

    for (int i = 0; i < sides; i++)
    {
        x2 = _Cos * x1 - _Sin * y1;
        y2 = _Sin * x1 + _Cos * y1;

                D3DRECT rect = { (long)(x + x1), (long)(y + y1), (long)(x + x1 + 2), (long)(y + y1 + 2) };
        pDevice->Clear(1, &rect, D3DCLEAR_TARGET, color, 0, 0);

        x1 = x2;
        y1 = y2;
    }
}

HRESULT APIENTRY Present_hook(IDirect3DDevice9* pDevice, const RECT* pSourceRect, const RECT* pDestRect, HWND hDestWindowOverride, const RGNDATA* pDirtyRegion, UINT a6)
{
    if (GetAsyncKeyState(VK_F9) & 1)
        aimbot = !aimbot;

    pDevice->GetViewport(&viewport);
    float ScreenCenterX = viewport.Width / 2.0f;
    float ScreenCenterY = viewport.Height / 2.0f;

    if (aimbot) {
        DrawCircle(pDevice, ScreenCenterX, ScreenCenterY, (float)FOV_Range, 32, YELLOW);
    }

    if (aimbot && !ModelInfo.empty())
    {
        int BestTarget = -1;
        float fClosestPos = (float)FOV_Range;

        for (unsigned int i = 0; i < ModelInfo.size(); i++)
        {
            float dist = GetDistance(ModelInfo[i]->Position2D.x, ModelInfo[i]->Position2D.y, ScreenCenterX, ScreenCenterY);
            if (dist < fClosestPos)
            {
                fClosestPos = dist;
                BestTarget = i;
            }
        }

        if (BestTarget != -1 && GetAsyncKeyState(VK_SHIFT) < 0)
        {
            float TargetX = (ModelInfo[BestTarget]->Position2D.x + Horizontal_Correction) - ScreenCenterX;
            float TargetY = (ModelInfo[BestTarget]->Position2D.y + Vertical_Correction) - ScreenCenterY;

            MouseMove(TargetX / Smoothing, TargetY / Smoothing);
        }

    for (auto p : ModelInfo) delete p;
        ModelInfo.clear();
    }

    return Present_orig(pDevice, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion, a6);
}

HRESULT APIENTRY DrawIndexedPrimitive_hook(IDirect3DDevice9* pDevice, D3DPRIMITIVETYPE Type, INT BaseVertexIndex, UINT MinVertexIndex, UINT NumVertices, UINT startIndex, UINT primCount)
{
    if (pDevice->GetStreamSource(0, &pBuffer, &offsets_in_bytes, &stride) == D3D_OK)
    {
        pBuffer->Release();
    }

     //if (stride == 36 || stride == 32 || stride == 44 || stride == 68 || stride == 40 || stride == 56)
    if (stride == 32)
    {
        if (texGreen == NULL) GenerateTexture(pDevice, &texGreen, GHOSTGREEN);

            pDevice->SetRenderState(D3DRS_ZENABLE, FALSE);         
            pDevice->SetTexture(0, texGreen);             
            DrawIndexedPrimitive_orig(pDevice, Type, BaseVertexIndex, MinVertexIndex, NumVertices, startIndex, primCount);

            pDevice->SetRenderState(D3DRS_ZENABLE, TRUE);

        if (aimbot)
        {
            DWORD zEnable;
            pDevice->GetRenderState(D3DRS_ZENABLE, &zEnable);

            if (zEnable == D3DZB_TRUE && NumVertices > 900)
            {
                WorldToScreen(pDevice);
            }
        }
    }

    return DrawIndexedPrimitive_orig(pDevice, Type, BaseVertexIndex, MinVertexIndex, NumVertices, startIndex, primCount);
}


DWORD WINAPI THR(LPVOID lpParameter)
{
    Sleep(10000);

    IDirect3D9* d3d = Direct3DCreate9(D3D_SDK_VERSION);
    if (!d3d) return 0;

    HWND tmpWnd = CreateWindowA("BUTTON", "Temp", WS_SYSMENU, 0, 0, 1, 1, NULL, NULL, NULL, NULL);
    D3DPRESENT_PARAMETERS d3dpp = { 0 };
    d3dpp.Windowed = TRUE;
    d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dpp.hDeviceWindow = tmpWnd;
    d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;

    IDirect3DDevice9* d3ddev;
    if (d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, tmpWnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &d3ddev) != D3D_OK)
    {
        d3d->Release();
        DestroyWindow(tmpWnd);
        return 0;
    }

    DWORD64* dVtable = (DWORD64*)(*(DWORD64*)d3ddev);
    Present_orig = (Present)dVtable[17];
    DrawIndexedPrimitive_orig = (DrawIndexedPrimitive)dVtable[82];

    MH_Initialize();
    MH_CreateHook((LPVOID)dVtable[17], (LPVOID)&Present_hook, (LPVOID*)&Present_orig);
    MH_CreateHook((LPVOID)dVtable[82], (LPVOID)&DrawIndexedPrimitive_hook, (LPVOID*)&DrawIndexedPrimitive_orig);
    MH_EnableHook(MH_ALL_HOOKS);

    d3ddev->Release();
    d3d->Release();
    DestroyWindow(tmpWnd);
    return 1;
}

extern "C" __declspec(dllexport) LRESULT CALLBACK NextHook(int nCode, WPARAM wParam, LPARAM lParam) {
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        dllHandle = hModule;
        DisableThreadLibraryCalls(hModule);
        CreateThread(0, 0, (LPTHREAD_START_ROUTINE)THR, 0, 0, 0);
    }
    return TRUE;
}