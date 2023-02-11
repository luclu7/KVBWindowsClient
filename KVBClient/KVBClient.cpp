// KVBClient.cpp : Ce fichier contient la fonction 'main'. L'exécution du programme commence et se termine à cet endroit.
//

#include <iostream>
#include "RailDriver.h"
#include "KVBProtocol.h"

#include "resource.h"

int main(int argc, char** argv)
{
    std::cout << "KVB Windows Client v1.0 !" << std::endl;

	std::string port = "COM2";

	if (argc > 1) {
		port = argv[1];
	}

	std::cout << "Using serial port " << port << std::endl;

    KVBProtocol::SerialConnection serialConnection(port.c_str(), 9600);
	RailDriverClass::RailDriver rd;

	serialConnection.open();

	rd.SetRailDriverConnected(true);

	// time for the arduino to wake up
	Sleep(1000);

	RailDriverClass::LocoName locoName = rd.GetLocoName();
	
	std::cout << "Locomotive " << locoName.provider << " " << locoName.product << " " << locoName.engineName << std::endl;

	// we make our copy of the train's controls to display it, for debugging purposes
	std::vector<RailDriverClass::Control> controls = rd.GetControls();

	for (RailDriverClass::Control control : controls) {
		std::cout << control.id << ": " << control.name << " (" << control.minValue << " -> " << control.maxValue << ")" << std::endl;
	}


	// structure temporaire pour monitorer les variables
	struct ValuesMonitor {
		std::string control_name;	// nom dans TS
		uint8_t kvbp_code;			// nom dans le KVBP(tm)
		int previousValue;
	};


	// TODO: refactor ça
	std::vector<ValuesMonitor> values = {
		{"Autotest_KVB_control", 0x0A, -1},

		{"KVB_LS_V_control", KVBProtocol::KVBPCodes::V, -1},
		{"KVB_LS_FU_control", KVBProtocol::KVBPCodes::FU, -1},
		{"KVB_BP_VAL_lumiere_control", KVBProtocol::KVBPCodes::VAL, -1},
		{"KVB_BP_VIO_lumiere_control", KVBProtocol::KVBPCodes::MV, -1},
		{"KVB_BP_CAR_lumiere_control", KVBProtocol::KVBPCodes::FC, -1},

		{"KVB_LS_SF_control", KVBProtocol::KVBPCodes::LSSF, -1},
		{"KVB_visu_control", KVBProtocol::KVBPCodes::visu, -1}
	};

	// on remplis le tableau des valeurs initialement, sans les comparer
	for (int i = 0; i < values.size(); i++) {
		int value = (int)rd.readControllerValue(values[i].control_name);
		serialConnection.writeData(values[i].kvbp_code, value);
		values[i].previousValue = value;
	}

	// don't forget to reset SOL/ENGIN
	serialConnection.writeData(KVBProtocol::KVBPCodes::SOL, 0);
	serialConnection.writeData(KVBProtocol::KVBPCodes::ENGIN, 0);

	while (true) {

		KVBProtocol::Message msg = serialConnection.readProtocol();
		if (msg.varName != 0) {
			//std::cout << "Received value " << msg.varName << ": " << msg.varValue << std::endl;

			switch (msg.varName) {
			case 0x01: // BP-VAL
				rd.writeControllerValue("KVB_BP_VAL_off_control", (float)msg.varValue);
				std::cout << "BP VAL: " << rd.readControllerValue("KVB_BP_VAL_off_control") << std::endl;
				break;

			case 0x02: // BP-MV
				rd.writeControllerValue("KVB_BP_VIO_off_control", (float)msg.varValue);
				std::cout << "BP MV: " << rd.readControllerValue("KVB_BP_VIO_off_control") << std::endl;
				break;

			case 0x03: // BP-FC
				rd.writeControllerValue("KVB_BP_CAR_off_control", (float)msg.varValue);
				std::cout << "BP FC: " << rd.readControllerValue("KVB_BP_CAR_off_control") << std::endl;
				break;

			case 0x04: // BP-TEST
				rd.writeControllerValue("KVB_BP_test_control", (float)msg.varValue);
				std::cout << "BP TEST: " << rd.readControllerValue("KVB_BP_test_control") << std::endl;
				break;


			case 0x05: // BP-SF
				rd.writeControllerValue("KVB_BP_SF_control", (float)msg.varValue);
				std::cout << "BP SF: " << rd.readControllerValue("KVB_BP_SF_control") << std::endl;
				break;

			}
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
			int value = (int)rd.readControllerValue(values[6].control_name); // LSSF
			if (value != values[6].previousValue) {
				serialConnection.writeData(values[6].kvbp_code, value);
				values[6].previousValue = value;
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

			for (int i = 1; i < values.size(); i++) {
				int value = (int)rd.readControllerValue(values[i].control_name);
				if (value != values[i].previousValue) {
					serialConnection.writeData(values[i].kvbp_code, value);
					values[i].previousValue = value;
				}
			}
		}
		
		// temporisation minimum entre les cycles
		// à supprimer?
		Sleep(20);
	}
}