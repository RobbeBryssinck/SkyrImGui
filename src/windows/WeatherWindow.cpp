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

    ImGui::Separator();

	/*
    static uint32_t s_weatherId = 0;
    ImGui::InputScalar("New weather ID", ImGuiDataType_U32, &s_weatherId, 0, 0, "%" PRIx32,
                       ImGuiInputTextFlags_CharsHexadecimal);

    if (ImGui::Button("Set weather"))
    {
		RE::TESForm* pWeatherForm = RE::TESForm::LookupByID(s_weatherId);
		if (pWeatherForm) {
			RE::TESWeather* pWeather = pWeather->As<RE::TESWeather>();
			if (pWeather)
				//pSky->
		}
        RE::TESWeather* pWeather = Cast<TESWeather>(TESForm::GetById(s_weatherId));
        if (pWeather)
            pSky->SetWeather(pWeather);
    }

    if (ImGui::Button("Force weather"))
    {
        TESWeather* pWeather = Cast<TESWeather>(TESForm::GetById(s_weatherId));
        if (pWeather)
            pSky->ForceWeather(pWeather);
    }

    ImGui::Separator();

    if (ImGui::Button("Reset weather"))
    {
        pSky->ResetWeather();
    }
	*/

    ImGui::End();
}
