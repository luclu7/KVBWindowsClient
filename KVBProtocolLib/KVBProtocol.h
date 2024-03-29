#pragma once

#include <Windows.h>
#include <iostream>
#include <vector>

#define bufferSize 4

namespace KVBProtocol
{   

	constexpr int ON = 1;
	constexpr int OFF = 0;

    enum KVBPCodesReception {
        BPVAL = 0x01,
        BPMV = 0x02,
        BPFC = 0x03,
        BPTEST = 0x04,
        BPSF = 0x05,

        V1 = 0x06,
        V2 = 0x07,

        L1 = 0x08,

        D1 = 0x09,
        D2 = 0x0A,
        D3 = 0x0B
    };


    // code KVBP � envoyer
    enum KVBPCodes {
        SOL = 0x01,
        ENGIN = 0x02,
        V = 0x03,
        FU = 0x04,
        VAL = 0x05,
		MV = 0x06,
		FC = 0x07,
		LSSF = 0x08,
		visu = 0x09,
		bipSurvitesse = 0x0B,
		audio = 0x0A
    };

	// code correspondant � la valeur � envoyer
    enum KVBPVisuCodes {
        eteint = 0,             //
        tirets = 1,             // --- ---
        doubleZeroAux = 2,      //  00    
        doubleZeroPrincipal = 3,//     00
        tripleZeroAux = 4,      // 000    
		preannonce = 5,     
        preannonceAux = 6,      //      p
        vitesseSup160 = 7,      //      b
        autotest = 8,           // 888 888
        faute = 9,

        uwu = 16,
        
        PA400 = 0x0A,       //  PA 400
		UC512 = 0x0B        //  UC 512
        
    };
    

    struct Message {
        int varName;
        int varValue;
    };


    class SerialConnection
    {
    public:
        SerialConnection(LPCSTR portName, int baudRate);
        ~SerialConnection();

        bool open();
        bool close();

        bool writeData(uint8_t varName, uint8_t varValue);
        bool readData(uint8_t& ack);
        Message readProtocol();
        
        bool checkAck(uint8_t& ack);
        int bytesInSerialBuffer();

    private:
        HANDLE m_handle;
        LPCSTR m_portName;
        int m_baudRate;


        static const uint8_t header = 0x23;
        static const uint8_t acknowledgment = 0x24;
    };
}
