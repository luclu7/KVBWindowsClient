#pragma once

#include <vector>
#include <string>
#include <fstream>
#include "RailDriver.h"

struct Settings {
	std::string serialPort;
	std::string controlsConfigFile;
};

Settings readSettingsFile(const std::string& filename);