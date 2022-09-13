#include "CellWindow.h"

#include <inttypes.h>
#include <imgui.h>

void CellWindow::Update()
{
	ImGui::Begin("Cell");

	auto* pPlayer = RE::PlayerCharacter::GetSingleton();

	if (!pPlayer) {
		ImGui::End();
		return;
	}

    if (auto* pWorldSpace = pPlayer->GetWorldspace())
    {
        if (ImGui::CollapsingHeader("World space", ImGuiTreeNodeFlags_DefaultOpen))
        {
            const uint32_t worldFormId = pWorldSpace->formID;
            ImGui::InputScalar("Id", ImGuiDataType_U32, (void*)&worldFormId, nullptr, nullptr, "%" PRIx32,
                               ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_CharsHexadecimal);

            char* pName = (char*)pWorldSpace->GetName();
            size_t nameLen = strlen(pName);
            ImGui::InputText("Name", pName, nameLen, ImGuiInputTextFlags_ReadOnly);

            char* pEditorId = (char*)pWorldSpace->GetFormEditorID();
            size_t editorIdLen = strlen(pEditorId);
            ImGui::InputText("Editor ID", pEditorId, editorIdLen, ImGuiInputTextFlags_ReadOnly);
        }
    }

    if (auto* pCell = pPlayer->parentCell)
    {
        if (ImGui::CollapsingHeader("Parent cell", ImGuiTreeNodeFlags_DefaultOpen))
        {
            const uint32_t cellId = pCell->formID;
            ImGui::InputScalar("Id", ImGuiDataType_U32, (void*)&cellId, nullptr, nullptr, "%" PRIx32,
                               ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_CharsHexadecimal);

            char* pName = (char*)pCell->GetName();
            size_t nameLen = strlen(pName);
            ImGui::InputText("Name", pName, nameLen, ImGuiInputTextFlags_ReadOnly);

            char* pEditorId = (char*)pCell->GetFormEditorID();
            size_t editorIdLen = strlen(pEditorId);
            ImGui::InputText("Editor ID", pEditorId, editorIdLen, ImGuiInputTextFlags_ReadOnly);
        }
    }

	ImGui::End();
}
