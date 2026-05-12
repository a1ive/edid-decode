// SPDX-License-Identifier: MIT
/*
 * Copyright 2024 Cisco Systems, Inc. and/or its affiliates. All rights reserved.
 *
 * Author: Hans Verkuil <hverkuil+cisco@kernel.org>
 */

#include <cctype>
#include <cerrno>
#include <csignal>
#include <cstring>
#include <ctime>
#include <string>

#include <fcntl.h>
#include <getopt.h>
#include <sys/ioctl.h>
#include <sys/epoll.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <math.h>
#include <dirent.h>

#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <linux/types.h>

#include "edid-decode.h"

/*
 * General note when reading from the DDC bus:
 * avoid reading more than 128 bytes. Trying to read 256 bytes in particular
 * will not work if there is a DisplayPort to HDMI adapter since the DP
 * REMOTE_I2C_READ/WRITE requests use 8 bit values for the length, so 256
 * would map to 0.
 */

// i2c addresses for edid
#define EDID_ADDR 0x50
#define SEGMENT_POINTER_ADDR 0x30

// i2c address for SCDC
#define SCDC_ADDR 0x54

// i2c addresses for HDCP
#define HDCP_PRIM_ADDR 0x3a
#define HDCP_SEC_ADDR 0x3b

static struct timespec start_monotonic;
static struct timeval start_timeofday;
static time_t valid_until_t;

static std::string ts2s(__u64 ts)
{
	static char buf[64];
	static unsigned last_secs;
	static time_t last_t;
	std::string s;
	struct timeval sub = {};
	struct timeval res;
	unsigned secs;
	__s64 diff;
	time_t t;

	diff = ts - start_monotonic.tv_sec * 1000000000ULL - start_monotonic.tv_nsec;
	if (diff >= 0) {
		sub.tv_sec = diff / 1000000000ULL;
		sub.tv_usec = (diff % 1000000000ULL) / 1000;
	}
	timeradd(&start_timeofday, &sub, &res);
	t = res.tv_sec;
	if (t >= valid_until_t) {
		struct tm tm = *localtime(&t);
		last_secs = tm.tm_min * 60 + tm.tm_sec;
		last_t = t;
		valid_until_t = t + 60 - last_secs;
		strftime(buf, sizeof(buf), "%a %b %e %T.000000", &tm);
	}
	secs = last_secs + t - last_t;
	sprintf(buf + 14, "%02u:%02u.%06llu", secs / 60, secs % 60, (__u64)res.tv_usec);
	return buf;
}

static __u64 current_ts()
{
	struct timespec ts;

	clock_gettime(CLOCK_MONOTONIC, &ts);
	return ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}


int request_i2c_adapter(const char *device)
{
	int fd = open(device, O_RDWR);

	if (fd >= 0)
		return fd;
	fprintf(stderr, "Error accessing i2c adapter %s\n", device);
	return fd;
}

static int read_edid_block(int adapter_fd, __u8 *edid,
			   uint8_t segment, uint8_t offset, uint8_t blocks,
			   bool silent)
{
	struct i2c_rdwr_ioctl_data data;
	struct i2c_msg write_message;
	struct i2c_msg read_message;
	struct i2c_msg seg_message;
	int err;

	seg_message = {
		.addr = SEGMENT_POINTER_ADDR,
		.len = 1,
		.buf = &segment
	};
	write_message = {
		.addr = EDID_ADDR,
		.len = 1,
		.buf = &offset
	};
	read_message = {
		.addr = EDID_ADDR,
		.flags = I2C_M_RD,
		.len = (__u16)(blocks * EDID_PAGE_SIZE),
		.buf = edid
	};

	if (segment) {
		struct i2c_msg msgs[3] = { seg_message, write_message, read_message };

		data.msgs = msgs;
		data.nmsgs = ARRAY_SIZE(msgs);
		err = ioctl(adapter_fd, I2C_RDWR, &data);
	} else {
		struct i2c_msg msgs[2] = { write_message, read_message };

		data.msgs = msgs;
		data.nmsgs = ARRAY_SIZE(msgs);
		err = ioctl(adapter_fd, I2C_RDWR, &data);
	}

	if (err < 0) {
		if (!silent)
			fprintf(stderr, "Unable to read edid: %s\n", strerror(errno));
		return err;
	}
	return 0;
}

int read_edid(int adapter_fd, unsigned char *edid, bool silent)
{
	unsigned n_extension_blocks;
	int err;

	err = read_edid_block(adapter_fd, edid, 0, 0, 2, silent);
	if (err)
		return err;
	n_extension_blocks = edid[126];
	if (!n_extension_blocks)
		return 1;

	// Check for HDMI Forum EDID Extension Override Data Block
	if (n_extension_blocks >= 1 &&
	    edid[128] == 2 &&
	    edid[133] == 0x78 &&
	    (edid[132] & 0xe0) == 0xe0 &&
	    (edid[132] & 0x1f) >= 2 &&
	    edid[134] > 1)
		n_extension_blocks = edid[134];

	for (unsigned i = 2; i <= n_extension_blocks; i += 2) {
		err = read_edid_block(adapter_fd, edid + i * 128, i / 2, 0,
				      (i + 1 > n_extension_blocks ? 1 : 2),
				      silent);
		if (err)
			return err;
	}
	return n_extension_blocks + 1;
}

int test_reliability(int adapter_fd, unsigned secs, unsigned msleep)
{
	unsigned char edid[EDID_PAGE_SIZE * EDID_MAX_BLOCKS];
	unsigned char edid_tmp[EDID_PAGE_SIZE * EDID_MAX_BLOCKS];
	unsigned iter = 0;
	unsigned blocks;
	int ret;

	ret = read_edid(adapter_fd, edid);
	if (ret <= 0) {
		printf("FAIL: could not read initial EDID.\n");
		return ret;
	}
	blocks = ret;

	if (secs)
		printf("Read EDID (%u bytes) for %u seconds with %u milliseconds between each read.\n\n",
		       blocks * EDID_PAGE_SIZE, secs, msleep);
	else
		printf("Read EDID (%u bytes) forever with %u milliseconds between each read.\n\n",
		       blocks * EDID_PAGE_SIZE, msleep);

	clock_gettime(CLOCK_MONOTONIC, &start_monotonic);
	gettimeofday(&start_timeofday, nullptr);

	time_t start = time(NULL);
	time_t start_test = start;

	while (true) {
		iter++;
		if (msleep)
			usleep(msleep * 1000);
		ret = read_edid(adapter_fd, edid_tmp);
		if (ret <= 0) {
			printf("\nFAIL: failed to read EDID (iteration %u).\n", iter);
			return -1;
		}
		if ((unsigned)ret != blocks) {
			printf("\nFAIL: expected %u blocks, read %d blocks (iteration %u).\n",
			       blocks, ret, iter);
			return -1;
		}
		if (memcmp(edid, edid_tmp, blocks * EDID_PAGE_SIZE)) {
			printf("Initial EDID:\n\n");
			for (unsigned i = 0; i < blocks; i++) {
				hex_block("", edid + i * EDID_PAGE_SIZE, EDID_PAGE_SIZE, false);
				printf("\n");
			}
			printf("EDID of iteration %u:\n\n", iter);
			for (unsigned i = 0; i < blocks; i++) {
				hex_block("", edid_tmp + i * EDID_PAGE_SIZE, EDID_PAGE_SIZE, false);
				printf("\n");
			}
			printf("FAIL: mismatch between EDIDs (iteration %u).\n", iter);
			return -1;
		}
		time_t cur = time(NULL);
		if (secs && cur - start_test > (time_t)secs)
			break;
		if (cur - start >= 10) {
			start = cur;
			printf("At iteration %u...\n", iter);
			fflush(stdout);
		}
	}

	printf("\n%u iterations over %u seconds: PASS\n", iter, secs);
	return 0;
}

static int read_hdcp_registers(int adapter_fd, __u8 *hdcp_prim, __u8 *hdcp_sec, __u8 *ksv_fifo)
{
	struct i2c_rdwr_ioctl_data data;
	struct i2c_msg write_message;
	struct i2c_msg read_message;
	__u8 offset = 0;
	__u8 ksv_fifo_offset = 0x43;
	int err;

	write_message = {
		.addr = HDCP_PRIM_ADDR,
		.len = 1,
		.buf = &offset
	};
	read_message = {
		.addr = HDCP_PRIM_ADDR,
		.flags = I2C_M_RD,
		.len = 128,
		.buf = hdcp_prim
	};

	struct i2c_msg msgs[2] = { write_message, read_message };

	data.msgs = msgs;
	data.nmsgs = ARRAY_SIZE(msgs);
	err = ioctl(adapter_fd, I2C_RDWR, &data);
	if (err == 2) {
		// Skip 0x80: that's a single-burst read address for HDCP 2.x.
		offset = 129;
		msgs[1].len = 127;
		msgs[1].buf = hdcp_prim + 129;
		err = ioctl(adapter_fd, I2C_RDWR, &data);
	}
	if (err != 2) {
		fprintf(stderr, "Unable to read Primary Link HDCP: %s\n",
			strerror(errno));
		return -1;
	}

	write_message = {
		.addr = HDCP_PRIM_ADDR,
		.len = 1,
		.buf = &ksv_fifo_offset
	};
	read_message = {
		.addr = HDCP_PRIM_ADDR,
		.flags = I2C_M_RD,
		.len = (__u16)((hdcp_prim[0x41] & 0x7f) * 5),
		.buf = ksv_fifo
	};

	if (read_message.len) {
		struct i2c_msg ksv_fifo_msgs[2] = { write_message, read_message };

		data.msgs = ksv_fifo_msgs;
		data.nmsgs = ARRAY_SIZE(msgs);
		err = ioctl(adapter_fd, I2C_RDWR, &data);
		if (err != 2) {
			fprintf(stderr, "Unable to read KSV FIFO: %s\n",
				strerror(errno));
			return -1;
		}
	}

	offset = 0;
	write_message = {
		.addr = HDCP_SEC_ADDR,
		.len = 1,
		.buf = &offset
	};
	read_message = {
		.addr = HDCP_SEC_ADDR,
		.flags = I2C_M_RD,
		.len = 128,
		.buf = hdcp_sec
	};

	struct i2c_msg sec_msgs[2] = { write_message, read_message };
	data.msgs = sec_msgs;
	data.nmsgs = ARRAY_SIZE(msgs);
	err = ioctl(adapter_fd, I2C_RDWR, &data);
	if (err == 2) {
		offset = 128;
		msgs[1].len = 128;
		msgs[1].buf = hdcp_sec + 128;
		err = ioctl(adapter_fd, I2C_RDWR, &data);
	}

	return 0;
}

int read_hdcp(int adapter_fd, parse_data &pdata)
{
	__u8 *hdcp_prim = &pdata.buf[0];
	__u8 *ksv_fifo = hdcp_prim + 256;
	__u8 *hdcp_sec = ksv_fifo + 128 * 5;

	if (read_hdcp_registers(adapter_fd, hdcp_prim, hdcp_sec, ksv_fifo))
		return -1;
	pdata.buf_size = pdata.buf_max_size;
	return 0;
}

static int read_hdcp_ri_register(int adapter_fd, __u16 *v)
{
	struct i2c_rdwr_ioctl_data data;
	struct i2c_msg write_message;
	struct i2c_msg read_message;
	__u8 ri[2];
	__u8 offset = 8;
	int err;

	write_message = {
		.addr = HDCP_PRIM_ADDR,
		.len = 1,
		.buf = &offset
	};
	read_message = {
		.addr = HDCP_PRIM_ADDR,
		.flags = I2C_M_RD,
		.len = 2,
		.buf = ri
	};

	struct i2c_msg msgs[2] = { write_message, read_message };

	data.msgs = msgs;
	data.nmsgs = ARRAY_SIZE(msgs);
	err = ioctl(adapter_fd, I2C_RDWR, &data);

	if (err < 0)
		fprintf(stderr, "Unable to read Ri: %s\n", strerror(errno));
	else
		*v = ri[1] << 8 | ri[0];

	return err < 0 ? err : 0;
}

int read_hdcp_ri(int adapter_fd, double ri_time)
{
	__u64 last_ts = current_ts();
	bool first = true;
	__u16 ri = 0;
	__u16 last_ri = 0;

	clock_gettime(CLOCK_MONOTONIC, &start_monotonic);
	gettimeofday(&start_timeofday, nullptr);

	while (1) {
		__u64 ts = current_ts();

		printf("Timestamp: %s", ts2s(ts).c_str());
		if (!read_hdcp_ri_register(adapter_fd, &ri)) {
			printf(" Ri': 0x%04x", ri);
			if (!first && ri != last_ri) {
				printf(" (changed from 0x%04x after %llu ms)",
				       last_ri, (ts - last_ts) / 1000000);
				last_ts = ts;
				last_ri = ri;
			}
			first = false;
		}
		printf("\n");
		fflush(stdout);
		usleep(ri_time * 1000000);
	}
	return 0;
}

static int read_scdc_registers(int adapter_fd, __u8 *scdc, bool update_only)
{
	struct i2c_rdwr_ioctl_data data;
	struct i2c_msg write_message;
	struct i2c_msg read_message;
	__u8 offset = update_only ? 0x10 : 0;
	int err;

	write_message = {
		.addr = SCDC_ADDR,
		.len = 1,
		.buf = &offset
	};
	read_message = {
		.addr = SCDC_ADDR,
		.flags = I2C_M_RD,
		.len = (__u16)(update_only ? 2 : 128),
		.buf = scdc
	};

	struct i2c_msg msgs[2] = { write_message, read_message };

	data.msgs = msgs + update_only;
	data.nmsgs = ARRAY_SIZE(msgs) - update_only;
	err = ioctl(adapter_fd, I2C_RDWR, &data);

	if (err < 0) {
		fprintf(stderr, "Unable to read SCDC %02x-%02x: %s\n",
			offset, offset + read_message.len - 1, strerror(errno));
		return -1;
	}
	if (update_only)
		return 0;
	offset = 128;
	data.msgs[1].buf = scdc + 128;
	err = ioctl(adapter_fd, I2C_RDWR, &data);

	if (err < 0) {
		fprintf(stderr, "Unable to read SCDC %02x-%02x: %s\n",
			offset, offset + read_message.len - 1, strerror(errno));
		return -1;
	}
	return 0;
}

int read_scdc(int adapter_fd, parse_data &pdata, bool update_only)
{
	if (read_scdc_registers(adapter_fd, pdata.buf, update_only))
		return -1;

	pdata.buf_size = update_only ? 2 : 256;
	return 0;
}
