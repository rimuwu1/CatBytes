#pragma once
#include "pch.h"
#include "rapidjson/document.h"
#include "rapidjson/filereadstream.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/ostreamwrapper.h"
#include "rapidjson/writer.h"
#include <fstream>
#include <iostream>
#include <sstream>

namespace GameSave
{
	void SaveGameProgress(int levelCompleted);
	int GetCurrentLevel();
	void LoadLevel(int levelNumber);
}
