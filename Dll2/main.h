#include "stdafx.h"
#include <windows.h>
#include <vector>
#include <d3d9.h>
#pragma comment(lib, "d3d9.lib")

//dx sdk 
#include "d3dx9.h"
#if defined _M_X64
#pragma comment(lib, "DXSDK/x64/d3dx9.lib") 
#elif defined _M_IX86
#pragma comment(lib, "d3dx9.lib")
#endif

//DX Includes
#include <DirectXMath.h>
using namespace DirectX;
#pragma comment(lib, "MinHook/bin/MinHook.x86.lib")
#include "MinHook/include/MinHook.h" //detour
using namespace std;

#pragma warning (disable: 4244) //

//==========================================================================================================================

HMODULE Hand;

UINT Stride;

D3DVIEWPORT9 Viewport; //use this Viewport
float ScreenCX;
float ScreenCY;

LPD3DXFONT Font; //font

				 //==========================================================================================================================

				 // Parameters:
				 //
				 //   float4 CameraPos;
				 //   float4 FogInfo;
				 //   float4 PointLightAttr[5];
				 //   float4 ShadowLightAttr[5];
				 //   row_major float4x4 texTrans0;
				 //   row_major float4x4 world;
				 //   row_major float4x4 wvp;
				 //
				 //
				 // Registers:
				 //
				 //   Name            Reg   Size
				 //   --------------- ----- ----
				 //   PointLightAttr  c0       5
				 //   world           c5       4
				 //   ShadowLightAttr c9       4
				 //   wvp             c13      4
				 //   texTrans0       c17      3
				 //   FogInfo         c20      1
				 //   CameraPos       c21      1

				 //calc distance
float GetDistance(float Xx, float Yy, float xX, float yY)
{
	return sqrt((yY - Yy) * (yY - Yy) + (xX - Xx) * (xX - Xx));
}

struct WeaponEspInfo_t
{
	float pOutX, pOutY, distance;
};
std::vector<WeaponEspInfo_t>WeaponEspInfo;

//w2s for weapons
void AddWeapons(LPDIRECT3DDEVICE9 Device)
{
	DirectX::XMVECTOR PosIn = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);

	DirectX::XMMATRIX proj;
	DirectX::XMMATRIX view;
	//DirectX::XMMATRIX world;
	DirectX::XMMATRIX world = DirectX::XMMatrixIdentity();

	Device->GetVertexShaderConstantF(13, (float*)&proj, 4);//13-16
	Device->GetVertexShaderConstantF(8, (float*)&view, 4);//unk
														  //Device->GetVertexShaderConstantF(5, (float*)&world, 4);//unk

														  //distance
	DirectX::XMMATRIX pWorldViewProj = proj * view;
	float distance = PosIn.m128_f32[0] * pWorldViewProj.r[0].m128_f32[3] + PosIn.m128_f32[1] * pWorldViewProj.r[1].m128_f32[3] + PosIn.m128_f32[2] * pWorldViewProj.r[2].m128_f32[3] + pWorldViewProj.r[3].m128_f32[3];

	DirectX::XMVECTOR PosOut = DirectX::XMVector3Project(PosIn, 0, 0, Viewport.Width, Viewport.Height, 0, 1, proj, view, world); //normal

	DirectX::XMFLOAT4 PosOut2d;
	DirectX::XMStoreFloat4(&PosOut2d, PosOut);

	WeaponEspInfo_t pWeaponEspInfo = { static_cast<float>(PosOut2d.x), static_cast<float>(PosOut2d.y), static_cast<float>(distance) };
	WeaponEspInfo.push_back(pWeaponEspInfo);
}

//==========================================================================================================================

void DrawBox(IDirect3DDevice9 *pDevice, float x, float y, float w, float h, D3DCOLOR Color)
{
	struct Vertex
	{
		float x, y, z, ht;
		DWORD Color;
	}
	V[4] = { { x, y + h, 0.0f, 0.0f, Color },{ x, y, 0.0f, 0.01f, Color },
	{ x + w, y + h, 0.0f, 0.0f, Color },{ x + w, y, 0.0f, 0.0f, Color } };
	pDevice->SetTexture(0, NULL);
	pDevice->SetPixelShader(0);
	pDevice->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE);
	pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, true);
	pDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
	pDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
	pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	pDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, V, sizeof(Vertex));
}

HRESULT DrawString(LPD3DXFONT Font, INT X, INT Y, DWORD dColor, CONST PCHAR cString, ...)
{
	HRESULT hRet;

	CHAR buf[512] = { NULL };
	va_list ArgumentList;
	va_start(ArgumentList, cString);
	_vsnprintf_s(buf, sizeof(buf), sizeof(buf) - strlen(buf), cString, ArgumentList);
	va_end(ArgumentList);

	RECT rc[2];
	SetRect(&rc[0], X, Y, X, 0);
	SetRect(&rc[1], X, Y, X + 50, 50);

	hRet = D3D_OK;

	if (SUCCEEDED(hRet))
	{
		Font->DrawTextA(NULL, buf, -1, &rc[0], DT_NOCLIP, 0xFF000000);
		hRet = Font->DrawTextA(NULL, buf, -1, &rc[1], DT_NOCLIP, dColor);
	}

	return hRet;
}

class D3DTLVERTEX
{
public:
	FLOAT X, Y, X2, Y2;
	DWORD Color;
};

void DrawLine(IDirect3DDevice9* m_pD3Ddev, float X, float Y, float X2, float Y2, float Width, D3DCOLOR Color)
{
	D3DTLVERTEX qV[2] = {
		{ (float)X , (float)Y, 0.0f, 1.0f, Color },
	{ (float)X2 , (float)Y2 , 0.0f, 1.0f, Color },
	};
	const DWORD D3DFVF_TL = D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1;
	m_pD3Ddev->SetRenderState(D3DRS_ALPHABLENDENABLE, true);
	m_pD3Ddev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
	m_pD3Ddev->SetFVF(D3DFVF_TL);
	m_pD3Ddev->SetTexture(0, NULL);
	m_pD3Ddev->DrawPrimitiveUP(D3DPT_LINELIST, 2, qV, sizeof(D3DTLVERTEX));
}