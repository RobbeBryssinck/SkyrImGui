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

uint16_t ProcessKey(uint16_t aKey, uint16_t aScanCode, bool aE0, bool aE1)
{
	if (aKey == VK_SHIFT)
	{
		aKey = static_cast<uint16_t>(MapVirtualKey(aScanCode, MAPVK_VSC_TO_VK_EX));
	}
	else if (aKey == VK_NUMLOCK)
	{
		aScanCode = static_cast<uint16_t>(MapVirtualKey(aKey, MAPVK_VK_TO_VSC) | 0x100);
	}

	if (aE1)
	{
		if (aKey == VK_PAUSE)
		{
			aScanCode = 0x45;
		}
		else
		{
			aScanCode = static_cast<uint16_t>(MapVirtualKey(aKey, MAPVK_VK_TO_VSC));
		}
	}

	if (aE0)
	{
		switch (aKey)
		{
		case VK_CONTROL:
			aKey = VK_RCONTROL;
			break;
		case VK_MENU:
			aKey = VK_RMENU;
			break;
		case VK_RETURN:
			aKey = VK_SEPARATOR;
			break;
		}
	}
	else
	{
		switch (aKey)
		{
		case VK_CONTROL:
			aKey = VK_LCONTROL;
			break;
		case VK_MENU:
			aKey = VK_LMENU;
			break;
		case VK_INSERT:
			aKey = VK_NUMPAD0;
			break;
		case VK_DELETE:
			aKey = VK_DECIMAL;
			break;
		case VK_HOME:
			aKey = VK_NUMPAD7;
			break;
		case VK_END:
			aKey = VK_NUMPAD1;
			break;
		case VK_PRIOR:
			aKey = VK_NUMPAD9;
			break;
		case VK_NEXT:
			aKey = VK_NUMPAD3;
			break;
		case VK_LEFT:
			aKey = VK_NUMPAD4;
			break;
		case VK_RIGHT:
			aKey = VK_NUMPAD6;
			break;
		case VK_UP:
			aKey = VK_NUMPAD8;
			break;
		case VK_DOWN:
			aKey = VK_NUMPAD2;
			break;
		case VK_CLEAR:
			aKey = VK_NUMPAD5;
			break;
		}
	}

	return aKey;
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
			ImGuiRunner::Get().RawInputHandler(input);

			const auto keyboard = input.data.keyboard;
			auto key = keyboard.VKey;
			bool E0 = keyboard.Flags & RI_KEY_E0;
			bool E1 = keyboard.Flags & RI_KEY_E1;

			if (key && key != 255) {
				key = ProcessKey(key, keyboard.MakeCode, E0, E1);

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
	MH_EnableHook(nullptr);
}

}
