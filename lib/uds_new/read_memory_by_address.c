#include "read_memory_by_address.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_DECLARE(uds_new, CONFIG_UDS_NEW_LOG_LEVEL);

/**
 * Check if the requested memory address range is within valid memory bounds
 * @param addr Starting address to check
 * @param size Size of memory region to check
 * @return true if address range is valid, false otherwise
 */
static bool is_memory_address_valid(uintptr_t addr, size_t size) {
    // Prevent overflow
    if (addr > UINTPTR_MAX - size) {
        LOG_ERR("Memory address overflow detected");
        return false;
    }
    
    // Don't allow NULL pointer access
    if (addr == 0) {
        LOG_ERR("Memory address is NULL");
        return false;
    }
    
    #ifdef CONFIG_BOARD_NATIVE_SIM
        return true; // Allow most reasonable addresses on simulator
    #else
        // Check if address is within RAM range
        uintptr_t ram_start = (uintptr_t)_image_ram_start;
        uintptr_t ram_end = (uintptr_t)_image_ram_end;

        if (addr >= ram_start && (addr + size) <= ram_end) {
            return true;
        }

        // Check if address is within Flash range  
        uintptr_t flash_start = (uintptr_t)__rom_region_start;
        uintptr_t flash_end = (uintptr_t)__rom_region_end;

        if (addr >= flash_start && (addr + size) <= flash_end) {
            return true;
        }

        LOG_ERR("Memory address 0x%08lX not in valid RAM or Flash range", 
                (unsigned long)addr);
        return false;
    #endif
}

UDSErr_t handle_read_memory_by_address(struct uds_new_instance_t* instance,
                                        UDSReadMemByAddrArgs_t* args) {
    // Validate input parameters
    if (args == NULL) {
        LOG_ERR("Read Memory By Address: NULL arguments");
        return UDS_NRC_GeneralProgrammingFailure;
    }
    
    if (args->memAddr == NULL) {
        LOG_ERR("Read Memory By Address: NULL memory address");
        return UDS_NRC_RequestOutOfRange;
    }
    
    if (args->memSize == 0) {
        LOG_ERR("Read Memory By Address: Zero memory size requested");
        return UDS_NRC_RequestOutOfRange;
    }
    
    if (args->copy == NULL) {
        LOG_ERR("Read Memory By Address: NULL copy function");
        return UDS_NRC_GeneralProgrammingFailure;
    }
    
    // Get the memory address as integer for range checking
    uintptr_t mem_addr = (uintptr_t)args->memAddr;
    
    LOG_DBG("Read Memory By Address: addr=0x%08lX, size=%zu", 
            (unsigned long)mem_addr, args->memSize);
    
    // Validate memory address range
    if (!is_memory_address_valid(mem_addr, args->memSize)) {
        LOG_WRN("Read Memory By Address: Invalid address range 0x%08lX-0x%08lX",
                (unsigned long)mem_addr, 
                (unsigned long)(mem_addr + args->memSize - 1));
        return UDS_NRC_RequestOutOfRange;
    }
    
    // Copy the requested memory to response buffer
    uint8_t copy_result = args->copy(&instance->iso14229.server, args->memAddr, args->memSize);
    if (copy_result != UDS_PositiveResponse) {
        LOG_ERR("Read Memory By Address: Copy failed with result %d", copy_result);
        return UDS_NRC_GeneralProgrammingFailure;
    }
    
    LOG_DBG("Read Memory By Address: Successfully copied %zu bytes from 0x%08lX",
            args->memSize, (unsigned long)mem_addr);
    
    return UDS_PositiveResponse;
}