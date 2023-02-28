#pragma once

#include <vector>
#include <string>
#include <fstream>
#include "RailDriver.h"

std::vector<RailDriverClass::Control> readConfigFile(const std::string& filename);
