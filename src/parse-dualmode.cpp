// SPDX-License-Identifier: MIT
/*
 * Copyright 2026 Cisco Systems, Inc. and/or its affiliates. All rights reserved.
 *
 * Author: Hans Verkuil <hverkuil+cisco@kernel.org>
 */

#include <stdio.h>

#include "edid-decode.h"

void edid_state::parse_dualmode(const unsigned char *dualmode, unsigned size)
{
	unsigned char buf[17] = {};
	static const char id[] = "DP-HDMI ADAPTOR\x04";

	if (memchk(dualmode, 16)) {
		printf("DVI Cable Adapter:\n");
	} else if (memcmp(dualmode, id, sizeof(id) - 1)) {
		memcpy(buf, dualmode, 16);
		printf("Not a dualmode HDMI cable adaptor:\n");
		hex_block("  ", dualmode, 16);
		return;
	} else {
		printf("HDMI Cable Adapter:\n");
	}
	printf("  Revision: %u\n", dualmode[0x10] & 7);
	if ((dualmode[0x10] & 0xf0) == 0xa0)
		printf("  Type 2 or above\n");

	unsigned oui = dualmode[0x11] << 16 | dualmode[0x12] << 8 | dualmode[0x13];
	const char *ouiname = oui_name(oui);
	if (ouiname)
		printf("  OUI: %s (%s)\n", ouitohex(oui).c_str(), ouiname);
	else
		printf("  OUI: %s\n", ouitohex(oui).c_str());
	memcpy(buf, dualmode + 0x14, 6);
	buf[6] = 0;
	printf("  Device Identification String: '%s'\n", buf);
	printf("  Hardware Revision: %u.%u\n", dualmode[0x1a] >> 4, dualmode[0x1a] & 0xf);
	printf("  Firmware Revision: %u.%u\n", dualmode[0x1b], dualmode[0x1c]);
	printf("  Maximum TMDS Clock Rate: %u Mbps\n", dualmode[0x1d] * 5 / 2);
	std::string i2c_caps;
	unsigned char v = dualmode[0x1e];
	if (v & 1)
		add_str(i2c_caps, "1 Kbps");
	if (v & 2)
		add_str(i2c_caps, "5 Kbps");
	if (v & 4)
		add_str(i2c_caps, "10 Kbps");
	if (v & 8)
		add_str(i2c_caps, "100 Kbps");
	printf("  I2C Speed Capabilities: %s\n", i2c_caps.c_str());
	printf("  TMDS Output Buffers: %s\n", (dualmode[0x20] & 1) ? "Enabled" : "Disabled");
	printf("  HDMI CEC Pin Control: %s\n", (dualmode[0x21] & 1) ? "Enabled" : "Disabled");
	v = dualmode[0x22];
	if (v & 1)
		add_str(i2c_caps, "1 Kbps");
	if (v & 2)
		add_str(i2c_caps, "5 Kbps");
	if (v & 4)
		add_str(i2c_caps, "10 Kbps");
	if (v & 8)
		add_str(i2c_caps, "100 Kbps");
	printf("  I2C Speed Status: %s\n", i2c_caps.c_str());
	printf("  Vendor-Defined Features:\n");
	hex_block("    ", dualmode + 0x30, 16);
}
