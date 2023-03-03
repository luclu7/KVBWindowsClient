// KVBClient.cpp : Ce fichier contient la fonction 'main'. L'exécution du programme commence et se termine à cet endroit.
//

#include <iostream>
#include "RailDriver.h"
#include "KVBProtocol.h"
#include "configFile.hpp"

#include "resource.h"


int roll_over_to_number(int targetValue, int control, RailDriverClass::RailDriver* rd);
void press_button(int btnId, RailDriverClass::RailDriver* rd);

int main(int argc, char** argv)
{
	std::cout << "KVB Windows Client v1.0 (built " << __DATE__  << " at " << __TIME__ << ")" << std::endl;


	// read settings from file
	Settings settings = readSettingsFile(".\\config.ini");
	
	std::string port = settings.serialPort;
	
	std::cout << "Using serial port " << port << std::endl;

	KVBProtocol::SerialConnection serialConnection(port.c_str(), 9600);
	serialConnection.open();
	
	RailDriverClass::RailDriver rd;
	rd.SetRailDriverConnected(true);

	Sleep(400);

	// we make our copy of the train's controls to check if TrainSim works
	std::vector<RailDriverClass::Control> TScontrols = rd.GetControls();	
	std::vector<RailDriverClass::Control> controls = rd.setControlsFile(settings.controlsConfigFile);

	bool isConnectedToTS = true;
	
	std::cout << "Found " << controls.size() << " controls in config file." << std::endl;
	std::cout << "Found " << TScontrols.size() << " controls in TrainSim." << std::endl;

	if (TScontrols.size() == 1) {
		std::cout << "No controls found. Is everything ok?" << std::endl;
		isConnectedToTS = false;
	}
	
	if (argc > 2)
	{
		std::string command = argv[2];
		if (command == "/clear") {
			// on remplis le tableau des valeurs initialement, sans les comparer
			std::cout << "Clearing all lamps..." << std::endl;
			for (int i = 1; i <= 8; i++) {
				serialConnection.writeData(i, 0);
			}
			std::cout << "Done." << std::endl;
			return EXIT_SUCCESS;
		}
		else if (command == "/set") {
			std::cout << "Setting all lamps..." << std::endl;
			for (int i = 1; i <= 8; i++) {
				serialConnection.writeData(i, 1);
			}
			std::cout << "Done." << std::endl;
			return EXIT_SUCCESS;
		}
		else if (command == "/requestTW") {
			std::cout << "Requesting TW..." << std::endl;
			serialConnection.writeData(0x0B, 0);

			while (serialConnection.bytesInSerialBuffer() == 0) {
				Sleep(100);
			}
			
			for (int i = 0; i < serialConnection.bytesInSerialBuffer()/4; i++) {
				KVBProtocol::Message msg = serialConnection.readProtocol();
				std::cout << "Received TW: " << msg.varName << std::endl;
			}
			
		}
		else if(command == "/list") {
			std::cout << "Printing all controls..." << std::endl;
			

			for (RailDriverClass::Control control : controls) {
				std::cout << control.id << ": " << control.name << " (" << control.minValue << " -> " << control.maxValue << ")" << std::endl;
			}
			return EXIT_SUCCESS;
		}
		else if (command == "/h" || command == "/help" || command == "/?") {
			std::cout << "Usage: " << argv[0] << " [/clear] [/set] [/list] [/h /help /?] <serial port>" << std::endl;
			std::cout << "The clear command shuts all the lamps down, the set command turns them all on." << std::endl;
			std::cout << "The /list command lists all of the locomotive's controls" << std::endl;
			return 0;
		}
		else {
			std::cout << "Unknown command: " << command << std::endl;
		}
	}

	// time for the arduino to wake up
	Sleep(1000);

	RailDriverClass::LocoName locoName = rd.GetLocoName();
	
	std::cout << "Locomotive " << locoName.provider << " " << locoName.product << " " << locoName.engineName << std::endl;


	// structure temporaire pour monitorer les variables
	struct ValuesMonitor {
		std::string control_name;	// nom dans TS
		uint8_t kvbp_code;			// nom dans le KVBP(tm)
		int previousValue;
	};


	// TODO: refactor ça
	std::vector<ValuesMonitor> values = {
		{"Autotest_KVB_control", 252, -1},
		{"CC6500_son_autotest_KVB_control", 0xFA, -1},

		{"KVB_LS_V_control", KVBProtocol::KVBPCodes::V, -1},
		{"KVB_LS_FU_control", KVBProtocol::KVBPCodes::FU, -1},
		{"KVB_BP_VAL_lumiere_control", KVBProtocol::KVBPCodes::VAL, -1},
		{"KVB_BP_VIO_lumiere_control", KVBProtocol::KVBPCodes::MV, -1},
		{"KVB_BP_CAR_lumiere_control", KVBProtocol::KVBPCodes::FC, -1},
		{"bip_V_control", KVBProtocol::KVBPCodes::bipSurvitesse, -1 },

		{"KVB_LS_SF_control", KVBProtocol::KVBPCodes::LSSF, -1},
		{"KVB_visu_control", KVBProtocol::KVBPCodes::visu, -1}
	};

	
	if(isConnectedToTS) {
		// on remplis le tableau des valeurs initialement, sans les comparer
		for (int i = 2; i < values.size(); i++) {
			int value = (int)rd.readControllerValue(values[i].control_name);
			serialConnection.writeData(values[i].kvbp_code, value);
			values[i].previousValue = value;
		}

		// don't forget to reset SOL/ENGIN
		serialConnection.writeData(KVBProtocol::KVBPCodes::SOL, 0);
		serialConnection.writeData(KVBProtocol::KVBPCodes::ENGIN, 0);
	}

	int lastBip = 0;
	int lastATbip = -1;
	
	while (true) {
		using namespace KVBProtocol;

		int result = -1;
		
		Message msg = serialConnection.readProtocol();
		if (msg.varName != 0) {
			//std::cout << "Received value " << msg.varName << ": " << msg.varValue << std::endl;

			switch (msg.varName) {
			case KVBPCodesReception::BPVAL: // BP-VAL
				rd.writeControllerValue("KVB_BP_VAL_off_control", (float)msg.varValue);
				std::cout << "BP VAL: " << rd.readControllerValue("KVB_BP_VAL_off_control") << std::endl;
				break;

			case KVBPCodesReception::BPMV: // BP-MV
				rd.writeControllerValue("KVB_BP_VIO_off_control", (float)msg.varValue);
				std::cout << "BP MV: " << rd.readControllerValue("KVB_BP_VIO_off_control") << std::endl;
				break;

			case KVBPCodesReception::BPFC: // BP-FC
				rd.writeControllerValue("KVB_BP_CAR_off_control", (float)msg.varValue);
				std::cout << "BP FC: " << rd.readControllerValue("KVB_BP_CAR_off_control") << std::endl;
				break;

			case KVBPCodesReception::BPTEST: // BP-TEST
				rd.writeControllerValue("KVB_BP_test_control", (float)msg.varValue);
				std::cout << "BP TEST: " << rd.readControllerValue("KVB_BP_test_control") << std::endl;
				break;


			case KVBPCodesReception::BPSF: // BP-SF
				rd.writeControllerValue("KVB_BP_SF_control", (float)msg.varValue);
				std::cout << "BP SF: " << rd.readControllerValue("KVB_BP_SF_control") << std::endl;
				break;

			case KVBPCodesReception::V1:
				if (isConnectedToTS)
					result = roll_over_to_number(msg.varValue, rd.getControllerID("KVB_V_cent_control"), &rd);
				std::cout << "V1: " << (result == -1 ? msg.varValue : result) << std::endl;

				break;
			
			case KVBPCodesReception::V2:
				if (isConnectedToTS)
					result = roll_over_to_number(msg.varValue, rd.getControllerID("KVB_V_diz_control"), &rd);
				std::cout << "V2: " << (result == -1 ? msg.varValue : result) << std::endl;
				break;
			
			case KVBPCodesReception::L1:
				if (isConnectedToTS)
					result = roll_over_to_number(msg.varValue, rd.getControllerID("KVB_L_control"), &rd);
				std::cout << "L1: " << (result == -1 ? msg.varValue : result) << std::endl;
				break;
				
			case KVBPCodesReception::D2:
				if (isConnectedToTS)
					result = roll_over_to_number(msg.varValue, rd.getControllerID("KVB_D_diz_control"), &rd);
				std::cout << "D2: " << (result == -1 ? msg.varValue : result) << std::endl;
				break;
			
			case KVBPCodesReception::D3:
				if (isConnectedToTS)
					result = roll_over_to_number(msg.varValue, rd.getControllerID("KVB_D_unit_control"), &rd);
				std::cout << "D3: " << (result == -1 ? msg.varValue : result) << std::endl;
				break;

			case 0x0c:
				if (isConnectedToTS)
					rd.writeControllerValue("KVB_potar_control", (float)msg.varValue);
				std::cout << "Received 0x0c:";
				switch (msg.varValue) {
				case 0x00:
					std::cout << "  - 0x00: VO" << std::endl;
					break;
				case 0x01:
					std::cout << "  - 0x01: ME" << std::endl;
					break;
				case 0x02:
					std::cout << "  - 0x02: MA" << std::endl;
					break;
				}
				break;
			}
		}

		if (!isConnectedToTS) {
			break;
		}

		// pour gérer l'autotest, on doit le lire en premier
		int autotest = (int)rd.readControllerValue("Autotest_KVB_control");
		
		if (autotest != values[0].previousValue) {
			if (autotest == 0 && (values[0].previousValue == 2 || values[0].previousValue == 3)) {
				serialConnection.writeData(KVBProtocol::KVBPCodes::ENGIN, 0);
				serialConnection.writeData(KVBProtocol::KVBPCodes::SOL, 0);

				values[0].previousValue = 0;
			}
		}

		int kvb_autotest_sound = (int)rd.readControllerValue("CC6500_son_autotest_KVB_control");
		if (kvb_autotest_sound != values[1].previousValue) {
			if (kvb_autotest_sound == 1) {
				serialConnection.writeData(0x0A, 3);
			}
			std::cout << "KVB Autotest sound: " << kvb_autotest_sound << std::endl;
			values[1].previousValue = kvb_autotest_sound;
		}


		// special case for autotest		
		// autotest goes 0 -> 1 (PA400, all lights off) -> 2 (UC512, SOL/ENGIN turned on) -> 0
		// autotest=3 when the test btn is pressed
		if (autotest != 0 && autotest < 3)
		{
			if (autotest != values[0].previousValue) {
				//std::cout << "                  Autotest: " << autotest << std::endl;

				using namespace KVBProtocol;
				switch (autotest)
				{
				case 1:

					serialConnection.writeData(KVBPCodes::visu, KVBPVisuCodes::PA400);
					for (uint8_t i = 1; i <= 4; i++)
					{
						serialConnection.writeData(i, OFF);
					}
					break;

				case 2:
					serialConnection.writeData(KVBPCodes::visu, KVBPVisuCodes::UC512);

					serialConnection.writeData(KVBPCodes::ENGIN, ON);
					serialConnection.writeData(KVBPCodes::SOL, ON);
					serialConnection.writeData(KVBPCodes::FU, OFF);
					serialConnection.writeData(KVBPCodes::V, OFF);
					break;

				case 3:
					serialConnection.writeData(KVBPCodes::visu, KVBPVisuCodes::autotest);

					serialConnection.writeData(KVBPCodes::ENGIN, OFF);
					serialConnection.writeData(KVBPCodes::SOL, OFF);
					serialConnection.writeData(KVBPCodes::FU, OFF);
					serialConnection.writeData(KVBPCodes::V, OFF);
					
				}

				values[0].previousValue = autotest;
			}

			// LSSF blinks during the autotest, so we need to check it manually
			int value = (int)rd.readControllerValue(values[7].control_name); // LSSF
			if (value != values[7].previousValue) {
				serialConnection.writeData(values[7].kvbp_code, value);
				values[7].previousValue = value;
			}


		}
		else
		{			
			// on parcours la liste des valeurs à monitorer, on lis leur valeur, la compare avec l'ancienne, et si elle est différente on l'envoie en série
			int autotest = (int)rd.readControllerValue(values[0].control_name);
			if (autotest != values[0].previousValue) {
				if (autotest == 3) {
					serialConnection.writeData(KVBProtocol::KVBPCodes::ENGIN, 1);
					serialConnection.writeData(KVBProtocol::KVBPCodes::SOL, 1);
				}

				values[0].previousValue = autotest;
			}

			for (int i = 2; i < values.size(); i++) {
				int value = (int)rd.readControllerValue(values[i].control_name);
				if (value != values[i].previousValue) {
					serialConnection.writeData(values[i].kvbp_code, value);
					values[i].previousValue = value;
				}
			}
		}

		int bip_v = (int)rd.readControllerValue("bip_V_control");
		if (bip_v != lastBip && bip_v != -50) {
			std::cout << "bip_V_control: " << bip_v << std::endl;
			serialConnection.writeData(0x0B, bip_v);
			lastBip = bip_v;
		}
		
		// temporisation minimum entre les cycles
		// à supprimer?
		Sleep(20);
	}
}


int roll_over_to_number(int targetValue, int control, RailDriverClass::RailDriver* rd)
{
	int minusButton = control - 2;
	int plusButton = control - 1;

	if (targetValue < 0 || targetValue > 9) return -1;  // targetValue is not a valid number (0-9)

	int A = int(float(rd->GetCurrentControllerValue(control)));  // Read the current state of A

	if (A == targetValue) {
		return A;  // A is already equal to targetValue, no need to do anything
	}

	if (A == 0 && targetValue == 9) {
		press_button(minusButton, rd);
		A = int(float(rd->GetCurrentControllerValue(control)));  // Read the current state of A
		return A;
	}
	else if (A == 9 && targetValue == 0) {
		press_button(plusButton, rd);
		A = int(float(rd->GetCurrentControllerValue(control)));  // Read the current state of A
		return A;
	}

	int difference = targetValue - A;

	if (difference > 0) {
		// Press the plus button
		for (int i = 0; i < difference; i++) {
			press_button(plusButton, rd);
			A = int(float(rd->GetCurrentControllerValue(control)));  // Read the current state of A
		}
	}
	else {
		// Press the minus button
		for (int i = 0; i < -difference; i++) {
			press_button(minusButton, rd);
			A = int(float(rd->GetCurrentControllerValue(control)));  // Read the current state of A
		}
	}

	return A;
}

void press_button(int btnId, RailDriverClass::RailDriver* rd)
{
	DWORD sleepTime = 50;

	rd->SetControllerValue(btnId, 1.0f);
	Sleep(sleepTime);
	rd->SetControllerValue(btnId, 0.0f);
	Sleep(sleepTime);
}