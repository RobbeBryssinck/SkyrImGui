#include "WeatherWindow.h"

#include <imgui.h>
#include <inttypes.h>

void WeatherWindow::Update()
{
	ImGui::Begin("Weather");

    RE::Sky* pSky = RE::Sky::GetSingleton();

    RE::TESWeather* pCurrentWeather = pSky->currentWeather;
    if (pCurrentWeather)
    {
        ImGui::InputScalar("Current weather ID", ImGuiDataType_U32, &pCurrentWeather->formID, 0, 0, "%" PRIx32,
                           ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_ReadOnly);
    }

    RE::TESWeather* pPreviousWeather = pSky->lastWeather;
    if (pPreviousWeather)
    {
        ImGui::InputScalar("Previous weather ID", ImGuiDataType_U32, &pPreviousWeather->formID, 0, 0, "%" PRIx32,
                           ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_ReadOnly);
    }

	ImGui::Text("Current transition: %.2f", pSky->currentWeatherPct);

    ImGui::Separator();

    static uint32_t s_weatherId = 0;
    ImGui::InputScalar("New weather ID", ImGuiDataType_U32, &s_weatherId, 0, 0, "%" PRIx32,
                       ImGuiInputTextFlags_CharsHexadecimal);

    if (ImGui::Button("Set weather"))
    {
		RE::TESForm* pWeatherForm = RE::TESForm::LookupByID(s_weatherId);
		if (pWeatherForm) {
			RE::TESWeather* pWeather = pWeatherForm->As<RE::TESWeather>();
			if (pWeather)
				pSky->SetWeather(pWeather, true, true);
		}
    }

	ImGui::Separator();

    if (ImGui::Button("Force weather"))
    {
		RE::TESForm* pWeatherForm = RE::TESForm::LookupByID(s_weatherId);
		if (pWeatherForm) {
			RE::TESWeather* pWeather = pWeatherForm->As<RE::TESWeather>();
			if (pWeather)
				pSky->ForceWeather(pWeather, true);
		}
    }

    ImGui::Separator();

    if (ImGui::Button("Reset weather"))
        pSky->ResetWeather();

    ImGui::End();
}
