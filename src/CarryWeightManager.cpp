#include "PCH.h"
#include "CarryWeightManager.h"
#include "Settings.h"

#include "RE/E/ExtraWorn.h"
#include "RE/E/ExtraWornLeft.h"

#include <array>
#include <cmath>
#include <unordered_map>
#include <unordered_set>

namespace
{
	bool InvokeFirstMatch(RE::GFxValue& a_obj, const std::array<const char*, 3>& a_names)
	{
		if (!a_obj.IsObject()) {
			return false;
		}

		for (const auto* name : a_names) {
			if (a_obj.Invoke(name)) {
				logger::debug("InventoryMenu refresh invoked: {}", name);
				return true;
			}
		}

		return false;
	}

	std::int32_t GetWornCount(const RE::InventoryEntryData* a_entry)
	{
		if (!a_entry || !a_entry->extraLists) {
			return 0;
		}

		std::int32_t wornCount = 0;
		for (const auto* extraList : *a_entry->extraLists) {
			if (!extraList) {
				continue;
			}

			if (extraList->HasType<RE::ExtraWorn>() || extraList->HasType<RE::ExtraWornLeft>()) {
				++wornCount;
			}
		}

		return wornCount;
	}

}

void CarryWeightManager::Update(RE::PlayerCharacter* a_player, bool a_force)
{
	if (!a_player) {
		return;
	}

	auto* inventory = a_player->GetInventoryChanges();
	if (!inventory) {
		return;
	}

	const float percent = Settings::GetSingleton()->GetWornWeightPercent();
	const bool percentChanged = percent != _lastPercent;

	// Skip heavy work if inventory hasn't changed and the slider hasn't changed.
	if (!a_force && !inventory->changed && !percentChanged && std::isfinite(_lastAdjustedWeight)) {
		return;
	}

	_lastPercent = percent;

	const bool weightsChanged = RebuildOverrides(a_player, percent);
	if (weightsChanged || percentChanged || a_force) {
		if (!_pendingMenuRefresh) {
			QueueInventoryRefresh();
		}
	}
}

void CarryWeightManager::Tick(RE::PlayerCharacter* a_player)
{
	if (!_pendingMenuRefresh || !a_player) {
		return;
	}

	auto* ui = RE::UI::GetSingleton();
	if (!ui || !ui->IsMenuOpen(RE::InventoryMenu::MENU_NAME)) {
		return;
	}

	auto* queue = RE::UIMessageQueue::GetSingleton();
	auto* strings = RE::InterfaceStrings::GetSingleton();
	if (queue && strings) {
		auto* data = queue->CreateUIMessageData(strings->inventoryUpdateData);
		RE::InventoryUpdateData* invData = data ?
			static_cast<RE::InventoryUpdateData*>(data) :
			new RE::InventoryUpdateData();

		if (invData) {
			invData->unk10 = static_cast<RE::RefHandle>(a_player->GetHandle().native_handle());
			invData->unk18 = nullptr;
			queue->AddMessage(strings->inventoryMenu, RE::UI_MESSAGE_TYPE::kInventoryUpdate, invData);
		}
	}

	if (auto menu = ui->GetMenu<RE::InventoryMenu>()) {
		RefreshInventoryMenu(menu.get(), a_player);
	}

	_pendingMenuRefresh = false;
}

void CarryWeightManager::RefreshInventoryMenu(RE::InventoryMenu* a_menu, RE::PlayerCharacter* a_player)
{
	if (!a_menu || !a_player) {
		return;
	}

	auto& data = a_menu->GetRuntimeData();
	if (data.itemList) {
		data.itemList->Update(a_player);
	}

	const std::array<const char*, 3> updateMethods{
		"UpdateEncumbrance",
		"UpdatePlayerInfo",
		"UpdateWeight"
	};

	if (data.bottomBar && InvokeFirstMatch(data.bottomBar->obj, updateMethods)) {
		return;
	}

	InvokeFirstMatch(data.root, updateMethods);
}

void CarryWeightManager::QueueInventoryRefresh()
{
	_pendingMenuRefresh = true;
}

bool CarryWeightManager::RebuildOverrides(RE::PlayerCharacter* a_player, float a_percent)
{
	if (!a_player) {
		return false;
	}

	auto* inventory = a_player->GetInventoryChanges();
	if (!inventory || !inventory->entryList) {
		return false;
	}

	std::unordered_map<RE::FormID, std::int32_t> wornCounts;
	std::unordered_map<RE::FormID, std::int32_t> entryWornCounts;
	for (const auto* entry : *inventory->entryList) {
		if (!entry) {
			continue;
		}

		const auto wornCount = GetWornCount(entry);
		if (wornCount <= 0) {
			continue;
		}

		auto* object = entry->GetObject();
		if (!object || !IsSupportedForm(object)) {
			continue;
		}

		const auto formID = object->GetFormID();
		wornCounts[formID] += wornCount;
		entryWornCounts[formID] += wornCount;
	}

	for (auto it = _wornCountOverrides.begin(); it != _wornCountOverrides.end();) {
		const auto formID = it->first;
		const auto overrideCount = std::max(0, it->second);
		auto entryIt = entryWornCounts.find(formID);
		if (entryIt != entryWornCounts.end() && entryIt->second == overrideCount) {
			it = _wornCountOverrides.erase(it);
			continue;
		}

		wornCounts[formID] = overrideCount;
		++it;
	}

	const auto totalCounts = a_player->GetInventoryCounts();
	std::unordered_set<RE::FormID> touchedForms;
	bool weightsChanged = false;

	float baseWeight = 0.0f;
	float wornWeight = 0.0f;
	float adjustedWeight = 0.0f;

	for (const auto& [object, count] : totalCounts) {
		if (!object || count <= 0) {
			continue;
		}

		const auto formID = object->GetFormID();
		const auto currentWeight = std::max(0.0f, object->GetWeight());
		touchedForms.insert(formID);

		if (!IsSupportedForm(object)) {
			baseWeight += currentWeight * static_cast<float>(count);
			adjustedWeight += currentWeight * static_cast<float>(count);
			continue;
		}

		auto wornCount = wornCounts.contains(formID) ? wornCounts[formID] : 0;
		wornCount = std::min(wornCount, count);
		auto originalWeight = currentWeight;
		if (auto it = _overrides.find(formID); it != _overrides.end()) {
			originalWeight = it->second.originalWeight;
		}
		baseWeight += originalWeight * static_cast<float>(count);
		wornWeight += originalWeight * static_cast<float>(wornCount);

		if (wornCount <= 0 || a_percent <= 0.0f) {
			if (_overrides.contains(formID)) {
				SetFormWeight(object, originalWeight);
				_overrides.erase(formID);
				weightsChanged = true;
			}
			adjustedWeight += originalWeight * static_cast<float>(count);
			continue;
		}

		if (!_overrides.contains(formID)) {
			_overrides.emplace(formID, WeightOverride{ originalWeight });
		}

		const float ratio = static_cast<float>(wornCount) / static_cast<float>(count);
		const float newWeight = std::max(0.0f, originalWeight * (1.0f - (a_percent * ratio)));
		if (std::abs(newWeight - currentWeight) > 0.0001f) {
			SetFormWeight(object, newWeight);
			weightsChanged = true;
		}

		adjustedWeight += newWeight * static_cast<float>(count);
	}

	for (auto it = _overrides.begin(); it != _overrides.end();) {
		if (!touchedForms.contains(it->first)) {
			if (auto* form = RE::TESForm::LookupByID(it->first)) {
				SetFormWeight(form, it->second.originalWeight);
				weightsChanged = true;
			}
			it = _overrides.erase(it);
		} else {
			++it;
		}
	}

	_lastBaseWeight = baseWeight;
	_lastWornWeight = wornWeight;
	_lastAdjustedWeight = adjustedWeight;

	if (weightsChanged) {
		inventory->changed = true;
	}

	return weightsChanged;
}

void CarryWeightManager::RestoreAllOverrides()
{
	for (const auto& [formID, info] : _overrides) {
		if (auto* form = RE::TESForm::LookupByID(formID)) {
			SetFormWeight(form, info.originalWeight);
		}
	}
	_overrides.clear();
	_wornCountOverrides.clear();
}

bool CarryWeightManager::IsSupportedForm(const RE::TESForm* a_form) const
{
	if (!a_form) {
		return false;
	}

	return a_form->IsArmor() || a_form->IsWeapon();
}

void CarryWeightManager::SetFormWeight(RE::TESForm* a_form, float a_weight)
{
	if (!a_form) {
		return;
	}

	auto* weightForm = a_form->As<RE::TESWeightForm>();
	if (!weightForm) {
		return;
	}

	weightForm->weight = std::max(0.0f, a_weight);
}

void CarryWeightManager::OnEquipEvent(RE::FormID a_formID, bool a_equipped)
{
	if (a_formID == 0) {
		return;
	}

	auto* form = RE::TESForm::LookupByID(a_formID);
	if (!IsSupportedForm(form)) {
		return;
	}

	const std::int32_t delta = a_equipped ? 1 : -1;
	auto& count = _wornCountOverrides[a_formID];
	count = std::max(0, count + delta);
}

