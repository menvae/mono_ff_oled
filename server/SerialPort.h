#ifndef MONO_FF_OLED_SERIALPORT_H
#define MONO_FF_OLED_SERIALPORT_H

#include <cstdint>
#include <windows.h>
#include <string>
#include <vector>

class SerialPort {
public:
    /**
     * @param portName   e.g. "COM4"
     * @param baudRate   Baud rate (default CBR_9600)
     * @param byteSize   Number of data bits (default 8)
     * @param stopBits   Stop bits (default ONESTOPBIT)
     * @param parity     Parity (default NOPARITY)
     */
    SerialPort(const std::string& portName,
               DWORD baudRate = CBR_9600,
               BYTE byteSize = 8,
               BYTE stopBits = ONESTOPBIT,
               BYTE parity = NOPARITY)
        : hSerial(INVALID_HANDLE_VALUE) {
        
        // Auto fix for Windows COM ports 10 and above
        std::string safePortName = portName;
        if (safePortName.find("\\\\.\\") == std::string::npos) {
            safePortName = "\\\\.\\" + safePortName;
        }

        open(safePortName, baudRate, byteSize, stopBits, parity);
    }

    ~SerialPort() {
        close();
    }

    SerialPort(const SerialPort&) = delete;
    SerialPort& operator=(const SerialPort&) = delete;

    bool isOpen() const {
        return hSerial != INVALID_HANDLE_VALUE;
    }

    void close() {
        if (hSerial != INVALID_HANDLE_VALUE) {
            CloseHandle(hSerial);
            hSerial = INVALID_HANDLE_VALUE;
        }
    }

    bool write(const uint8_t* buffer, size_t size) {
        if (!isOpen()) {
            lastError = "Serial port not open";
            return false;
        }

        DWORD bytesWritten = 0;
        if (!WriteFile(hSerial, buffer, static_cast<DWORD>(size), &bytesWritten, NULL)) {
            lastError = "WriteFile failed";
            return false;
        }

        if (bytesWritten != size) {
            lastError = "Partial write: only " +
                        std::to_string(bytesWritten) + " bytes written";
            return false;
        }
        return true;
    }

    bool write(const std::vector<uint8_t>& data) {
        return write(data.data(), data.size());
    }

    bool write(const std::string& data) {
        if (!isOpen()) {
            lastError = "Serial port not open";
            return false;
        }

        DWORD bytesWritten = 0;
        if (!WriteFile(hSerial, data.c_str(),
                       static_cast<DWORD>(data.length()), &bytesWritten, NULL)) {
            lastError = "WriteFile failed";
            return false;
        }

        if (bytesWritten != data.length()) {
            lastError = "Partial write: only " +
                        std::to_string(bytesWritten) + " bytes written";
            return false;
        }
        return true;
    }

    bool read(std::string& data, DWORD maxBytes, DWORD timeoutMs = 1000) {
        if (!isOpen()) {
            lastError = "Serial port not open";
            return false;
        }

        // Set read timeout
        COMMTIMEOUTS timeouts;
        if (!GetCommTimeouts(hSerial, &timeouts)) {
            lastError = "Failed to get timeouts";
            return false;
        }
        timeouts.ReadIntervalTimeout = MAXDWORD;
        timeouts.ReadTotalTimeoutMultiplier = MAXDWORD;
        timeouts.ReadTotalTimeoutConstant = timeoutMs;
        if (!SetCommTimeouts(hSerial, &timeouts)) {
            lastError = "Failed to set timeouts";
            return false;
        }

        char* buffer = new char[maxBytes];
        DWORD bytesRead = 0;
        BOOL result = ReadFile(hSerial, buffer, maxBytes, &bytesRead, NULL);

        if (!result) {
            delete[] buffer; // delete on failure
            lastError = "ReadFile failed";
            return false;
        }

        if (bytesRead == 0) {
            delete[] buffer; // delete on timeout
            lastError = "Read timeout";
            return false;
        }

        data.assign(buffer, bytesRead);
        delete[] buffer; // delete AFTER data has been assigned
        return true;
    }

    [[nodiscard]] std::string getLastError() const {
        return lastError;
    }

private:
    HANDLE hSerial;
    std::string lastError;

    bool open(const std::string& portName,
              DWORD baudRate,
              BYTE byteSize,
              BYTE stopBits,
              BYTE parity) {
        close();

        hSerial = CreateFile(portName.c_str(),
                             GENERIC_READ | GENERIC_WRITE,
                             0,
                             NULL,
                             OPEN_EXISTING,
                             FILE_ATTRIBUTE_NORMAL,
                             NULL);

        if (hSerial == INVALID_HANDLE_VALUE) {
            lastError = "Failed to open " + portName;
            return false;
        }

        if (!configure(baudRate, byteSize, stopBits, parity)) {
            close();
            return false;
        }

        COMMTIMEOUTS timeouts = {0};
        timeouts.ReadIntervalTimeout = 50;
        timeouts.ReadTotalTimeoutConstant = 50;
        timeouts.ReadTotalTimeoutMultiplier = 10;
        timeouts.WriteTotalTimeoutConstant = 50;
        timeouts.WriteTotalTimeoutMultiplier = 10;
        if (!SetCommTimeouts(hSerial, &timeouts)) {
            lastError = "Failed to set timeouts";
            close();
            return false;
        }

        return true;
    }

    bool configure(DWORD baudRate, BYTE byteSize, BYTE stopBits, BYTE parity) {
        DCB dcbSerialParams = {0};
        dcbSerialParams.DCBlength = sizeof(dcbSerialParams);

        if (!GetCommState(hSerial, &dcbSerialParams)) {
            lastError = "Failed to get current serial parameters";
            return false;
        }

        dcbSerialParams.BaudRate = baudRate;
        dcbSerialParams.ByteSize = byteSize;
        dcbSerialParams.StopBits = stopBits;
        dcbSerialParams.Parity   = parity;
        
        dcbSerialParams.fDtrControl = DTR_CONTROL_ENABLE;
        dcbSerialParams.fRtsControl = RTS_CONTROL_ENABLE;
        
        dcbSerialParams.fOutX = FALSE;
        dcbSerialParams.fInX = FALSE;

        if (!SetCommState(hSerial, &dcbSerialParams)) {
            lastError = "Failed to set serial parameters";
            return false;
        }

        return true;
    }
};

#endif //MONO_FF_OLED_SERIALPORT_H