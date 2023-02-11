#include "RailDriver.h"
#include <iostream>

using namespace RailDriverClass;

HINSTANCE RailDriver::hinstLib = NULL;
char*(*RailDriver::GetControllerList_ptr)() = NULL;
float(*RailDriver::GetCurrentControllerValue_ptr)(int) = NULL;
void(*RailDriver::SetControllerValue_ptr)(int, float) = NULL;
void(*RailDriver::SetRailDriverConnected_ptr)(bool) = NULL;
float(*RailDriver::GetControllerValue_ptr)(int, int) = NULL;
bool(*RailDriver::GetRailSimLocoChanged_ptr)() = NULL;
char* (*RailDriver::GetLocoName_ptr)() = NULL;

RailDriver::RailDriver() {
    hinstLib = LoadLibraryA("C:\\Program Files (x86)\\Steam\\steamapps\\common\\RailWorks\\plugins\\RailDriver64.dll");
    if (hinstLib != NULL) {
        GetControllerList_ptr = (char*(*)())GetProcAddress(hinstLib, "GetControllerList");
        GetCurrentControllerValue_ptr = (float(*)(int))GetProcAddress(hinstLib, "GetCurrentControllerValue");
        SetControllerValue_ptr = (void(*)(int, float))GetProcAddress(hinstLib, "SetControllerValue");
        SetRailDriverConnected_ptr = (void(*)(bool))GetProcAddress(hinstLib, "SetRailDriverConnected");
        GetControllerValue_ptr = (float(*)(int, int))GetProcAddress(hinstLib, "GetControllerValue");
		GetRailSimLocoChanged_ptr = (bool(*)())GetProcAddress(hinstLib, "GetRailSimLocoChanged");
		GetLocoName_ptr = (char* (*)())GetProcAddress(hinstLib, "GetLocoName");
    }
    
	controls = GetControls();
}

RailDriver::~RailDriver() {
    if (hinstLib != NULL) {
        FreeLibrary(hinstLib);
        hinstLib = NULL;
    }
}


int RailDriver::getControllerID(std::string controlName) {
    for (int i = 0; i < controls.size(); i++) {
        if (controls[i].name == controlName) {
            return controls[i].id;
        }
    }
    return -1;
}

float RailDriver::readControllerValue(std::string controlName) {
    int controlID = getControllerID(controlName);
    if (controlID == -1) {
        return -50;
    }
    return GetCurrentControllerValue(controlID);
}

int RailDriver::writeControllerValue(std::string controlName, float value) {
    int controlID = getControllerID(controlName);
    if (controlID == -1) {
        return -50;
    }
    SetControllerValue(controlID, value);
    return 0;
}

std::vector<Value> RailDriverClass::RailDriver::getValues()
{    
	std::vector<Value> result;
	for (int i = 0; i < controls.size(); i++) {
		float value = GetCurrentControllerValue(i);
		result.push_back({ i, controls[i].name, value });
	}
	return result;
}

char* RailDriver::GetControllerList() {
    if (GetControllerList_ptr != NULL)
        return (*GetControllerList_ptr)();
    return (char*) "Error";
}

float RailDriver::GetCurrentControllerValue(int Control) {
    if (GetCurrentControllerValue_ptr != NULL)
        return (*GetCurrentControllerValue_ptr)(Control);
    return 0.0f;
}

void RailDriver::SetControllerValue(int Control, float Value) {
	if (SetControllerValue_ptr != NULL)
		(*SetControllerValue_ptr)(Control, Value);
}

void RailDriver::SetRailDriverConnected(bool Value) {
	if (SetRailDriverConnected_ptr != NULL)
		(*SetRailDriverConnected_ptr)(Value);
}

float RailDriver::GetControllerValue(int Control, int Mode) {
	if (GetControllerValue_ptr != NULL)
		return (*GetControllerValue_ptr)(Control, Mode);
	return 0.0f;
}

LocoName RailDriver::GetLocoName()
{
    if (GetLocoName_ptr == NULL)
		return { "Error", "Error", "Error"};
    
    std::string name = (std::string)(*GetLocoName_ptr)();

	std::string delimiter = "::";
	size_t pos = 0;
	std::string token;
	int index = 0;
	LocoName result;
	while ((pos = name.find(delimiter)) != std::string::npos) {
		token = name.substr(0, pos);
		switch(index) {
        case 0:
            result.provider = token;
            break;
        case 1:
			result.product = token;
            break;
        }
		name.erase(0, pos + delimiter.length());
		index++;
	}
    result.engineName = token;

	return result;
}

std::vector<Control> RailDriver::GetControls()
{
    // Call the function and store the result
	std::string controlsStr(GetControllerList());

    std::vector<Control> result;

    // separate the string by "::" into a vector
    std::string delimiter = "::";
    size_t pos = 0;
    std::string token;
    int index = 0;
    while ((pos = controlsStr.find(delimiter)) != std::string::npos) {
        token = controlsStr.substr(0, pos);
        float minVal = GetControllerValue(index, 1);
        float maxVal = GetControllerValue(index, 2);
        result.push_back({ index, token, minVal, maxVal });
        controlsStr.erase(0, pos + delimiter.length());
        index++;
    }
    float minVal = GetControllerValue(index, 1);
    float maxVal = GetControllerValue(index, 2);

    result.push_back({ index, controlsStr, minVal, maxVal });

    //std::cout << "Result: " << result << std::endl;

    // print the result
   /*
   for (int i = 0; i < result.size(); i++) {
        std::cout << "Result " << result[i].id << ": " << result[i].name << " (" << result[i].minValue << "-" << result[i].maxValue << ")" << std::endl;
    }
    */

    // Return the result
    return result;
}

bool RailDriverClass::RailDriver::didValuesChange()
{
	std::vector<Value> newValues;

    if (values.size() == 0) {
        values = getValues();
		return true;
    }    
    
	for (int i = 0; i < controls.size(); i++) {
		float value = GetCurrentControllerValue(i);
		newValues.push_back({ i, controls[i].name, value });
	}

	bool changed = false;
	for (int i = 0; i < newValues.size(); i++) {
		if (newValues[i].value != values[i].value) {
			changed = true;
			break;
		}
	}

	values = newValues;

	return changed;
}