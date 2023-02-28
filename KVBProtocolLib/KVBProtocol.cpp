#include "KVBProtocol.h"

using namespace KVBProtocol;

SerialConnection::SerialConnection(LPCSTR portName, int baudRate) : m_handle(INVALID_HANDLE_VALUE)
{
	m_portName = portName;
	m_baudRate = baudRate;
}

SerialConnection::~SerialConnection()
{
    close();
}

bool SerialConnection::open()
{
    DCB dcbSerialParams = { 0 };
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);

    /*m_handle = CreateFileA(m_portName,
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);*/

    m_handle = CreateFileA(m_portName, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);

    if (m_handle == INVALID_HANDLE_VALUE)
    {
        std::cerr << "Failed to open serial port: " << m_portName << std::endl;
        return false;
    }

    if (!GetCommState(m_handle, &dcbSerialParams))
    {
        std::cerr << "Failed to get serial port state: " << m_portName << std::endl;
        CloseHandle(m_handle);
        m_handle = INVALID_HANDLE_VALUE;
        return false;
    }

    dcbSerialParams.BaudRate = m_baudRate;
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity = NOPARITY;

    if (!SetCommState(m_handle, &dcbSerialParams))
    {
        std::cerr << "Failed to set serial port state: " << m_portName << std::endl;
        CloseHandle(m_handle);
        m_handle = INVALID_HANDLE_VALUE;
        return false;
    }

    return true;
}

bool SerialConnection::close()
{
    if (m_handle != INVALID_HANDLE_VALUE)
    {
        CloseHandle(m_handle);
        m_handle = INVALID_HANDLE_VALUE;
    }

    return true;
}

bool SerialConnection::writeData(uint8_t varName, uint8_t varValue)
{
    uint8_t buffer[bufferSize] = { header, varName, varValue, uint8_t(header+varName+varValue) };
    DWORD bytesWritten;

    if (!WriteFile(m_handle, buffer, bufferSize, &bytesWritten, NULL))
    {
        std::cerr << "Failed to write data to serial port: " << m_portName << std::endl;
        CloseHandle(m_handle);
        m_handle = INVALID_HANDLE_VALUE;
        return false;
    }
    
	std::cout << "Sent: 0x" << std::hex << (unsigned int) buffer[0] << " 0x" << std::hex << (unsigned int) buffer[1] << " 0x" << std::hex << (unsigned int) buffer[2] << " 0x" << std::hex << (unsigned int) buffer[3] << " ";

    uint8_t ack;
    if (!checkAck(ack)) {
        std::cerr << "- Wrong checksum (0x" << std::hex << (unsigned int) ack << ")" << std::endl;
        return false;
    }
	std::cout << "- Checksum OK" << std::endl;

    return true;
}

bool SerialConnection::readData(uint8_t& ack)
{
    DWORD bytesRead;

    if (!ReadFile(m_handle, &ack, sizeof(ack), &bytesRead, NULL))
    {
        std::cerr << "Failed to read data from serial port: " << m_portName << std::endl;
        CloseHandle(m_handle);
        m_handle = INVALID_HANDLE_VALUE;
        return false;
    }

    return true;
}

Message SerialConnection::readProtocol()
{
    COMSTAT comStat;
    DWORD errors;
    uint8_t buffer[bufferSize];
    DWORD bytesRead;

    if (!ClearCommError(m_handle, &errors, &comStat))
    {
        std::cerr << "Failed to get communication error status for serial port: " << m_portName << std::endl;
        return {};
    }

    //std::cout << "Nbre available: " << comStat.cbInQue << std::endl;

    if (comStat.cbInQue < bufferSize)
    {
        //std::cerr << "Not enough bytes in buffer to read protocol from serial port: " << m_portName << std::endl;
        return {};
    }

    if (!ReadFile(m_handle, buffer, bufferSize, &bytesRead, NULL))
    {
        std::cerr << "Failed to read protocol from serial port: " << m_portName << std::endl;
        CloseHandle(m_handle);
        m_handle = INVALID_HANDLE_VALUE;
        return {};
    }

	std::cout << "Header: 0x" << std::hex << (int)buffer[0] << " VarName: 0x" << std::hex << (int)buffer[1] << " VarValue: 0x" << std::hex << (int)buffer[2] << " Checksum: 0x" << std::hex << (int)buffer[3] << std::endl;
	
    if (buffer[0] != header)
	{
		std::cerr << "Wrong header" << std::endl;
		return {};
	}

	if (buffer[3] != uint8_t(buffer[0] + buffer[1] + buffer[2]))
	{
		std::cerr << "Wrong checksum" << std::endl;
		return {};
	}
        
    return {buffer[1], buffer[2]};
}

bool SerialConnection::checkAck(uint8_t& ack)
{
    readData(ack);

    return ack == 0x24;
}

int SerialConnection::bytesInSerialBuffer()
{
    DWORD dwModemStatus;
    if (!GetCommModemStatus(m_handle, &dwModemStatus))
    {
        std::cout << "Failed to get modem status" << std::endl;
        return 0;
    }

    if (dwModemStatus & MS_RING_ON)
    {
        DWORD dwBytesAvailable;
        if (!ClearCommError(m_handle, &dwBytesAvailable, NULL))
        {
            std::cout << "Failed to clear comm error" << std::endl;
            return 0;
        }

        std::cout << dwBytesAvailable << " bytes available in the serial buffer" <<std::endl;
        return dwBytesAvailable;
    }
    else
    {
        std::cout << "No bytes available in the serial buffer" << std::endl;
        return 0;
    }

}
