#include "D3D11Hooks.h"

#include <MinHook.h>

#include <SKSE/IAT.h>
#include <d3d11.h>
#include <ImGuiRunner.h>
#include "DInputHook.h"

using TD3D11CreateDeviceAndSwapChain = HRESULT(*)(IDXGIAdapter* pAdapter, D3D_DRIVER_TYPE DriverType, HMODULE Software, UINT Flags, const D3D_FEATURE_LEVEL* pFeatureLevels,
    UINT FeatureLevels, UINT SDKVersion, const DXGI_SWAP_CHAIN_DESC* pSwapChainDesc, IDXGISwapChain** ppSwapChain, ID3D11Device** ppDevice, D3D_FEATURE_LEVEL* pFeatureLevel,
    ID3D11DeviceContext** ppImmediateContext);

using TDXGISwapChainPresent = HRESULT(STDMETHODCALLTYPE*)(IDXGISwapChain* This, UINT SyncInterval, UINT Flags);

TD3D11CreateDeviceAndSwapChain RealD3D11CreateDeviceAndSwapChain = nullptr;
TDXGISwapChainPresent RealDXGISwapChainPresent = nullptr;

HRESULT __stdcall HookDXGISwapChainPresent(IDXGISwapChain* This, UINT SyncInterval, UINT Flags)
{
	ImGuiRunner& runner = ImGuiRunner::Get();

	static std::once_flag s_initializer;
	std::call_once(s_initializer, [runner, This]() mutable
		{
			runner.Create(This);
		});

	runner.Present();

	const auto result = RealDXGISwapChainPresent(This, SyncInterval, Flags);
	if (result == DXGI_ERROR_DEVICE_REMOVED || result == DXGI_ERROR_DEVICE_RESET)
	{
		runner.Lost();
	}

	return result;
}

HRESULT HookD3D11CreateDeviceAndSwapChain(_In_opt_ IDXGIAdapter* pAdapter, D3D_DRIVER_TYPE DriverType, HMODULE Software, UINT Flags, _In_opt_  const D3D_FEATURE_LEVEL* pFeatureLevels,
	UINT FeatureLevels, UINT SDKVersion, _In_opt_ const DXGI_SWAP_CHAIN_DESC* pSwapChainDesc, _Out_opt_ IDXGISwapChain** ppSwapChain, _Out_opt_ ID3D11Device** ppDevice,
	_Out_opt_ D3D_FEATURE_LEVEL* pFeatureLevel, _Out_opt_ ID3D11DeviceContext** ppImmediateContext)
{
	const auto result = RealD3D11CreateDeviceAndSwapChain(pAdapter, DriverType, Software, Flags, pFeatureLevels, FeatureLevels, SDKVersion, pSwapChainDesc, ppSwapChain, ppDevice, pFeatureLevel, ppImmediateContext);

	if (RealDXGISwapChainPresent == nullptr && ppSwapChain) {
#pragma warning( suppress : 6001 )
		auto pVTable = **reinterpret_cast<uintptr_t***>(ppSwapChain);

		RealDXGISwapChainPresent = reinterpret_cast<TDXGISwapChainPresent>(pVTable[8]);

		auto pHook = &HookDXGISwapChainPresent;
		REL::safe_write(reinterpret_cast<uintptr_t>(&pVTable[8]), &pHook, sizeof(uintptr_t));
	}

	return result;
}

struct RendererInitOSData
{
    HWND hWnd;
    HINSTANCE hInstance;
    WNDPROC pWndProc;
    HICON hIcon;
    const char* pClassName;
    uint32_t uiAdapter;
    int bCreateSwapChainRenderTarget;
};

uint16_t ExtractKey(RAWKEYBOARD aKeyboard)
{
	auto key = aKeyboard.VKey;
	bool e0 = aKeyboard.Flags & RI_KEY_E0;
	bool e1 = aKeyboard.Flags & RI_KEY_E1;
	auto aScanCode = aKeyboard.MakeCode;

	if (key == VK_SHIFT)
	{
		key = static_cast<uint16_t>(MapVirtualKey(aScanCode, MAPVK_VSC_TO_VK_EX));
	}
	else if (key == VK_NUMLOCK)
	{
		aScanCode = static_cast<uint16_t>(MapVirtualKey(key, MAPVK_VK_TO_VSC) | 0x100);
	}

	if (e1)
	{
		if (key == VK_PAUSE)
		{
			aScanCode = 0x45;
		}
		else
		{
			aScanCode = static_cast<uint16_t>(MapVirtualKey(key, MAPVK_VK_TO_VSC));
		}
	}

	if (e0)
	{
		switch (key)
		{
		case VK_CONTROL:
			key = VK_RCONTROL;
			break;
		case VK_MENU:
			key = VK_RMENU;
			break;
		case VK_RETURN:
			key = VK_SEPARATOR;
			break;
		}
	}
	else
	{
		switch (key)
		{
		case VK_CONTROL:
			key = VK_LCONTROL;
			break;
		case VK_MENU:
			key = VK_LMENU;
			break;
		case VK_INSERT:
			key = VK_NUMPAD0;
			break;
		case VK_DELETE:
			key = VK_DECIMAL;
			break;
		case VK_HOME:
			key = VK_NUMPAD7;
			break;
		case VK_END:
			key = VK_NUMPAD1;
			break;
		case VK_PRIOR:
			key = VK_NUMPAD9;
			break;
		case VK_NEXT:
			key = VK_NUMPAD3;
			break;
		case VK_LEFT:
			key = VK_NUMPAD4;
			break;
		case VK_RIGHT:
			key = VK_NUMPAD6;
			break;
		case VK_UP:
			key = VK_NUMPAD8;
			break;
		case VK_DOWN:
			key = VK_NUMPAD2;
			break;
		case VK_CLEAR:
			key = VK_NUMPAD5;
			break;
		}
	}

	return key;
}

using TRendererInit = void(void*, RendererInitOSData*, void*, void*);
static TRendererInit* RealRendererInit = nullptr;

static WNDPROC RealWndProc = nullptr;

LRESULT CALLBACK HookWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static bool s_isEnabledCache = false;

	if (DInputHook::Get().IsEnabled()) {
		ImGuiRunner::Get().WndProc(hwnd, uMsg, wParam, lParam);

		if (uMsg == WM_INPUT)
		{
			RAWINPUT input{};
			UINT size = sizeof(RAWINPUT);
			GetRawInputData(reinterpret_cast<HRAWINPUT>(lParam), RID_INPUT, &input, &size, sizeof(RAWINPUTHEADER));

			const auto keyboard = input.data.keyboard;

			if (keyboard.VKey && keyboard.VKey != 255) {
				const auto key = ExtractKey(keyboard);

				if (!(keyboard.Flags & RI_KEY_BREAK) && key == VK_RCONTROL || key == VK_F2) {
					DInputHook::Get().SetEnabled(false);
				}
			}
		}
	}

	if (s_isEnabledCache != DInputHook::Get().IsEnabled()) {
		s_isEnabledCache = DInputHook::Get().IsEnabled();
		if (s_isEnabledCache) {
			while (ShowCursor(TRUE) <= 0)
				;
		} else {
			while (ShowCursor(FALSE) >= 0)
				;
		}
	}

    return RealWndProc(hwnd, uMsg, wParam, lParam);
}

void HookRendererInit(void* apThis, RendererInitOSData* apOSData, void* apFBData, void* apOut)
{
	RealWndProc = apOSData->pWndProc;
	apOSData->pWndProc = HookWndProc;
	RealRendererInit(apThis, apOSData, apFBData, apOut);
}

namespace D3D11Hooks
{

void D3D11Hooks::Install() noexcept
{
	RealD3D11CreateDeviceAndSwapChain = reinterpret_cast<TD3D11CreateDeviceAndSwapChain>(SKSE::PatchIAT(HookD3D11CreateDeviceAndSwapChain, "d3d11.dll", "D3D11CreateDeviceAndSwapChain"));

	constexpr REL::ID RendererInitAddr{77226};
	REL::Relocation<TRendererInit> RendererInit{ RendererInitAddr };

	RealRendererInit = reinterpret_cast<TRendererInit*>(RendererInit.address());
	MH_CreateHook(RealRendererInit, HookRendererInit, reinterpret_cast<void**>(&RealRendererInit));

	auto dwStyle = WS_OVERLAPPEDWINDOW;
	REL::safe_write(RendererInit.address() + 0x175, &dwStyle, sizeof(dwStyle));

	constexpr REL::ID WindowLocAddr{68781};
	REL::Relocation<TRendererInit> WindowLoc{ WindowLocAddr };

	int32_t windowLocSetting = 3;
	REL::safe_write(WindowLoc.address() + 0x57, &windowLocSetting, sizeof(windowLocSetting));

	MH_EnableHook(nullptr);
}

}
