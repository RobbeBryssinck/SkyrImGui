#pragma once

#include <list>
#include <set>

struct DInputHook
{
	void SetEnabled(bool aEnabled) noexcept;
	[[nodiscard]] bool IsEnabled() const noexcept { return m_enabled; }
	void SetToggleKeys(std::initializer_list<unsigned long> aKeys) noexcept;
	[[nodiscard]] bool IsToggleKey(unsigned int aKey) const noexcept;

	void Acquire() const noexcept;
	void Unacquire() const noexcept;

	static void Install() noexcept;
	static DInputHook& Get() noexcept;

	void Update() const noexcept;

private:

	DInputHook() noexcept;
	~DInputHook() = default;

	std::set<unsigned long> m_toggleKeys;

	bool m_enabled{ false };
};
