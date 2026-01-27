#include "FileManager.h"

rapidjson::Document saveData;


namespace GameSave {
	void SaveGameProgress(int levelCompleted) {
		// Update current level
		saveData["metadata"]["current_level"].SetInt(levelCompleted + 1);

		// Mark level as completed
		std::string levelKey = "level_" + std::to_string(levelCompleted);
		saveData[levelKey.c_str()]["completed"].SetBool(true);

		// Update save date
		time_t now = time(0);
		char buffer[100];
		struct tm timeinfo;
		localtime_s(&timeinfo, &now);
		strftime(buffer, sizeof(buffer), "%d-%m-%Y %H:%M:%S", &timeinfo);
		saveData["metadata"]["save_date"].SetString(buffer, strlen(buffer));

		// Write to file
		std::ofstream ofs("Assets/Data/GameSave.json");
		rapidjson::OStreamWrapper osw(ofs);
		rapidjson::Writer<rapidjson::OStreamWrapper> writer(osw);
		saveData.Accept(writer);
	}

	int GetCurrentLevel() {
		return saveData["metadata"]["current_level"].GetInt();
	}

	void LoadLevel(int levelNumber) {
		std::string levelKey = "level_" + std::to_string(levelNumber);

		if (!saveData.HasMember(levelKey.c_str())) {
			std::cout << "Level " << levelNumber << " doesn't exist!" << std::endl;
			return;
		}

		const rapidjson::Value& levelData = saveData[levelKey.c_str()];
	}
} //namespace GameSave
