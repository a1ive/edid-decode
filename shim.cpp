#include <iostream>
#include <stdio.h>
#include <stdarg.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <io.h>

#include "src/edid-decode.h"

#include "winedid.h"

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

extern "C" int open(const char* pathname, int flags, ...)
{
	if (_strnicmp(pathname, "/MONITOR", 8) == 0)
	{
		long monitor_index = strtol(pathname + 8, NULL, 10);

		//fprintf(stderr, "Open monitor index %ld\n", monitor_index);

		constexpr size_t EDID_BUFFER_SIZE = 256;
		char edid_buffer[EDID_BUFFER_SIZE];
		int bytes_read = get_monitor_edid((int)monitor_index, edid_buffer, EDID_BUFFER_SIZE);

		if (bytes_read <= 0)
		{
			errno = EIO;
			return -1;
		}

		int pipe_fds[2];
		if (_pipe(pipe_fds, bytes_read, _O_BINARY) != 0)
			return -1;

		int read_fd = pipe_fds[0];
		int write_fd = pipe_fds[1];

		int bytes_written = _write(write_fd, edid_buffer, bytes_read);

		_close(write_fd);

		if (bytes_written != bytes_read)
		{
			_close(read_fd);
			errno = EIO;
			return -1;
		}

		return read_fd;
	}

	int pmode = 0;
	if (flags & _O_CREAT)
	{
		va_list args;
		va_start(args, flags);
		pmode = va_arg(args, int);
		va_end(args);
	}
	return _open(pathname, flags, pmode);
}
