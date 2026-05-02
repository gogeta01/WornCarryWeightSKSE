#include "PCH.h"
#include "Settings.h"

#include <SimpleIni.h>

std::filesystem::path Settings::GetIniPath()
{
	return L"Data/SKSE/Plugins/WornCarryWeight.ini";
}

void Settings::Clamp()
{
	_wornWeightPercent = std::clamp(_wornWeightPercent, 0.0f, 1.0f);
}

void Settings::Load()
{
	CSimpleIniA ini;
	ini.SetUnicode();

	const auto path = GetIniPath();
	if (ini.LoadFile(path.c_str()) == SI_OK) {
		_wornWeightPercent = static_cast<float>(
			ini.GetDoubleValue("General", "fWornWeightPercent", 1.0));
		Clamp();
		logger::info("Settings loaded: wornWeightPercent={:.3f}", _wornWeightPercent);
		return;
	}

	// Create default INI if missing
	_wornWeightPercent = 1.0f;
	Save();
	logger::info("Settings file not found, created default INI");
}

void Settings::Save() const
{
	CSimpleIniA ini;
	ini.SetUnicode();

	ini.SetDoubleValue("General", "fWornWeightPercent", _wornWeightPercent,
		"; Percentage of worn item weight excluded from inventory weight (0.0 - 1.0)");

	const auto path = GetIniPath();
	ini.SaveFile(path.c_str());
}

void Settings::SetWornWeightPercent(float a_percent)
{
	_wornWeightPercent = a_percent;
	Clamp();
}




