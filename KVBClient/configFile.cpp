#include "configFile.hpp"

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