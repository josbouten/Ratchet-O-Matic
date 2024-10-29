#ifndef _EEPROM
#define _EEPROM

#define MARKER ((unsigned long) 0x66666666 )           // The MARKER measures 4 bytes in size.
#define ERASED_MARKER ((unsigned long) 0x33333333 )
#define DELAY_TIME_BEFORE_WRITING_TO_EEPROM_IN_MS 2000 // Time in milli seconds.

// Note, the size of the SettingsObjType_t struct in bytes must be an integer
// multiple of the size of MARKER !

#include <EEPROM.h>
#include "Debug.hpp"
class Eeprom {
    private:
        // Start address of memory struct in eeprom.
        unsigned int _writeAddress;
        unsigned int _readAddress;
        // Size of EEPROM in bytes.
        unsigned int _sizeOfEepromInBytes;
        unsigned long _dataItem;
        bool writeToEeprom;
        unsigned long writeTimer;

        void eraseMarkerByte() {
            debug_print2("eeprom: erasing MARKER at address: %d\n", _readAddress);
            EEPROM.put(_readAddress, ERASED_MARKER);
        }

        // Set startAddress for writing data to.
        void setStartAddress(unsigned int address) {
            debug_print2("eeprom: set start address to: %d\n", address);
            _writeAddress = address;
        }

        int getSize() {
            return(_sizeOfEepromInBytes);
        }

        void init() {
            // Look for marker byte in eeprom and store its memory address.
            // This is the address preceding the data that was stored into the eeprom the last time.
            for (unsigned int address = 0; address < _sizeOfEepromInBytes - sizeof(MARKER); address += sizeof(MARKER)) {
                EEPROM.get(address, _dataItem);
                //debug_print3(address, " ", _dataItem);
                if (_dataItem == MARKER) {
                    _readAddress = address;
                    _writeAddress = _readAddress + sizeof(SettingsObjType) + sizeof(MARKER);
                    debug_print2("eeprom: init found start address for reading: %d\n", _readAddress);
                    return;
                }
            }
            writeToEeprom = false;
            writeTimer = millis();
            // If we get to here, no marker byte was found. So we are dealing with an empty EEPROM.
            _writeAddress = 0;
            _readAddress = 0;
            debug_print("eeprom: init of eeprom object finished at start of eeprom.\n");
        }


    public:

        Eeprom() {}

        Eeprom(unsigned int length): _sizeOfEepromInBytes(length) {
            debug_print2("eeprom: size of eeprom: %d\n", _sizeOfEepromInBytes);
            // Find or set the _writeAddress.
            init();
        }

        // Check whether eeprom is empty or not.
        bool isEmpty() {
            if (_writeAddress == _readAddress) {
                return(true);
            } else {
                return(false);
            }
        }

        // Return address to start reading.
        int getReadAddress() {
            return(_readAddress);
        }

        int getWriteAddress() {
            return(_writeAddress);
        }

        // Write data to the EEPROM.
        // Returns the nr of data bytes written including one marker byte.
        int write(SettingsObjType_t settings) {
            // Will return nr of databytes written including one marker byte.
            if ((sizeof(SettingsObjType_t) + sizeof(MARKER)) > _sizeOfEepromInBytes) {
                //debug_print3("eeprom: data chunk size (+ marker): %d too large for this EEPROM (%d)!\n", sizeof(SettingsObjType_t) + sizeof(MARKER), _sizeOfEepromInBytes);
                return(-1);
            }
            // Erase marker byte.
            eraseMarkerByte();
            // Jump over old data and calculate the new start address for writing the data.
            if ((_writeAddress + sizeof(SettingsObjType_t) + sizeof(MARKER)) > (_sizeOfEepromInBytes - sizeof(SettingsObjType_t) - sizeof(MARKER))) {
                // There is insufficient room near the end of the eeprom.
                debug_print("eeprom: can not write data past the end of the EEPROM; beginning at address 0.\n");
                resetStartAddress();
            }
            // Write new marker followed by the data.
            debug_print2("eeprom: writing MARKER to address: %d\n", _writeAddress);
            EEPROM.put(_writeAddress, MARKER);
            debug_print3("eeprom: writing data from address: %d until %d\n", _writeAddress + sizeof(MARKER), _writeAddress + sizeof(MARKER) + sizeof(SettingsObjType_t));
            EEPROM.put(_writeAddress + sizeof(MARKER), settings);
            _readAddress = _writeAddress;
            _writeAddress += sizeof(SettingsObjType_t) + sizeof(MARKER);
            return(sizeof(SettingsObjType_t) + sizeof(MARKER));
        }

        // Set startAddress for writing data to 0.
        void resetStartAddress() {
            setStartAddress(0);
        }

        // Read the data chunk follwing the startAddress.
        // Will return the nr of data read or -1.
        int read(SettingsObjType_t *p) {
            EEPROM.get(_readAddress, _dataItem);
            if (_dataItem == MARKER) {
                EEPROM.get(_readAddress + sizeof(MARKER), *p);
                return(sizeof(SettingsObjType_t));
            } else {
                return(-1);
            }
        }

        void writeSettings(void) {
            debug_print("-");
            writeToEeprom = true;
            writeTimer = millis() + DELAY_TIME_BEFORE_WRITING_TO_EEPROM_IN_MS;
        }

        void tick(void) {
            if ((writeToEeprom == true) && (millis() > writeTimer)) {
                writeToEeprom = false;
                #ifdef WRITE_TO_EEPROM
                    debug_print2("\neeprom: writing settings (%d bytes) to EEPROM.", sizeof(SettingsObjType));
                    if (write(settings) < 0) {
                        debug_print("eeprom: error writing data to eeprom. Data does not fit in eeprom.\n");
                    } else {
                        debug_print2("eeprom: read  address after writing: %d\n", getReadAddress());
                        debug_print2("eeprom: write address after writing: %d\n", getWriteAddress());
                    }
                    debug_print3("eeprom: device mode: %d %s\n", settings.device_mode, mode_str[settings.device_mode].c_str());
                    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
                    delay(100);
                    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
                #else
                    debug_print("\neeprom: skipping writing settings to EEPROM.");
                #endif
            }
        }

};
#endif