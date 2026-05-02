#include "PCH.h"
#include "Hooks.h"
#include "CarryWeightManager.h"

namespace
{
	struct PlayerUpdateHook
	{
		static void thunk(RE::PlayerCharacter* a_this, float a_delta)
		{
			func(a_this, a_delta);
			CarryWeightManager::GetSingleton()->Update(a_this);
			CarryWeightManager::GetSingleton()->Tick(a_this);
		}

		static inline REL::Relocation<decltype(thunk)> func;
		static constexpr std::size_t size = 0xAD;
	};

	class EquipEventHandler : public RE::BSTEventSink<RE::TESEquipEvent>
	{
	public:
		static EquipEventHandler* GetSingleton()
		{
			static EquipEventHandler singleton;
			return std::addressof(singleton);
		}

		RE::BSEventNotifyControl ProcessEvent(
			const RE::TESEquipEvent* a_event,
			RE::BSTEventSource<RE::TESEquipEvent>*) override
		{
			if (!a_event || !a_event->actor) {
				return RE::BSEventNotifyControl::kContinue;
			}

			auto* player = RE::PlayerCharacter::GetSingleton();
			if (!player || a_event->actor.get() != player) {
				return RE::BSEventNotifyControl::kContinue;
			}

			CarryWeightManager::GetSingleton()->OnEquipEvent(a_event->baseObject, a_event->equipped);
			CarryWeightManager::GetSingleton()->Update(player, true);
			return RE::BSEventNotifyControl::kContinue;
		}
	};
}

void Hooks::Install()
{
	stl::write_vfunc<RE::PlayerCharacter, PlayerUpdateHook>();
	RE::ScriptEventSourceHolder::GetSingleton()->AddEventSink(EquipEventHandler::GetSingleton());
	logger::info("Installed PlayerCharacter update hook and equip event sink");
}

