#ifndef EDID_DECODE_SHIM_H
#define EDID_DECODE_SHIM_H

#ifndef __HAS_I2C_DEV__

int read_edid(int adapter_fd, unsigned char* edid, int silent);

#endif // __HAS_I2C_DEV__

#endif // EDID_DECODE_SHIM_H
