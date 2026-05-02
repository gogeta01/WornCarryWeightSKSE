#include "PCH.h"
#include "CarryWeightManager.h"
#include "Hooks.h"
#include "Menu.h"
#include "Settings.h"

namespace Plugin
{
	inline constexpr REL::Version VERSION{ 1, 0, 0, 0 };
	inline constexpr auto NAME = "WornCarryWeight"sv;
}

namespace
{
	std::string g_logPath;
	std::string g_fallbackLogPath;

	void InitializeLog()
	{
		std::vector<std::shared_ptr<spdlog::sinks::sink>> sinks;

		auto path = logger::log_directory();
		if (path) {
			*path /= fmt::format("{}.log"sv, Plugin::NAME);
			g_logPath = path->string();
			sinks.push_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true));
		}

		const auto fallbackPath = std::filesystem::path("Data/SKSE/Plugins") / fmt::format("{}.log"sv, Plugin::NAME);
		g_fallbackLogPath = fallbackPath.string();
		std::error_code ec;
		std::filesystem::create_directories(fallbackPath.parent_path(), ec);
		sinks.push_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>(fallbackPath.string(), true));

		const auto level = spdlog::level::trace;
		auto log = std::make_shared<spdlog::logger>("global log"s, sinks.begin(), sinks.end());
		log->set_level(level);
		log->flush_on(level);

		spdlog::set_default_logger(std::move(log));
		spdlog::set_pattern("[%H:%M:%S.%e] %v"s);
		if (!g_logPath.empty()) {
			logger::info("Log initialized: {}", g_logPath);
		}
		logger::info("Log fallback: {}", g_fallbackLogPath);

		// Ensure a visible log file exists even if the SKSE log directory isn't used.
		{
			std::ofstream fallbackStream(g_fallbackLogPath, std::ios::app);
			if (fallbackStream.is_open()) {
				fallbackStream << "[Init] WornCarryWeight logging started\n";
			}
		}
	}
}

void MessageHandler(SKSE::MessagingInterface::Message* a_msg)
{
	switch (a_msg->type) {
	case SKSE::MessagingInterface::kDataLoaded:
		Settings::GetSingleton()->Load();
		Hooks::Install();
		if (!g_logPath.empty()) {
			RE::DebugNotification(std::format("WornCarryWeight log: {}", g_logPath).c_str());
		} else {
			RE::DebugNotification(std::format("WornCarryWeight log: {}", g_fallbackLogPath).c_str());
		}
		logger::info("Worn Carry Weight initialized");
		break;

	case SKSE::MessagingInterface::kPostLoadGame:
	case SKSE::MessagingInterface::kNewGame:
		if (auto* player = RE::PlayerCharacter::GetSingleton()) {
			CarryWeightManager::GetSingleton()->Update(player, true);
			logger::info("Carry weight applied on load");
		}
		break;
	}
}

extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Query(const SKSE::QueryInterface* a_skse, SKSE::PluginInfo* a_info)
{
	a_info->infoVersion = SKSE::PluginInfo::kVersion;
	a_info->name = Plugin::NAME.data();
	a_info->version = Plugin::VERSION[0];

	if (a_skse->IsEditor()) {
		logger::critical("Loaded in editor, marking as incompatible"sv);
		return false;
	}

	const auto ver = a_skse->RuntimeVersion();
	if (ver < SKSE::RUNTIME_SSE_1_5_39) {
		logger::critical(FMT_STRING("Unsupported runtime version {}"), ver.string());
		return false;
	}

	return true;
}

extern "C" DLLEXPORT constinit auto SKSEPlugin_Version = []() {
	SKSE::PluginVersionData v;

	v.PluginVersion(Plugin::VERSION);
	v.PluginName(Plugin::NAME);
	v.AuthorName("User");

	v.UsesAddressLibrary(true);
	v.CompatibleVersions({ SKSE::RUNTIME_SSE_LATEST });
	v.HasNoStructUse(true);

	return v;
}();

extern "C" DLLEXPORT bool SKSEAPI SKSEPlugin_Load(const SKSE::LoadInterface* a_skse)
{
	InitializeLog();
	logger::info("{} v{}"sv, Plugin::NAME, Plugin::VERSION.string());

	SKSE::Init(a_skse);

	auto messaging = SKSE::GetMessagingInterface();
	if (!messaging->RegisterListener("SKSE", MessageHandler)) {
		logger::error("Failed to register messaging listener");
		return false;
	}

	Menu::Register();
	logger::info("Plugin loaded successfully");
	return true;
}

