#include "svar_add.h"
#include "svar_internal.h"

// Global mutex for all variable access (read + write)
static SemaphoreHandle_t svarMutex = NULL;

// Initialize the SVAR module. Do not use SVAR_setVar/SVAR_getVar APIs before this function
uint8_t SVAR_Init( void )
{
    uint16_t value = 0x0000;
    svAdd_t address = 0;

    // Check if EEPROM is present
    // if(sv_checkEepromPage( sizeof(setupData_t) + 1 ) == 0)       return 0;

    // Initialize the mutex if not already done
    if (svarMutex == NULL)
    {
        svarMutex = xSemaphoreCreateMutex();
        if (svarMutex == NULL)
            return 0;
        
    }

    sv_getVar_EEPROM(address, 
                    &value, 
                    sizeof(uint16_t));

    if(value != SVAR_FACTORY_BYTES_VALUE)
        sv_initSectionWrite();
    else   // Read all previously written segment data and load into RAM
        sv_initLoadSegments();

    
    return 1;
}

uint8_t SVAR_setVar (svAdd_t Address, uint8_t size, uint32_t *variable)
{
    if(variable == NULL)
        return 0; 
    if (svarMutex == NULL)
        return 0; // Mutex not initialized

        // Lock the mutex (exclusive access)
    if (xSemaphoreTake(svarMutex, SVAR_WRITE_TIMEOUT_TICKS) == pdTRUE)
    {
        uint8_t segId = sv_getSegFromAdd(Address);
        // Write to RAM
        sv_setVar_RAM(Address, (uint8_t *)variable, size);

        // Write to EEPROM (persistent)
        if (segment[segId].flavor == SHADOWED_EEPROM)
        {
            sv_setVar_EEPROM(Address, (uint8_t *)variable, size);
        }

        // Release the mutex
        xSemaphoreGive(svarMutex);
        return 1;
    }

    return 0;
}

uint8_t SVAR_getVar (svAdd_t Address, uint8_t size, uint32_t *variable)
{
    if(variable == NULL)
        return 0; 
    if (svarMutex == NULL)
        return 0; // Mutex not initialized
        
    // Lock the mutex (exclusive access)
    if (xSemaphoreTake(svarMutex, SVAR_READ_TIMEOUT_TICKS) == pdTRUE)
    {
        // Read from RAM (always consistent)
        sv_getVar_RAM(Address, (uint8_t *)variable, size);

        // If SHADOWED_EEPROM, also get fresh value from EEPROM
        // uint8_t segId = sv_getSegFromAdd(Address);
        // if (segment[segId].flavor == SHADOWED_EEPROM)
        // {
        //     sv_getVar_EEPROM(Address, (uint8_t *)variable, size);
        // }

        // Release the mutex
        xSemaphoreGive(svarMutex);
        return 1;
    }

    return 0;
}

// Register a callback to call the function if a segment's variable is accessed 
uint8_t SVAR_setReadCb( void )
{
    ;
}

// Register a callback to call the function if a segment's variable is accessed 
uint8_t SVAR_setWriteCb( void )
{
    
}
