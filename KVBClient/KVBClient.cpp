// KVBClient.cpp : Ce fichier contient la fonction 'main'. L'exécution du programme commence et se termine à cet endroit.
//

#include <iostream>
#include "RailDriver.h"
#include "KVBProtocol.h"

int main()
{
    std::cout << "Hello World!\n";

    KVBProtocol::SerialConnection serialConnection("COM3", 9600);
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
		float previousValue;
	};


	// TODO: refactor ça
	std::vector<ValuesMonitor> values = {
		{"Autotest_KVB_control", 0x0A, -1},

		{"KVB_LS_V_control", 0x03, -1},
		{"KVB_LS_FU_control", 0x04, -1},
		{"KVB_BP_VAL_lumiere_control", 0x05, -1},
		{"KVB_BP_VIO_lumiere_control", 0x06, -1},
		{"KVB_BP_CAR_lumiere_control", 0x07, -1},

		{"KVB_LS_SF_control", 0x08, -1},
		{"KVB_visu_control", 0x09, -1}
	};

	// on remplis le tableau des valeurs initialement, sans les comparer
	for (int i = 0; i < values.size(); i++) {
		int value = (int)rd.readControllerValue(values[i].control_name);
		serialConnection.writeData(values[i].kvbp_code, value);
		values[i].previousValue = value;
	}

	while (true) {

		KVBProtocol::Message msg = serialConnection.readProtocol();
		if (msg.varName != 0) {
			std::cout << "Received value " << msg.varName << ": " << msg.varValue << std::endl;

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

		// on parcours la liste des valeurs à monitorer, on lis leur valeur, la compare avec l'ancienne, et si elle est différente on l'envoie en série
		for (int i = 0; i < values.size(); i++) {
			int value = (int)rd.readControllerValue(values[i].control_name);
			if (value != values[i].previousValue) {
				serialConnection.writeData(values[i].kvbp_code, value);
				values[i].previousValue = value;
			}
		}
		
		// temporisation minimum entre les cycles
		// à supprimer?
		Sleep(50);
	}    
}