#ifndef EDID_DECODE_SHIM_H
#define EDID_DECODE_SHIM_H

#ifndef __HAS_I2C_DEV__

int read_edid(int adapter_fd, unsigned char* edid, int silent);

#ifdef __cplusplus
struct parse_data;
int read_hdcp(int adapter_fd, parse_data &pdata);
#endif

#endif // __HAS_I2C_DEV__

#endif // EDID_DECODE_SHIM_H
