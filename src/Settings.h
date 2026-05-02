#pragma once

class Settings
{
public:
	static Settings* GetSingleton()
	{
		static Settings singleton;
		return std::addressof(singleton);
	}

	void Load();
	void Save() const;

	float GetWornWeightPercent() const { return _wornWeightPercent; }
	void SetWornWeightPercent(float a_percent);

private:
	static std::filesystem::path GetIniPath();
	void Clamp();

	float _wornWeightPercent{ 1.0f };  // 0.0 - 1.0
};




