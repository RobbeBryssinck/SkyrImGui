#include "FormCache.h"

#include <Windows.h>

FormCache& FormCache::Get() noexcept
{
	static FormCache s_instance;
	if (s_instance.formIDsToEditorIDs.empty())
		s_instance.Initialize();
	if (GetAsyncKeyState(VK_F8) & 0x01)
		s_instance.Initialize();
	return s_instance;
}

const std::string& FormCache::GetName(uint32_t aFormID) const noexcept
{
	auto it = formIDsToEditorIDs.find(aFormID);
	if (it == formIDsToEditorIDs.end()) {
		static std::string s_empty = "";
		return s_empty;
	}
	else
		return it->second;
}

void FormCache::Initialize() noexcept
{
	auto* pEditorMap = RE::TESForm::GetAllFormsByEditorID().first;
	if (!pEditorMap) {
		logger::error("Editor map not found");
		return;
	}

	formIDsToEditorIDs.clear();

	auto& lock = RE::TESForm::GetAllFormsByEditorID().second.get();
	lock.LockForRead();
	for (auto const& [editorID, pForm] : *pEditorMap) {
		if (pForm->formType == RE::FormType::Weather)
			logger::info("Weather {:X}", pForm->formID);
		formIDsToEditorIDs.insert({ pForm->formID, editorID.c_str() });
	}
	lock.UnlockForRead();

	logger::info("Entries: {}", formIDsToEditorIDs.size());
}
