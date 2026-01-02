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
#include "MinHook/include/MinHook.h" //detour
using namespace std;

#pragma warning (disable: 4244)

HMODULE dllHandle;

typedef HRESULT(APIENTRY *DrawIndexedPrimitive)(IDirect3DDevice9*, D3DPRIMITIVETYPE, INT, UINT, UINT, UINT, UINT);
DrawIndexedPrimitive DrawIndexedPrimitive_orig = 0;

typedef HRESULT(APIENTRY* Present) (IDirect3DDevice9*, const RECT *, const RECT *, HWND, const RGNDATA *, UINT);
Present Present_orig = 0;

DWORD aimkey = VK_RBUTTON;	     	//aimkey (google Virtual-Key Codes)
int aimfov = 2;				        //aim field of view in % 
int aimheight = 10;        			//adjust aim height, 0 = feet
DWORD astime = timeGetTime();
unsigned int asdelay = 35;  // def : 49

bool IsPressed = false;
D3DVIEWPORT9 viewport = { 0 };
static bool aimbot = false;
bool DrawModel;
unsigned int offsets_in_bytes, stride;
LPDIRECT3DVERTEXBUFFER9 pBuffer = nullptr;

struct ModelInfo_t
{
	D3DXVECTOR3 Position2D;
	D3DXVECTOR3 Position3D;
	float CrosshairDistance;
};
vector<ModelInfo_t*>ModelInfo;

#define GHOSTRED		D3DCOLOR_ARGB(255,175,000,000)
#define GHOSTGREEN		D3DCOLOR_ARGB(255,000,175,000)
#define PURPLE          D3DCOLOR_ARGB(255,159,000,252)
#define BLACK           D3DCOLOR_ARGB(255,000,000,000)
#define YELLOW          D3DCOLOR_ARGB(255,195,250,004)

void DrawFilledRect(LPDIRECT3DDEVICE9 Device, int x, int y, int w, int h, D3DCOLOR color)
{
	D3DRECT BarRect = { x, y, x + w, y + h };
	Device->Clear(1, &BarRect, D3DCLEAR_TARGET | D3DCLEAR_TARGET, color, 0, 0);
}

//WorldToScreen
void WorldToScreen(LPDIRECT3DDEVICE9 Device)
{
	ModelInfo_t* pModel = new ModelInfo_t;

	Device->GetViewport(&viewport);
	D3DXMATRIX projection, view, world;
	D3DXVECTOR3 vScreenCoord(0, 0, (float)aimheight), vWorldLocation(0, 0, 0);

	Device->GetTransform(D3DTS_VIEW, &view);
	Device->GetTransform(D3DTS_PROJECTION, &projection);
	Device->GetTransform(D3DTS_WORLD, &world);
	D3DXVec3Project(&vScreenCoord, &vWorldLocation, &viewport, &projection, &view, &world);

	if (vScreenCoord.z < 1)
	{
		pModel->Position2D.x = vScreenCoord.x;
		pModel->Position2D.y = vScreenCoord.y;
	}

	ModelInfo.push_back(pModel);
}

void MouseMove(float x, float y)
{
	INPUT Input = { 0 };
	Input.type = INPUT_MOUSE;
	Input.mi.dwFlags = MOUSEEVENTF_MOVE;
	Input.mi.dx = x;
	Input.mi.dy = y;
	SendInput(1, &Input, sizeof(INPUT));
}

int PI2 = 6.283185307179586;
float GetDistance(float Xx, float Yy, float xX, float yY)
{
	return sqrt((yY - Yy) * (yY - Yy) + (xX - Xx) * (xX - Xx));
}

LPDIRECT3DTEXTURE9 texRed, texGreen, texYellow, texBlue, Tex;
HRESULT GenerateTexture(IDirect3DDevice9 *pD3Ddev, IDirect3DTexture9 **ppD3Dtex, DWORD colour32)
{
	if (FAILED(pD3Ddev->CreateTexture(8, 8, 1, 0, D3DFMT_A4R4G4B4, D3DPOOL_MANAGED, ppD3Dtex, NULL)))
		return E_FAIL;

	WORD colour16 = ((WORD)((colour32 >> 28) & 0xF) << 12)
		| (WORD)(((colour32 >> 20) & 0xF) << 8)
		| (WORD)(((colour32 >> 12) & 0xF) << 4)
		| (WORD)(((colour32 >> 4) & 0xF) << 0);

	D3DLOCKED_RECT d3dlr;
	(*ppD3Dtex)->LockRect(0, &d3dlr, 0, 0);
	WORD *pDst16 = (WORD*)d3dlr.pBits;

	for (int xy = 0; xy < 8 * 8; xy++)
		*pDst16++ = colour16;

	(*ppD3Dtex)->UnlockRect(0);

	return S_OK;
}

struct D3DTLVERTEX
{
	float x, y, z, rhw;
	DWORD color;
};

void DrawLine(LPDIRECT3DDEVICE9 pDevice, float X, float Y, float X2, float Y2, DWORD dwColor)
{
	if (!pDevice) return;
	D3DTLVERTEX qV[2] =
	{
		{ (float)X, (float)Y, 0.0f, 1.0f, dwColor },
		{ (float)X2, (float)Y2, 0.0f, 1.0f, dwColor },
	};
	pDevice->SetTexture(0, NULL);
	pDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
	pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
	pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, 1);
	pDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
	pDevice->SetRenderState(D3DRS_ZENABLE, D3DZB_FALSE);
	pDevice->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1);
	pDevice->DrawPrimitiveUP(D3DPT_LINELIST, 1, qV, sizeof(D3DTLVERTEX));

}


int aimsens = 2;
int aimspeed_isbasedon_distance = 1;
float AimSpeed = 0.4f;
int aimspeed = 40;                           //5 slow dont move mouse faster than 5 pixel, 100 fast
int as_xhairdst = 9.0;				 		 //autoshoot activates below this crosshair distance

void aimAtPos(float x, float y)
{
	float TargetX = 0;
	float TargetY = 0;

	float ScreenCenterX = viewport.Width / 2.0f;
	float ScreenCenterY = viewport.Height / 2.01f;

	//X Axis
	if (x != 0)
	{
		if (x > ScreenCenterX)
		{
			TargetX = -(ScreenCenterX - x);
			TargetX /= AimSpeed;
			if (TargetX + ScreenCenterX > ScreenCenterX * 2) TargetX = 0;
		}

		if (x < ScreenCenterX)
		{
			TargetX = x - ScreenCenterX;
			TargetX /= AimSpeed;
			if (TargetX + ScreenCenterX < 0) TargetX = 0;
		}
	}

	//Y Axis
	if (y != 0)
	{
		if (y > ScreenCenterY)
		{
			TargetY = -(ScreenCenterY - y);
			TargetY /= 9.0f;
			if (TargetY + ScreenCenterY > ScreenCenterY * 1.88) TargetY = 0;
		}

		if (y < ScreenCenterY)
		{
			TargetY = y - ScreenCenterY;
			TargetY /= 9.0f;
			if (TargetY + ScreenCenterY < 0) TargetY = 0;
		}
	}

	if (TargetX != 0 && TargetY != 0)
	{
		//dont move mouse more than 50 pixels at time (ghetto HFR)
		float dirX = TargetX > 0 ? 1.0f : -1.0f;
		float dirY = TargetY > 0 ? 1.0f : -1.0f;
		TargetX = dirX * fmin(aimspeed, abs(TargetX));
		TargetY = dirY * fmin(aimspeed, abs(TargetY));
		MouseMove((float)TargetX, (float)TargetY);
	}
}

void FillRGB(int x, int y, int w, int h, D3DCOLOR color, IDirect3DDevice9* pDevice)
{
	D3DRECT rec = { x, y, x + w, y + h };
	pDevice->Clear(1, &rec, D3DCLEAR_TARGET, color, 0, 0);
}

void DrawPoint(int x, int y, int w, int h, DWORD color, IDirect3DDevice9* pDevice)
{
	FillRGB((int)x, (int)y, (int)w, (int)h, color, pDevice);
}