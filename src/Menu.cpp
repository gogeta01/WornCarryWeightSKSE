#include "PCH.h"
#include "Menu.h"
#include "Settings.h"
#include "CarryWeightManager.h"
#include "SKSEMenuFramework.h"

namespace Menu
{
	void Register()
	{
		if (!SKSEMenuFramework::IsInstalled()) {
			logger::warn("SKSE Menu Framework is not installed - menu disabled");
			return;
		}

		SKSEMenuFramework::SetSection("Worn Carry Weight");
		SKSEMenuFramework::AddSectionItem("Settings", Render);
		logger::info("Menu registered with SKSE Menu Framework");
	}

	void __stdcall Render()
	{
		auto* settings = Settings::GetSingleton();
		auto* manager = CarryWeightManager::GetSingleton();

		ImGui::Text("Worn Carry Weight v1.0.0");
		ImGui::TextWrapped("Exclude worn equipment weight from inventory weight.");
		ImGui::Separator();

		float percent = settings->GetWornWeightPercent() * 100.0f;
		if (ImGui::SliderFloat("Worn Weight Exclusion (%)", &percent, 0.0f, 100.0f, "%.0f%%")) {
			settings->SetWornWeightPercent(percent / 100.0f);
		}

		if (ImGui::IsItemHovered()) {
			ImGui::SetTooltip("0%% = no exclusion\n100%% = full worn weight excluded");
		}

		ImGui::Spacing();

		if (ImGui::Button("Save Settings")) {
			settings->Save();
			RE::DebugNotification("Worn Carry Weight settings saved");
		}

		ImGui::SameLine();

		if (ImGui::Button("Reload INI")) {
			settings->Load();
			RE::DebugNotification("Worn Carry Weight settings reloaded");
		}

		ImGui::Separator();
		ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Live Weights:");
		ImGui::Text("Base inventory weight: %.1f", manager->GetLastBaseWeight());
		ImGui::Text("Worn item weight: %.1f", manager->GetLastWornWeight());
		ImGui::Text("Adjusted inventory weight: %.1f", manager->GetLastAdjustedWeight());
	}
}




