#pragma once

class IDXGISwapChain;

class ImGuiRunner
{
public:
	static ImGuiRunner& Get() noexcept;

	ImGuiRunner();

	void Create(IDXGISwapChain* pChain) noexcept;
	void Present() noexcept;
	void Lost() noexcept;
};
