#pragma once

#include <Windows.h>
#include <iostream>
#include <vector>

#define bufferSize 4

namespace KVBProtocol
{   
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
        
        bool checkAck();
        int bytesInSerialBuffer();

    private:
        HANDLE m_handle;
        LPCSTR m_portName;
        int m_baudRate;


        static const uint8_t header = 0x23;
        static const uint8_t acknowledgment = 0x24;
    };
}
