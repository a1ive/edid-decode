// SPDX-License-Identifier: MIT
/*
 * Copyright 2026 Cisco Systems, Inc. and/or its affiliates. All rights reserved.
 *
 * Author: Hans Verkuil <hverkuil+cisco@kernel.org>
 */

#include <stdio.h>

#include "edid-decode.h"

void edid_state::parse_hdcp(const unsigned char *hdcp, unsigned size)
{
	const unsigned char *hdcp_prim = hdcp;
	const unsigned char *ksv_fifo = hdcp + 256;
	const unsigned char *hdcp_sec = hdcp + 256 + 128 * 5;

	printf("HDCP Primary Link:\n\n");
	printf("Bksv: %02x %02x %02x %02x %02x\n",
	       hdcp_prim[0], hdcp_prim[1], hdcp_prim[2], hdcp_prim[3], hdcp_prim[4]);
	printf("Ri': %02x %02x\n", hdcp_prim[9], hdcp_prim[8]);
	printf("Pj': %02x\n", hdcp_prim[0x0a]);
	printf("Aksv: %02x %02x %02x %02x %02x\n",
	       hdcp_prim[0x10], hdcp_prim[0x11], hdcp_prim[0x12], hdcp_prim[0x13], hdcp_prim[0x14]);
	printf("Ainfo: %02x\n", hdcp_prim[0x15]);
	printf("An: %02x %02x %02x %02x %02x %02x %02x %02x\n",
	       hdcp_prim[0x18], hdcp_prim[0x19], hdcp_prim[0x1a], hdcp_prim[0x1b],
	       hdcp_prim[0x1c], hdcp_prim[0x1d], hdcp_prim[0x1e], hdcp_prim[0x1f]);
	printf("V'.H0: %02x %02x %02x %02x\n",
	       hdcp_prim[0x20], hdcp_prim[0x21], hdcp_prim[0x22], hdcp_prim[0x23]);
	printf("V'.H1: %02x %02x %02x %02x\n",
	       hdcp_prim[0x24], hdcp_prim[0x25], hdcp_prim[0x26], hdcp_prim[0x27]);
	printf("V'.H2: %02x %02x %02x %02x\n",
	       hdcp_prim[0x28], hdcp_prim[0x29], hdcp_prim[0x2a], hdcp_prim[0x2b]);
	printf("V'.H3: %02x %02x %02x %02x\n",
	       hdcp_prim[0x2c], hdcp_prim[0x2d], hdcp_prim[0x2e], hdcp_prim[0x2f]);
	printf("V'.H4: %02x %02x %02x %02x\n",
	       hdcp_prim[0x30], hdcp_prim[0x31], hdcp_prim[0x32], hdcp_prim[0x33]);
	unsigned char v = hdcp_prim[0x40];
	printf("Bcaps: %02x\n", v);
	if (v & 0x01)
		printf("\tFAST_REAUTHENTICATION\n");
	if (v & 0x02)
		printf("\t1.1_FEATURES\n");
	if (v & 0x10)
		printf("\tFAST\n");
	if (v & 0x20)
		printf("\tREADY\n");
	if (v & 0x40)
		printf("\tREPEATER\n");
	unsigned short vv = hdcp_prim[0x41] | (hdcp_prim[0x42] << 8);
	printf("Bstatus: %04x\n", vv);
	printf("\tDEVICE_COUNT: %u\n", vv & 0x7f);
	if (vv & 0x80)
		printf("\tMAX_DEVS_EXCEEDED\n");
	printf("\tDEPTH: %u\n", (vv >> 8) & 0x07);
	if (vv & 0x800)
		printf("\tMAX_CASCADE_EXCEEDED\n");
	if (vv & 0x1000)
		printf("\tHDMI_MODE\n");
	if (vv & 0x7f) {
		printf("KSV FIFO:\n");
		for (unsigned i = 0; i < (vv & 0x7f); i++)
			printf("\t%03u: %02x %02x %02x %02x %02x\n", i,
			       ksv_fifo[i * 5], ksv_fifo[i * 5 + 1],
			       ksv_fifo[i * 5 + 2], ksv_fifo[i * 5 + 3],
			       ksv_fifo[i * 5 + 4]);
	}
	v = hdcp_prim[0x50];
	printf("HDCP2Version: %02x\n", v);
	if (v & 4)
		printf("\tHDCP2.2\n");
	vv = hdcp_prim[0x70] | (hdcp_prim[0x71] << 8);
	printf("RxStatus: %04x\n", vv);
	if (vv & 0x3ff)
		printf("\tMessage_Size: %u\n", vv & 0x3ff);
	if (vv & 0x400)
		printf("\tREADY\n");
	if (vv & 0x800)
		printf("\tREAUTH_REQ\n");

	if (memchk(hdcp_sec, 256))
		return;

	printf("HDCP Secondary Link:\n\n");
	printf("Bksv: %02x %02x %02x %02x %02x\n",
	       hdcp_sec[0], hdcp_sec[1], hdcp_sec[2], hdcp_sec[3], hdcp_sec[4]);
	printf("Ri': %02x %02x\n", hdcp_sec[9], hdcp_sec[9]);
	printf("Pj': %02x\n", hdcp_sec[0x0a]);
	printf("Aksv: %02x %02x %02x %02x %02x\n",
	       hdcp_sec[0x10], hdcp_sec[0x11], hdcp_sec[0x12], hdcp_sec[0x13], hdcp_sec[0x14]);
	printf("Ainfo: %02x\n", hdcp_sec[0x15]);
	printf("An: %02x %02x %02x %02x %02x %02x %02x %02x\n",
	       hdcp_sec[0x18], hdcp_sec[0x19], hdcp_sec[0x1a], hdcp_sec[0x1b],
	       hdcp_sec[0x1c], hdcp_sec[0x1d], hdcp_sec[0x1e], hdcp_sec[0x1f]);
}
