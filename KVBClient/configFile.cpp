#include "configFile.hpp"

Settings readSettingsFile(const std::string& filename)
{
	// read settings from file
	Settings settings;


	// check if file exists
	GetFileAttributesA(filename.c_str()); // from winbase.h
	if (INVALID_FILE_ATTRIBUTES == GetFileAttributesA(filename.c_str()) && GetLastError() == ERROR_FILE_NOT_FOUND)
	{
		std::cerr << "Error opening file " << filename << std::endl << "Using default values." << std::endl;
		
		std::cout << "Creating file " << filename << "..." << std::endl;

		// create file
		std::ofstream file(filename);
		if (!file.is_open()) {
			std::cerr << "Error creating file " << filename << std::endl;
			return settings;
		}
		// write default values
		file << "[Settings]" << std::endl;
		file << "SerialPort=COM3" << std::endl;
		file << "ControlsConfigFile=6500_ctrls.ini" << std::endl;

		// close file
		file.close();

		std::cout << "File " << filename << " created with default values." << std::endl;
	}

	// read settings from file
	const int bufferSize = 256;
	char buffer[bufferSize];

	GetPrivateProfileStringA("Settings", "SerialPort", "COM3", buffer, bufferSize, filename.c_str());
	settings.serialPort = buffer;

	GetPrivateProfileStringA("Settings", "ControlsConfigFile", "6500_ctrls.ini", buffer, bufferSize, filename.c_str());
	settings.controlsConfigFile = buffer;

	return settings;
}
