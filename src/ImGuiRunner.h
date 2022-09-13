#pragma once

#include <Windows.h>

struct IDXGISwapChain;

class ImGuiRunner
{
public:
	static ImGuiRunner& Get() noexcept;

	ImGuiRunner();

	void Create(IDXGISwapChain* pChain) noexcept;
	void Present() noexcept;
	void Lost() noexcept;

	LRESULT WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	void RawInputHandler(RAWINPUT& aRawInput);
};
