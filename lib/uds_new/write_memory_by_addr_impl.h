#include <iso14229.h>

#define CUSTOMUDS_WriteMemoryByAddr_SID 0x3D

struct CUSTOMUDS_WriteMemoryByAddr {
  uint32_t addr;
  const uint8_t* data;
  size_t len;

  struct {
    uint8_t memory_addr_len;
    uint8_t memory_size_len;
  } _answer_context;
};

UDSErr_t customuds_decode_write_memory_by_addr(
    UDSCustomArgs_t* data, struct CUSTOMUDS_WriteMemoryByAddr* args);

UDSErr_t customuds_answer(UDSServer_t* srv,
                          UDSCustomArgs_t* data,
                          const struct CUSTOMUDS_WriteMemoryByAddr* args);
