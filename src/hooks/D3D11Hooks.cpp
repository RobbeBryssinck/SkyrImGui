#include "D3D11Hooks.h"

#include <MinHook.h>

#include <SKSE/IAT.h>
#include <d3d11.h>
#include <ImGuiRunner.h>

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

using TRendererInit = void(void*, RendererInitOSData*, void*, void*);
static TRendererInit* RealRendererInit = nullptr;

static WNDPROC RealWndProc = nullptr;

LRESULT CALLBACK HookWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return RealWndProc(hwnd, uMsg, wParam, lParam);
}

void HookRendererInit(void* apThis, RendererInitOSData* apOSData, void* apFBData, void* apOut)
{
	logger::info("Hello world");
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
