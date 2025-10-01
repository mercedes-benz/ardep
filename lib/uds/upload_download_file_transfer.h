#ifndef UDS_UPLOAD_DOWNLOAD_FILE_TRANSFER_H
#define UDS_UPLOAD_DOWNLOAD_FILE_TRANSFER_H

#include "uds.h"

#include <ardep/uds.h>

bool uds_file_transfer_is_active(void);
UDSErr_t uds_file_transfer_request(struct uds_context* const context);
UDSErr_t uds_file_transfer_continue(struct uds_context* const context);
UDSErr_t uds_file_transfer_exit(void);

#endif  // UDS_UPLOAD_DOWNLOAD_FILE_TRANSFER_H
