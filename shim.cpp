
#include "src/edid-decode.h"

#ifdef __HAS_I2C_DEV__

int request_i2c_adapter(const char* device)
{
	//printf("request_i2c_adapter: %s\n", device);
	return -ENODEV;
}

int read_edid(int adapter_fd, unsigned char* edid, bool silent)
{
	//printf("read_edid: adapter_fd=%d, silent=%c\n", adapter_fd, silent ? 'y' : 'n');
	return -ENODEV;
}

int test_reliability(int adapter_fd, unsigned secs, unsigned msleep)
{
	//printf("test_reliability: adapter_fd=%d, secs=%u, msleep=%u\n", adapter_fd, secs, msleep);
	return -ENODEV;
}

int read_hdcp(int adapter_fd)
{
	//printf("read_hdcp: adapter_fd=%d\n", adapter_fd);
	return -ENODEV;
}

int read_hdcp_ri(int adapter_fd, double ri_time)
{
	//printf("read_hdcp_ri: adapter_fd=%d, ri_time=%.2f\n", adapter_fd, ri_time);
	return -ENODEV;
}

#else

int read_edid(int adapter_fd, unsigned char* edid, int silent)
{
	return -ENODEV;
}

#endif
