#ifndef ARDEP_UDS_NEW_H
#define ARDEP_UDS_NEW_H

// Forward declaration to avoid include dependency issues
#include "ardep/iso14229.h"

#include <iso14229.h>

struct uds_new_instance_t;

enum ecu_reset_type {
  ECU_RESET_HARD = 1,
  ECU_RESET_KEY_OFF_ON = 2,
  ECU_RESET_ENABLE_RAPID_POWER_SHUT_DOWN = 4,
  ECU_RESET_DISABLE_RAPID_POWER_SHUT_DOWN = 5,
  ECU_RESET_VEHICLE_MANUFACTURER_SPECIFIC_START = 0x40,
  ECU_RESET_VEHICLE_MANUFACTURER_SPECIFIC_END = 0x5F,
  ECU_RESET_SYSTEM_SUPPLIER_SPECIFIC_START = 0x60,
  ECU_RESET_SYSTEM_SUPPLIER_SPECIFIC_END = 0x7E,
};

/**
 * @brief Callback type for ECU reset events
 *
 * @param inst Pointer to the UDS server instance
 * @param reset_type Type of reset to perform
 * @param user_context User-defined context pointer as passed to \ref
 * uds_new_init()
 */
typedef UDSErr_t (*ecu_reset_callback_t)(struct uds_new_instance_t* inst,
                                         enum ecu_reset_type reset_type,
                                         void* user_context);

/**
 * Set the ECU reset callback function for custom callbacks
 *
 * @param inst Pointer to the UDS server instance
 * @param callback Pointer to the callback function to set
 * @return 0 on success, negative error code on failure
 */
typedef int (*set_ecu_reset_callback_fn)(struct uds_new_instance_t* inst,
                                         ecu_reset_callback_t callback);

#ifdef CONFIG_UDS_NEW_USE_DYNAMIC_DATA_BY_ID
/**
 * @brief Register a data identifier for the data at @p addr.
 *
 * This macro registers a data identifier at runtime, associating an id
 * with a memory address so it can be read by the <read_data_by_identifier>
 * command.
 *
 * See UDS_NEW_REGISTER_DATA_IDENTIFIER_STATIC_MEM for details.
 */
typedef int (*register_data_by_identifier_fn)(struct uds_new_instance_t* inst,
                                              uint16_t data_id,
                                              void* addr,
                                              size_t len,
                                              size_t len_elem);
#endif  // CONFIG_UDS_NEW_USE_DYNAMIC_DATA_BY_ID

struct uds_new_instance_t {
  struct iso14229_zephyr_instance iso14229;
  struct uds_new_registration_t* static_registrations;
  void* user_context;

#ifdef CONFIG_UDS_NEW_USE_DYNAMIC_DATA_BY_ID
  struct uds_new_registration_t* dynamic_registrations;
  register_data_by_identifier_fn register_data_by_identifier;
#endif  // CONFIG_UDS_NEW_USE_DYNAMIC_DATA_BY_ID
};

int uds_new_init(struct uds_new_instance_t* inst,
                 const UDSISOTpCConfig_t* iso_tp_config,
                 const struct device* can_dev,
                 void* user_context);

#ifdef CONFIG_UDS_NEW_ENABLE_RESET

#endif  // CONFIG_UDS_NEW_ENABLE_RESET

/**
 * @brief opaque data. used internally
 */
struct uds_new_registration_t;

enum uds_new_registration_type_t {
  UDS_NEW_REGISTRATION_TYPE__DATA_IDENTIFIER,
};

struct uds_new_registration_t {
  struct uds_new_instance_t* instance;

  enum uds_new_registration_type_t type;

  void* user_data;
  bool can_read;
  bool can_write;

  union {
    struct {
      uint16_t data_id;
      size_t len;
      size_t len_elem;
      UDSErr_t (*read)(void* data,
                       size_t* len,
                       struct uds_new_registration_t*
                           reg);  // where data is the output and len
                                  // is the maximum size of the output
                                  // and must be written to be the real
                                  // length; return value is an error
                                  // or UDS_OK
      UDSErr_t (*write)(const void* data,
                        size_t len,
                        struct uds_new_registration_t*
                            reg);  // where data is the new data, len is
                                   // the length of the written data and
                                   // return value is an error or UDS_OK
    } data_identifier;
  };

  struct uds_new_registration_t* next;  // only used for dynamic registrations
};

UDSErr_t _uds_new_data_identifier_static_read(
    void* data, size_t* len, struct uds_new_registration_t* reg);
UDSErr_t _uds_new_data_identifier_static_write(
    const void* data, size_t len, struct uds_new_registration_t* reg);

// clang-format off

/**
 * @brief Register a static data identifier for the data at @p addr.
 *
 * This macro registers a static data identifier, associating a name and id
 * with a memory address so it can be read by the <read_data_by_identifier> 
 * command.
 * 
 * To not convert the data to big endian before sending, pass 1 as len_elem and
 * the size of the data in bytes to len.
 *
 * @param name      User identifier.
 * @param _instance uds_new instance that owns the reference.
 * @param _data_id  Identifier for the data at @p addr.
 * @param addr      Memory address where the data is found.
 * @param len       Length of the data at @p addr in elements.
 * @param len_elem  Length of each element in bytes ad @p addr. These amount of
 * byte are converted to be each
 */
#define UDS_NEW_REGISTER_DATA_IDENTIFIER_STATIC_MEM(    \
  _instance,                                           \
  _data_id,                                                  \
  addr,                                                      \
  _len,                                                      \
  _len_elem,                                                 \
  readable,                                                  \
  writable                                                   \
  )                                                          \
  STRUCT_SECTION_ITERABLE(uds_new_registration_t, id##_data_id) = {  \
    .instance = _instance,                                   \
    .type = UDS_NEW_REGISTRATION_TYPE__DATA_IDENTIFIER,      \
    .user_data = addr,                                       \
    .can_read = readable,                                    \
    .can_write = writable,                                   \
    .data_identifier =                                       \
        {                                                    \
          .data_id = _data_id,                               \
          .len = _len,                                       \
          .len_elem = _len_elem,                             \
          .read = _uds_new_data_identifier_static_read,      \
          .write = _uds_new_data_identifier_static_write,    \
        },                                                   \
  };

/**
 * @brief Register a static data identifier for a primitive data type.
 *
 * This macro registers a static data identifier, associating a name and id
 * with a variable so it can be read by the <read_data_by_identifier> command.
 *
 * @param name      User identifier.
 * @param _instance uds_new instance that owns the reference.
 * @param _data_id  Identifier for the data at @p addr.
 * @param variable  Variable to associate with the data identifier.
 */
#define UDS_NEW_REGISTER_DATA_IDENTIFIER_STATIC(_instance, _data_id, \
                                                variable,\
  readable,                                                  \
  writable                                                   \
                                              )                  \
UDS_NEW_REGISTER_DATA_IDENTIFIER_STATIC_MEM(                               \
  _instance,                                                               \
  _data_id,                                                                \
  &variable,                                                               \
  1,                                                                       \
  sizeof(variable),                                                         \
  readable,                                                  \
  writable                                                   \
)


/**
 * @brief Register a static data identifier for an array of data.
 *
 * This macro registers a static data identifier, associating a name and id
 * with an array of data so it can be read by the <read_data_by_identifier>
 * command.
 * 
 * Every element of the array is converted to Big Endian format before transmit
 *
 * @param name      User identifier.
 * @param _instance uds_new instance that owns the reference.
 * @param _data_id  Identifier for the data at @p addr.
 * @param array     Array to associate with the data identifier.
 */
#define UDS_NEW_REGISTER_DATA_IDENTIFIER_STATIC_ARRAY(      \
     _instance, _data_id, array,\
  readable,                                                  \
  writable                                                   \
  )                       \
UDS_NEW_REGISTER_DATA_IDENTIFIER_STATIC_MEM(                \
  _instance,                                                \
  _data_id,                                                 \
  &array[0],                                                \
  ARRAY_SIZE(array),                                        \
  sizeof(array[0]),                                          \
  readable,                                                  \
  writable                                                   \
)
// clang-format on

#endif  // ARDEP_UDS_NEW_H