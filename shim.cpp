
#include "src/edid-decode.h"

int request_i2c_adapter(const char* device)
{
	return -ENODEV;
}

int read_edid(int adapter_fd, unsigned char* edid, bool silent)
{
	return -ENODEV;
}

int test_reliability(int adapter_fd, unsigned secs, unsigned msleep)
{
	return -ENODEV;
}

int read_hdcp(int adapter_fd)
{
	return -ENODEV;
}

int read_hdcp_ri(int adapter_fd, double ri_time)
{
	return -ENODEV;
}
