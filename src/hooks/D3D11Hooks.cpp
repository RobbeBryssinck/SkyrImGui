#include "D3D11Hooks.h"
#include <SKSE/IAT.h>/
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
		auto pVTable = **reinterpret_cast<uintptr_t***>(ppSwapChain);

		RealDXGISwapChainPresent = reinterpret_cast<TDXGISwapChainPresent>(pVTable[8]);

		uint32_t oldProtect = 0;
		VirtualProtect(reinterpret_cast<void*>(pVTable[8]), 8, PAGE_EXECUTE_READWRITE, reinterpret_cast<DWORD*>(&oldProtect));
		REL::safe_write(pVTable[8], &HookDXGISwapChainPresent, 8);
		VirtualProtect(reinterpret_cast<void*>(pVTable[8]), 8, oldProtect, reinterpret_cast<DWORD*>(&oldProtect));
	}

	return result;
}

namespace D3D11Hooks
{

void D3D11Hooks::Install() noexcept
{
	RealD3D11CreateDeviceAndSwapChain = reinterpret_cast<TD3D11CreateDeviceAndSwapChain>(SKSE::PatchIAT(HookD3D11CreateDeviceAndSwapChain, "d3d11.dll", "D3D11CreateDeviceAndSwapChain"));
}

}
