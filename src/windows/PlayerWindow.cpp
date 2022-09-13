#include "PlayerWindow.h"

#include <imgui.h>

#include <RE/P/PlayerCharacter.h>

void PlayerWindow::Update()
{
	ImGui::Begin("Player");

	auto* pPlayer = RE::PlayerCharacter::GetSingleton();

	if (!pPlayer) {
		ImGui::End();
		return;
	}

	auto* pLeft = pPlayer->GetEquippedObject(true);
	auto* pRight = pPlayer->GetEquippedObject(false);
	uint32_t leftId = pLeft ? pLeft->formID : 0;
	uint32_t rightId = pRight ? pRight->formID : 0;

    ImGui::InputScalar("Left Item", ImGuiDataType_U32, (void*)&leftId, nullptr, nullptr, nullptr,
                       ImGuiInputTextFlags_ReadOnly);
    ImGui::InputScalar("Right Item", ImGuiDataType_U32, (void*)&rightId, nullptr, nullptr, nullptr,
                       ImGuiInputTextFlags_ReadOnly);

	ImGui::End();
}
