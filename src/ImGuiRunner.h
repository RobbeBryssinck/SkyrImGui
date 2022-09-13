#pragma once

#include <Windows.h>
#include <windows/Window.h>
#include <memory>

struct IDXGISwapChain;

class ImGuiRunner
{
public:
	static ImGuiRunner& Get() noexcept;

	ImGuiRunner();

	ImGuiRunner(const ImGuiRunner&) = delete;
	ImGuiRunner& operator=(const ImGuiRunner&) = delete;

	void Create(IDXGISwapChain* pChain) noexcept;
	void Present() noexcept;
	void Lost() noexcept;

	LRESULT WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
	void CreateWindows() noexcept;

	std::vector<std::unique_ptr<Window>> windows{};
};
