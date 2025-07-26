/*****************************************************************************
 * System Variables (SVAR/sv) Driver file:
 *      This file contains the functions that implement the 
 *      core logic for reading and writing SVARs into their
 *      respective memories (RAM & EEPROM).
 *****************************************************************************/
#include "svar_internal.h"

static svAdd_t sv_dataStartOffset;
// sv = short for svar (system variables)

// SVAR Set Variable in RAM
uint8_t sv_setVar_RAM(svAdd_t Address, uint8_t *value, uint8_t size)
{
    uint8_t segId = sv_getSegFromAdd(Address);
    if (segId >= svarSetup.segCount) return 0;

    uint8_t *buffer = segment[segId].buffer + (Address - segment[segId].addOffset);
    for(int i = 0; i < size; i++)
    {
        buffer[size - 1 - i] = value[i];
    }

    return 1;
}

// SVAR Get Variable from RAM
uint8_t sv_getVar_RAM(svAdd_t Address, uint8_t *value, uint8_t size)
{
    uint8_t segId = sv_getSegFromAdd(Address);
    if (segId >= svarSetup.segCount) return 0;

    uint8_t *buffer = segment[segId].buffer + (Address - segment[segId].addOffset);

    for(int i = 0; i < size; i++)
    {
        value[i] = buffer[size - 1 - i];
    }

    return 1;
}

// SVAR Set Variable in EEPROM
uint8_t sv_setVar_EEPROM(svAdd_t Address, uint8_t *value, uint8_t size)
{
    uint8_t segmentIndex = sv_getSegFromAdd(Address);
    if (segmentIndex >= svarSetup.segCount) return 0;

    uint32_t eepromAddress = sv_convLogicalAdd2EepromAdd(Address);
    if( eepromAddress == __UINT32_MAX__)    return 0;

    // Manual byte-by-byte write (Big-Endian)
    for (int i = 0; i < size; i++)
    {
        if (!sv_eepromWrite(eepromAddress + i, &value[i]))
            return 0;
    }

    return 1;
}

// SVAR Get Variable from EEPROM
uint8_t sv_getVar_EEPROM(svAdd_t Address, uint8_t *value, uint8_t size)
{
    uint8_t segmentIndex = sv_getSegFromAdd(Address);
    if (segmentIndex >= svarSetup.segCount) return 0;

    uint32_t eepromAddress = sv_convLogicalAdd2EepromAdd(Address);
    if( eepromAddress == __UINT32_MAX__)    return 0;

    // Manual byte-by-byte read (Big-Endian)
    for (int i = 0; i < size; i++)
    {
        if (!sv_eepromRead(eepromAddress + i, &value[i]))
            return 0;
    }

    return 1;
}

uint8_t sv_getSegFromAdd(svAdd_t Address)
{
    // Check all segments except the last
    for (uint8_t i = 0; i < (svarSetup.segCount - 1); i++)
    {
        if (Address >= segment[i].addOffset && 
            Address < segment[i + 1].addOffset)
        {
            return i;
        }
    }
    
    // Last segment case (inclusive)
    if (Address >= segment[svarSetup.segCount - 1].addOffset)
    {
        return (svarSetup.segCount - 1);
    }

    return 0xFF; // Invalid address (not found in any segment)
}

// Get EEPROM address from logical address
uint32_t sv_convLogicalAdd2EepromAdd(svAdd_t Address)
{
    uint8_t segmentIndex = sv_getSegFromAdd(Address);
    if (segmentIndex >= svarSetup.segCount) return __UINT32_MAX__;
    
    /*****************************************************************************
     * EEPROM Endurance consideration (cell wear-tear handling):
     *       As EEPROMs have a definite limit on write cycles on any page,
     *       when a page gets dirty (expires), the whole segment which gets 
     *       stored in EEPROM needs to be shifted to a fresh page after a 
     *       time period. This would result in change of EEPROM address of
     *       all the variables in that segment. The software should be still
     *       be able to map the changed address to the logical address without 
     *       any modifications in SVAR module.         
     * 
     *****************************************************************************/ 

    // Calculate the relative logical address within the segment
    uint32_t segmentOffset = (Address - segment[segmentIndex].addOffset);

    // Calculate the EEPROM address relative to logical address
    uint32_t eepromAddress = segment[segmentIndex].nvmOffset + segmentOffset;

    if (eepromAddress > SVAR_MAX_EEPROM_ADDRESS)    return __UINT32_MAX__;

    return eepromAddress; 
}

void sv_initSectionWrite( void )
{
    uint16_t value;
    svAdd_t address;

    address = 0;
    value = svarSetup.factoryBytes;
    sv_setVar_EEPROM(address, 
                    &value, 
                    sizeof(uint16_t));

    address += sizeof(uint16_t);
    value = svarSetup.pageSize;
    sv_setVar_EEPROM(address, 
                    &value, 
                    sizeof(uint16_t));

    address += sizeof(uint16_t);
    value = svarSetup.segCount;
    sv_setVar_EEPROM(address, 
                    &value, 
                    sizeof(uint8_t));

    address += sizeof(uint8_t);
    value = svarSetup.packedStorage_en;
    sv_setVar_EEPROM(address, 
                    &value, 
                    sizeof(uint8_t));

    // Write segment related data    
    uint8_t i = 0;
    address += sizeof(uint8_t);
    for (i = 0; i < svarSetup.segCount; i++)
    {
        // Write flavor (as enum value)
        sv_setVar_EEPROM(address, (uint8_t *)&segment[i].flavor, sizeof(storageFlavor_t));
        address += sizeof(storageFlavor_t);

        // Write offset
        sv_setVar_EEPROM(address, (uint8_t *)&segment[i].offset, sizeof(uint32_t));
        address += sizeof(uint32_t);

        // Write writeTime
        sv_setVar_EEPROM(address, (uint8_t *)&segment[i].writeTime, sizeof(uint32_t));
        address += sizeof(uint32_t);
    }

    // data will start from the next page
    sv_dataStartOffset = svarSetup.pageSize;     

    if(address > svarSetup.pageSize)
        sv_dataStartOffset += svarSetup.pageSize;
}

// Function to initialize and read setup data
void sv_initSectionRead(void)
{
    uint16_t value;
    svAdd_t address = 0;

    sv_getVar_EEPROM(address, (uint8_t *)&svarSetup.factoryBytes, sizeof(uint16_t));
    address += sizeof(uint16_t);

    sv_getVar_EEPROM(address, (uint8_t *)&svarSetup.pageSize, sizeof(uint16_t));
    address += sizeof(uint16_t);

    sv_getVar_EEPROM(address, (uint8_t *)&svarSetup.segCount, sizeof(uint8_t));
    address += sizeof(uint8_t);

    sv_getVar_EEPROM(address, (uint8_t *)&svarSetup.packedStorage_en, sizeof(uint8_t));
    address += sizeof(uint8_t);

    // Read segment metadata
    for (uint8_t i = 0; i < svarSetup.segCount; i++)
    {
        sv_getVar_EEPROM(address, (uint8_t *)&segment[i].flavor, sizeof(storageFlavor_t));
        address += sizeof(storageFlavor_t);

        sv_getVar_EEPROM(address, (uint8_t *)&segment[i].offset, sizeof(uint32_t));
        address += sizeof(uint32_t);

        sv_getVar_EEPROM(address, (uint8_t *)&segment[i].writeTime, sizeof(uint32_t));
        address += sizeof(uint32_t);
    }

    // Calculate data start offset
    sv_dataStartOffset = svarSetup.pageSize;     
    if (address > svarSetup.pageSize)
        sv_dataStartOffset += svarSetup.pageSize;
}

// Function to check integrity of an EEPROM page 
uint8_t sv_checkEepromPage( svAdd_t address )
{
    uint8_t value;
    uint8_t orgVal;
    uint8_t status = 1;
    
    sv_getVar_EEPROM(address, 
                    &orgVal, 
                    sizeof(uint8_t));

    value = 0xAA;
    sv_setVar_EEPROM(address, 
                    &value, 
                    sizeof(uint8_t));

    value = 0x00;    
    sv_getVar_EEPROM(address, 
                    &value, 
                    sizeof(uint8_t));

    if(value != 0xAA)
        return 0;
    
    value = 0x55;
    sv_setVar_EEPROM(address, 
                    &value, 
                    sizeof(uint8_t));

    value = 0x00;    
    sv_getVar_EEPROM(address, 
                    &value, 
                    sizeof(uint8_t));

    if(value != 0x55)
        return 0;
    
    //Write back the org value
    sv_setVar_EEPROM(address, 
                &orgVal, 
                sizeof(uint8_t));

    return 1;                
}

void sv_initLoadSegments( void )
{

}



