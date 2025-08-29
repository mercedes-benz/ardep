#include "read_memory_by_address.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include <zephyr/linker/linker-defs.h>

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

    #if CONFIG_BOARD_NATIVE_SIM
        return true; // In simulation, allow all addresses
    #else
        // Check SRAM address range
        #if DT_HAS_CHOSEN(zephyr_sram) && DT_NODE_HAS_PROP(DT_CHOSEN(zephyr_sram), reg)
            #define RAM_NODE   DT_CHOSEN(zephyr_sram)
            const static uintptr_t ram_base = DT_REG_ADDR(RAM_NODE);
            const static uintptr_t ram_end = ram_base + DT_REG_SIZE(RAM_NODE);

            if (addr >= ram_base && (addr + size) <= ram_end) {
                return true;
            }
        #endif

        // Flash memory range check
        #if DT_HAS_CHOSEN(zephyr_flash) && DT_NODE_HAS_PROP(DT_CHOSEN(zephyr_flash), reg)
            #define FLASH_NODE  DT_CHOSEN(zephyr_flash)
            const static uintptr_t flash_base = DT_REG_ADDR(FLASH_NODE);
            const static uintptr_t flash_end = flash_base + DT_REG_SIZE(FLASH_NODE);

            if (addr >= flash_base && (addr + size) <= flash_end) {
                return true;
            }
        #endif

        LOG_ERR("Memory address 0x%08lX not in valid range", 
                (unsigned long)addr);
        return false;
    #endif
}

UDSErr_t handle_read_memory_by_address(struct uds_new_instance_t* instance,
                                        UDSReadMemByAddrArgs_t* args) {
    if (args->memAddr == NULL) {
        LOG_ERR("Read Memory By Address: NULL memory address");
        return UDS_NRC_RequestOutOfRange;
    }
    
    if (args->memSize == 0) {
        LOG_ERR("Read Memory By Address: Zero memory size requested");
        return UDS_NRC_RequestOutOfRange;
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
        return UDS_NRC_RequestOutOfRange;
    }
    
    LOG_DBG("Read Memory By Address: Successfully copied %zu bytes from 0x%08lX",
            args->memSize, (unsigned long)mem_addr);
    
    return UDS_PositiveResponse;
}