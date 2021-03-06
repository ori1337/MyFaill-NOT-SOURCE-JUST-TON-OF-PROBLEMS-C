/*
* Ros D3D by n7

How to compile:
- compile with visual studio community 2017 (..\Microsoft Visual Studio\2017\Community\Common7\IDE\devenv.exe)
- select Release x86
- click: project -> properties -> configuration properties -> general -> character set -> change to "not set"
- compile with CTRL + Shift + B

Optional: remove dependecy on vs runtime:
- click: project -> properties -> configuration properties -> C/C++ -> code generation -> runtime library: Multi-threaded (/MT)
*/
#include "stdafx.h"
#include "main.h" //less important stuff & helper funcs here

typedef HRESULT(APIENTRY *SetStreamSource)(IDirect3DDevice9*, UINT, IDirect3DVertexBuffer9*, UINT, UINT);
HRESULT APIENTRY SetStreamSource_hook(IDirect3DDevice9*, UINT, IDirect3DVertexBuffer9*, UINT, UINT);
SetStreamSource SetStreamSource_orig = 0;

typedef HRESULT(APIENTRY *DrawIndexedPrimitive)(IDirect3DDevice9*, D3DPRIMITIVETYPE, INT, UINT, UINT, UINT, UINT);
HRESULT APIENTRY DrawIndexedPrimitive_hook(IDirect3DDevice9*, D3DPRIMITIVETYPE, INT, UINT, UINT, UINT, UINT);
DrawIndexedPrimitive DrawIndexedPrimitive_orig = 0;

typedef HRESULT(APIENTRY* EndScene) (IDirect3DDevice9*);
HRESULT APIENTRY EndScene_hook(IDirect3DDevice9*);
EndScene EndScene_orig = 0;

typedef HRESULT(APIENTRY *Reset)(IDirect3DDevice9*, D3DPRESENT_PARAMETERS*);
HRESULT APIENTRY Reset_hook(IDirect3DDevice9*, D3DPRESENT_PARAMETERS*);
Reset Reset_orig = 0;

//==========================================================================================================================

HRESULT APIENTRY SetStreamSource_hook(LPDIRECT3DDEVICE9 pDevice, UINT StreamNumber, IDirect3DVertexBuffer9* pStreamData, UINT OffsetInBytes, UINT sStride)
{
	if (StreamNumber == 0)
		Stride = sStride;

	return SetStreamSource_orig(pDevice, StreamNumber, pStreamData, OffsetInBytes, sStride);
}

//==========================================================================================================================

HRESULT APIENTRY DrawIndexedPrimitive_hook(IDirect3DDevice9* pDevice, D3DPRIMITIVETYPE Type, INT BaseVertexIndex, UINT MinVertexIndex, UINT NumVertices, UINT startIndex, UINT primCount)
{
	//wallhack
	if (Stride == 48) //weapons
	{
		pDevice->SetRenderState(D3DRS_ZENABLE, false);
		DrawIndexedPrimitive_orig(pDevice, Type, BaseVertexIndex, MinVertexIndex, NumVertices, startIndex, primCount);
		pDevice->SetRenderState(D3DRS_ZENABLE, true);
	}

	//weapon esp
	if (Stride == 48) //weapons
		AddWeapons(pDevice);

	return DrawIndexedPrimitive_orig(pDevice, Type, BaseVertexIndex, MinVertexIndex, NumVertices, startIndex, primCount);
}

//==========================================================================================================================

HRESULT APIENTRY EndScene_hook(IDirect3DDevice9* pDevice)
{
	//get viewport
	pDevice->GetViewport(&Viewport);
	ScreenCX = (float)Viewport.Width / 2.0f;
	ScreenCY = (float)Viewport.Height / 2.0f;

	//create font
	if (Font == NULL)
		D3DXCreateFont(pDevice, 14, 0, FW_BOLD, 0, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, TEXT("Italic"), &Font);

	//do esp
	if (WeaponEspInfo.size() != NULL)
	{
		for (unsigned int i = 0; i < WeaponEspInfo.size(); i++)
		{
			//box esp
			if (WeaponEspInfo[i].pOutX > 1 && WeaponEspInfo[i].pOutY > 1)
				DrawBox(pDevice, (int)WeaponEspInfo[i].pOutX, (int)WeaponEspInfo[i].pOutY, 6.0f, 12.0f, D3DCOLOR_ARGB(120, 30, 200, 200));

			//line esp
			if (WeaponEspInfo[i].pOutX > 1 && WeaponEspInfo[i].pOutY > 1)
				DrawLine(pDevice, (int)WeaponEspInfo[i].pOutX, (int)WeaponEspInfo[i].pOutY, ScreenCX, ScreenCY * 2.0f, 20, D3DCOLOR_ARGB(255, 255, 255, 255));//0.1up, 1.0middle, 2.0down

																																							  //distance esp
			//if (WeaponEspInfo[i].pOutX > 1 && WeaponEspInfo[i].pOutY > 1)
				//DrawString(Font, (int)WeaponEspInfo[i].pOutX, (int)WeaponEspInfo[i].pOutY, D3DCOLOR_ARGB(255, 255, 255, 255), "%.f", (float)WeaponEspInfo[i].distance / 10.0f);
		}
	}
	WeaponEspInfo.clear();

	return EndScene_orig(pDevice);
}

//==========================================================================================================================

HRESULT APIENTRY Reset_hook(IDirect3DDevice9* pDevice, D3DPRESENT_PARAMETERS *pPresentationParameters)
{
	if (Font)
		Font->OnLostDevice();

	HRESULT ResetReturn = Reset_orig(pDevice, pPresentationParameters);

	if (SUCCEEDED(ResetReturn))
	{
		if (Font)
			Font->OnResetDevice();
	}

	return ResetReturn;
}

//==========================================================================================================================

DWORD WINAPI RosD3D(__in LPVOID lpParameter)
{
	HMODULE dDll = NULL;
	while (!dDll)
	{
		dDll = GetModuleHandleA("d3d9.dll");
		Sleep(100);
	}
	CloseHandle(dDll);

	IDirect3D9* d3d = NULL;
	IDirect3DDevice9* d3ddev = NULL;

	HWND tmpWnd = CreateWindowA("BUTTON", "RosD3D", WS_SYSMENU | WS_MINIMIZEBOX, CW_USEDEFAULT, CW_USEDEFAULT, 300, 300, NULL, NULL, Hand, NULL);
	if (tmpWnd == NULL)
	{
		//Log("[DirectX] Failed to create temp window");
		return 0;
	}

	d3d = Direct3DCreate9(D3D_SDK_VERSION);
	if (d3d == NULL)
	{
		DestroyWindow(tmpWnd);
		//Log("[DirectX] Failed to create temp Direct3D interface");
		return 0;
	}

	D3DPRESENT_PARAMETERS d3dpp;
	ZeroMemory(&d3dpp, sizeof(d3dpp));
	d3dpp.Windowed = TRUE;
	d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	d3dpp.hDeviceWindow = tmpWnd;
	d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;

	HRESULT result = d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, tmpWnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dpp, &d3ddev);
	if (result != D3D_OK)
	{
		d3d->Release();
		DestroyWindow(tmpWnd);
		//Log("[DirectX] Failed to create temp Direct3D device");
		return 0;
	}

	// We have the device, so walk the vtable to get the address of all the dx functions in d3d9.dll
#if defined _M_X64
	DWORD64* dVtable_ = (DWORD64*)d3ddev;
	dVtable_ = (DWORD64*)dVtable_[0];
#elif defined _M_IX86
	DWORD* dVtable_ = (DWORD*)d3ddev;
	dVtable_ = (DWORD*)dVtable_[0]; // == *d3ddev
#endif
									//Log("[DirectX] dVtable_: %x", dVtable_);

									//for(int i = 0; i < 95; i++)
									//{
									//Log("[DirectX] vtable[%i]: %x, pointer at %x", i, dVtable_[i], &dVtable_[i]);
									//}

									// Set EndScene_orig to the original EndScene etc.
	EndScene_orig = (EndScene)dVtable_[42];
	SetStreamSource_orig = (SetStreamSource)dVtable_[100];
	DrawIndexedPrimitive_orig = (DrawIndexedPrimitive)dVtable_[82];
	Reset_orig = (Reset)dVtable_[16];

	// Detour functions x86 & x64
	if (MH_Initialize() != MH_OK) { return 1; }
	if (MH_CreateHook((DWORD_PTR*)dVtable_[42], &EndScene_hook, reinterpret_cast<void**>(&EndScene_orig)) != MH_OK) { return 1; }
	if (MH_EnableHook((DWORD_PTR*)dVtable_[42]) != MH_OK) { return 1; }
	if (MH_CreateHook((DWORD_PTR*)dVtable_[100], &SetStreamSource_hook, reinterpret_cast<void**>(&SetStreamSource_orig)) != MH_OK) { return 1; }
	if (MH_EnableHook((DWORD_PTR*)dVtable_[100]) != MH_OK) { return 1; }
	if (MH_CreateHook((DWORD_PTR*)dVtable_[82], &DrawIndexedPrimitive_hook, reinterpret_cast<void**>(&DrawIndexedPrimitive_orig)) != MH_OK) { return 1; }
	if (MH_EnableHook((DWORD_PTR*)dVtable_[82]) != MH_OK) { return 1; }
	if (MH_CreateHook((DWORD_PTR*)dVtable_[16], &Reset_hook, reinterpret_cast<void**>(&Reset_orig)) != MH_OK) { return 1; }
	if (MH_EnableHook((DWORD_PTR*)dVtable_[16]) != MH_OK) { return 1; }

	//Log("[Detours] Detours attached\n");

	d3ddev->Release();
	d3d->Release();
	DestroyWindow(tmpWnd);

	return 1;
}

//==========================================================================================================================

BOOL WINAPI DllMain(HMODULE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH: // A process is loading the DLL.
		Hand = hinstDLL;
		DisableThreadLibraryCalls(hinstDLL); // disable unwanted thread notifications to reduce overhead
		CreateThread(0, 0, RosD3D, 0, 0, 0); //init our hooks
		break;

	case DLL_PROCESS_DETACH: // A process unloads the DLL.
		break;
	}
	return TRUE;
}