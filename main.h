#pragma once

#include <windows.h>
#include <vector>
#include <fstream>
#include <process.h>
#include <stdint.h>
#include <d3d9.h>
#pragma comment(lib, "d3d9.lib")
#include "DXSDK\d3dx9.h"
#if defined _M_X64
#pragma comment(lib, "DXSDK/x64/d3dx9.lib") 
#elif defined _M_IX86
#pragma comment(lib, "DXSDK/x86/d3dx9.lib")
#endif
#pragma comment(lib, "winmm.lib")
#include "MinHook/include/MinHook.h"
using namespace std;

#pragma warning (disable: 4244)

extern HMODULE dllHandle;

typedef HRESULT(APIENTRY* DrawIndexedPrimitive)(IDirect3DDevice9*, D3DPRIMITIVETYPE, INT, UINT, UINT, UINT, UINT);
typedef HRESULT(APIENTRY* Present) (IDirect3DDevice9*, const RECT*, const RECT*, HWND, const RGNDATA*, UINT);

extern DrawIndexedPrimitive DrawIndexedPrimitive_orig;
extern Present Present_orig;

extern float Smoothing;
extern int FOV_Range;
extern float Horizontal_Correction;
extern float Vertical_Correction;

extern DWORD aimkey;
extern int aimfov;
extern int aimheight;
extern DWORD astime;
extern unsigned int asdelay;

extern bool IsPressed;
extern D3DVIEWPORT9 viewport;
extern bool aimbot;
extern bool DrawModel;
extern unsigned int offsets_in_bytes, stride;
extern LPDIRECT3DVERTEXBUFFER9 pBuffer;

extern bool esp_enabled;
extern bool esp_box;
extern bool esp_line;
extern bool esp_name;
extern float esp_max_distance;
extern D3DCOLOR esp_box_color;
extern D3DCOLOR esp_line_color;
extern D3DCOLOR esp_name_color;

#define GHOSTRED		D3DCOLOR_ARGB(255,175,000,000)
#define GHOSTGREEN		D3DCOLOR_ARGB(255,000,175,000)
#define PURPLE          D3DCOLOR_ARGB(255,159,000,252)
#define BLACK           D3DCOLOR_ARGB(255,000,000,000)
#define YELLOW          D3DCOLOR_ARGB(255,195,250,004)
#define ESP_BOX_COLOR   D3DCOLOR_ARGB(255,000,255,000) 
#define ESP_LINE_COLOR  D3DCOLOR_ARGB(255,255,000,000) 
#define ESP_NAME_COLOR  D3DCOLOR_ARGB(255,255,255,255) 

struct ModelInfo_t
{
	D3DXVECTOR3 Position2D;
	D3DXVECTOR3 Position3D;
	float CrosshairDistance;
	float Distance;
	char Name[32];
};

extern std::vector<ModelInfo_t*> ModelInfo;

void DrawFilledRect(LPDIRECT3DDEVICE9 Device, int x, int y, int w, int h, D3DCOLOR color);
void WorldToScreen(LPDIRECT3DDEVICE9 Device);
void MouseMove(float x, float y);
float GetDistance(float Xx, float Yy, float xX, float yY);
HRESULT GenerateTexture(IDirect3DDevice9* pD3Ddev, IDirect3DTexture9** ppD3Dtex, DWORD colour32);
void DrawLine(LPDIRECT3DDEVICE9 pDevice, float X, float Y, float X2, float Y2, DWORD dwColor);
void aimAtPos(float x, float y);
void FillRGB(int x, int y, int w, int h, D3DCOLOR color, IDirect3DDevice9* pDevice);
void DrawPoint(int x, int y, int w, int h, DWORD color, IDirect3DDevice9* pDevice);
void DrawBox(LPDIRECT3DDEVICE9 pDevice, float x, float y, float width, float height, DWORD color);
void DrawBox(LPDIRECT3DDEVICE9 pDevice, float x, float y, float width, float height, float thickness, D3DCOLOR color);
void DrawESP(LPDIRECT3DDEVICE9 pDevice);

HRESULT APIENTRY Present_hook(IDirect3DDevice9* pDevice, const RECT* pSourceRect, const RECT* pDestRect, HWND hDestWindowOverride, const RGNDATA* pDirtyRegion, UINT a6);
HRESULT APIENTRY DrawIndexedPrimitive_hook(IDirect3DDevice9* pDevice, D3DPRIMITIVETYPE Type, INT BaseVertexIndex, UINT MinVertexIndex, UINT NumVertices, UINT startIndex, UINT primCount);
DWORD WINAPI THR(LPVOID lpParameter);
extern "C" __declspec(dllexport) LRESULT CALLBACK NextHook(int nCode, WPARAM wParam, LPARAM lParam);
BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved);

extern LPDIRECT3DTEXTURE9 texRed, texGreen, texYellow, texBlue, Tex;

extern int aimsens;
extern int aimspeed_isbasedon_distance;
extern float AimSpeed;
extern int aimspeed;
extern int as_xhairdst;