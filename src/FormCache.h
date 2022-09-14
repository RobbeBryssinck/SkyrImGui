#pragma once

#include <string>
#include <unordered_map>

class FormCache
{
public:
	static FormCache& Get() noexcept;
	const std::string& GetName(uint32_t aFormID) const noexcept;

private:
	void Initialize() noexcept;

	std::unordered_map<uint32_t, std::string> formIDsToEditorIDs;
};
