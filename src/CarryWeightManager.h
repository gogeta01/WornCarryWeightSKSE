#pragma once

#include <unordered_map>

namespace RE
{
	class InventoryMenu;
	class PlayerCharacter;
	class TESForm;
}

class CarryWeightManager
{
public:
	static CarryWeightManager* GetSingleton()
	{
		static CarryWeightManager singleton;
		return std::addressof(singleton);
	}

	void Update(RE::PlayerCharacter* a_player, bool a_force = false);
	void Tick(RE::PlayerCharacter* a_player);
	void OnEquipEvent(RE::FormID a_formID, bool a_equipped);

	float GetLastBaseWeight() const { return _lastBaseWeight; }
	float GetLastWornWeight() const { return _lastWornWeight; }
	float GetLastAdjustedWeight() const { return _lastAdjustedWeight; }

private:
	struct WeightOverride
	{
		float originalWeight{ 0.0f };
	};

	bool RebuildOverrides(RE::PlayerCharacter* a_player, float a_percent);
	void RestoreAllOverrides();
	bool IsSupportedForm(const RE::TESForm* a_form) const;
	void SetFormWeight(RE::TESForm* a_form, float a_weight);

	void RefreshInventoryMenu(RE::InventoryMenu* a_menu, RE::PlayerCharacter* a_player);
	void QueueInventoryRefresh();

	std::unordered_map<RE::FormID, WeightOverride> _overrides;
	std::unordered_map<RE::FormID, std::int32_t> _wornCountOverrides;
	float _lastBaseWeight{ 0.0f };
	float _lastWornWeight{ 0.0f };
	float _lastAdjustedWeight{ 0.0f };
	float _lastPercent{ -1.0f };
	bool _pendingMenuRefresh{ false };
};

