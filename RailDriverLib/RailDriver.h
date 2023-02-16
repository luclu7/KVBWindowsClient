#pragma once

#include <Windows.h>
#include <iostream>
#include <vector>

namespace RailDriverClass {
    struct Control {
        int id;
        std::string name;
        float minValue, maxValue;
    };

    struct Value {
        int id;
        std::string name;
        float value;
    };

    struct LocoName {
		std::string provider;
		std::string product;
		std::string engineName;
    };


    class RailDriver {
    public:
		std::vector<Control> controls;
        
        RailDriver();
        ~RailDriver();
        static float GetCurrentControllerValue(int Control);
        static void SetControllerValue(int Control, float Value);
        static void SetRailDriverConnected(bool Value);
        static float GetControllerValue(int Control, int Mode);
		static LocoName GetLocoName();
        //static bool GetRailSimLocoChanged();
		std::vector<Control> GetControls();
        bool didValuesChange();

        float readControllerValue(std::string controlName);
        int writeControllerValue(std::string controlName, float value);
        int getControllerID(std::string controlName);
                
    private:
        static HINSTANCE hinstLib;
        
		std::vector<Value> values;

        static char*(*GetControllerList_ptr)();
        static float(*GetCurrentControllerValue_ptr)(int);
        static void(*SetControllerValue_ptr)(int, float);
        static void(*SetRailDriverConnected_ptr)(bool);
        static float(*GetControllerValue_ptr)(int, int);
        static bool(*GetRailSimLocoChanged_ptr)();
		static char*(*GetLocoName_ptr)();
		std::vector<Value> getValues();



        static char* GetControllerList();
    };
}
