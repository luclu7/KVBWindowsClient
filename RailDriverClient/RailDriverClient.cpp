// RailDriverClient.cpp : Ce fichier contient la fonction 'main'. L'exécution du programme commence et se termine à cet endroit.
//

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <map>
#include <iomanip>

#include "RailDriver.h"

std::vector<RailDriverClass::Control> readConfigFile(const std::string& filename);

int main()
{
	RailDriverClass::RailDriver rd;

	rd.setControlsFile("6500_ctrls.ini");

	rd.SetRailDriverConnected(true);

	Sleep(1000);
	
	std::vector<RailDriverClass::Control> controls = readConfigFile("6500_ctrls.ini");

	// create a map consisting of the control id and the last control value
	std::map<int, float> controlValues;
	for (auto& control : controls) {
		controlValues[control.id] = control.id;
	}

	std::cout << std::setprecision(2) << std::fixed;	

	while (true) {
		rd.SetRailDriverConnected(true);
		
		// monitore the controls
		for (auto& control : controls) {
			float value = rd.GetCurrentControllerValue(control.id);

			if (value != controlValues[control.id]) {
				// print the value as 2f float
				std::cout << "Control " << control.name << " = " << value << std::endl;
				controlValues[control.id] = value;
			}
		}

		std::cout << std::endl;
		

		Sleep(500);
	}

	
}

std::vector<RailDriverClass::Control> readConfigFile(const std::string& filename) {
	std::vector<RailDriverClass::Control> controls;

	std::ifstream file(filename);
	if (!file.is_open()) {
		std::cerr << "Error opening file " << filename << std::endl;
		return controls;
	}

	std::string line;
	while (std::getline(file, line)) {
		auto pos = line.find('=');
		if (pos != std::string::npos) {
			auto controlId = line.substr(0, pos);
			auto controlName = line.substr(pos + 1);

			std::cout << "Control " << controlId << " = " << controlName << std::endl;

			RailDriverClass::Control control;
			control.id = std::stoi(controlId);
			control.name = controlName;
			controls.push_back(control);
		}
	}

	return controls;
}