#include "ImGuiRunner.h"

#include <PCH.h>

#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>

#include <d3d11.h>

ImGuiRunner& ImGuiRunner::Get() noexcept
{
	static ImGuiRunner s_instance;
	return s_instance;
}

ImGuiRunner::ImGuiRunner()
{
	ImGui::CreateContext();
}

void ImGuiRunner::Create(IDXGISwapChain* pChain) noexcept
{
	ID3D11Device* d3dDevice = nullptr;
    ID3D11DeviceContext* d3dContext = nullptr;

	DXGI_SWAP_CHAIN_DESC desc{};
    desc.OutputWindow = nullptr;
	pChain->GetDesc(&desc);

    // init platform
	if (!ImGui_ImplWin32_Init(desc.OutputWindow))
		logger::error("Failed to init ImGui Win32");

    pChain->GetDevice(__uuidof(d3dDevice), reinterpret_cast<void**>(&d3dDevice));
    d3dDevice->GetImmediateContext(&d3dContext);

    ImGui_ImplDX11_Init(d3dDevice, d3dContext);
}

void ImGuiRunner::Present() noexcept
{
    ImGui_ImplDX11_NewFrame();

    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

	// TODO: draw stuff
	ImGui::Begin("Test");
	ImGui::Text("Hello world");
	ImGui::End();

    ImGui::Render();

    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

void ImGuiRunner::Lost() noexcept
{
}

