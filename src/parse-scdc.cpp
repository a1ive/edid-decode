// SPDX-License-Identifier: MIT
/*
 * Copyright 2024 Cisco Systems, Inc. and/or its affiliates. All rights reserved.
 *
 * Author: Hans Verkuil <hverkuil+cisco@kernel.org>
 */

#include <stdio.h>

#include "edid-decode.h"

static void print_scdc_update(const unsigned char *scdc_update)
{
	unsigned char flags = scdc_update[0];

	printf("Update Flags: 0x%02x 0x%02x\n", flags, scdc_update[1]);
	if (flags & 0x01)
		printf("\tStatus_Update\n");
	if (flags & 0x02)
		printf("\tCED_Update\n");
	if (flags & 0x04)
		printf("\tRR_Test\n");
	if (flags & 0x08)
		printf("\tSource_Test_Update\n");
	if (flags & 0x10)
		printf("\tFRL_start\n");
	if (flags & 0x20)
		printf("\tFLT_update\n");
	if (flags & 0x40)
		printf("\tRSED_Update\n");
	if (flags & 0x80)
		printf("\tLIP_Update\n");
}

void edid_state::parse_scdc(const unsigned char *scdc, unsigned size)
{
	static const char *frl_rates[] = {
		"Disable FRL",
		"3 Lanes at 3 Gbps per Lane",
		"3 Lanes at 6 Gbps per Lane",
		"4 Lanes at 6 Gbps per Lane",
		"4 Lanes at 8 Gbps per Lane",
		"4 Lanes at 10 Gbps per Lane",
		"4 Lanes at 12 Gbps per Lane",
		"4 Lanes at 16 Gbps per Lane",
		"4 Lanes at 20 Gbps per Lane",
		"4 Lanes at 24 Gbps per Lane",
	};
	unsigned char v;

	if (size == 2) {
		print_scdc_update(scdc);
		return;
	}

	printf("Sink Version: %u Source Version: %u\n", scdc[0x01], scdc[0x02]);

	v = scdc[0x03];
	printf("Sink Supported Features: 0x%02x\n", v);
	if (v & 0x80)
		printf("\tSink_LIP_Sup\n");

	v = scdc[0x05];
	printf("Source Supported Features: 0x%02x\n", v);
	if (v & 0x80)
		printf("\tSource_LIP_Sup\n");

	print_scdc_update(scdc + 0x10);

	v = scdc[0x20];
	printf("TMDS Configuration: 0x%02x\n", v);
	if (v & 0x01)
		printf("\tScrambling_Enable\n");
	printf("\tTMDS_Bit_Clock_Ratio: 1/%u\n", (v & 0x02) ? 40 : 10);

	v = scdc[0x21];
	printf("TMDS Scrambler Status: 0x%02x\n", v);
	if (v & 0x01)
		printf("\tTMDS_Scrambling_Status\n");

	v = scdc[0x30];
	printf("Sink Configuration: 0x%02x 0x%02x\n", v, scdc[0x31]);
	if (v & 0x01)
		printf("\tRR_Enable\n");
	if (v & 0x02)
		printf("\tFLT_no_retrain\n");
	if (v & 0x04)
		printf("\tDAISY_ERR\n");
	if (v & 0x08)
		printf("\tMONO_DIR_ON\n");
	if (v & 0x10)
		printf("\tMONO_DIR_ERR\n");
	if (v & 0x20)
		printf("\tCA_PWR_ERR\n");
	v = scdc[0x31];
	if ((v & 0xf) >= ARRAY_SIZE(frl_rates))
		printf("\tFRL_Rate: %u\n", v & 0xf);
	else
		printf("\tFRL_Rate: %s\n", frl_rates[v & 0xf]);
	printf("\tFFE_Levels: %u\n", v >> 4);

	bool uses_4lanes = (v & 0xf) >= 3;

	v = scdc[0x35];
	printf("Source Test Configuration: 0x%02x\n", v);
	if (v & 0x02)
		printf("\tTxFFE_Pre_Shoot_Only\n");
	if (v & 0x04)
		printf("\tTxFFE_De_Emphasis_Only\n");
	if (v & 0x08)
		printf("\tTxFFE_No_FFE\n");
	if (v & 0x20)
		printf("\tFLT_no_timeout\n");
	if (v & 0x40)
		printf("\tDSC_FRL_Max\n");
	if (v & 0x80)
		printf("\tFRL_Max\n");

	v = scdc[0x40];
	printf("Status Flags: 0x%02x 0x%02x 0x%02x\n", v, scdc[0x41], scdc[0x42]);
	if (v & 0x01)
		printf("\tClock_Detected\n");
	if (v & 0x02)
		printf("\tCh0_Ln0_Locked\n");
	if (v & 0x04)
		printf("\tCh1_Ln1_Locked\n");
	if (v & 0x08)
		printf("\tCh2_Ln2_Locked\n");
	if (v & 0x10)
		printf("\tLane3_Locked\n");
	if (v & 0x40)
		printf("\tFLT_Ready\n");
	if (v & 0x80)
		printf("\tDSC_DecodeFail\n");
	if (scdc[0x41] || scdc[0x42]) {
		v = scdc[0x41];
		printf("\tLn0_LTP_req: %u\n", v & 0xf);
		printf("\tLn1_LTP_req: %u\n", v >> 4);
		v = scdc[0x42];
		printf("\tLn2_LTP_req: %u\n", v & 0xf);
		printf("\tLn3_LTP_req: %u\n", v >> 4);
	}

	if ((scdc[0x51] & 0x80) || (scdc[0x53] & 0x80) || (scdc[0x55] & 0x80) ||
	    (scdc[0x58] & 0x80) || (scdc[0x5a] & 0x80)) {
		printf("Character Error Detection:\n");
		unsigned short cnt = scdc[0x51] << 8 | scdc[0x50];
		if (cnt & 0x8000)
			printf("\tChannel 0 Error Count: %u\n", cnt & 0x7fff);
		cnt = scdc[0x53] << 8 | scdc[0x52];
		if (cnt & 0x8000)
			printf("\tChannel 1 Error Count: %u\n", cnt & 0x7fff);
		cnt = scdc[0x55] << 8 | scdc[0x54];
		if (cnt & 0x8000)
			printf("\tChannel 2 Error Count: %u\n", cnt & 0x7fff);
		cnt = scdc[0x58] << 8 | scdc[0x57];
		if (cnt & 0x8000)
			printf("\tLane 3 Error Count: %u\n", cnt & 0x7fff);
		unsigned char sum = 0;
		for (unsigned i = 0x50; i <= 0x55; i++)
			sum += scdc[i];
		if (uses_4lanes)
			sum += scdc[0x57] + scdc[0x58];
		sum = 0xff - sum;
		sum++;
		if (sum != scdc[0x56])
			printf("\tInvalid Checksum: expected 0x%02x, got 0x%02x\n",
			       sum, scdc[0x56]);
		cnt = scdc[0x5a] << 8 | scdc[0x59];
		if (cnt & 0x8000)
			printf("\tReed-Solomon Corrections Counter: %u\n", cnt & 0x7fff);
	} else {
		printf("Character Error Detection: no errors detected\n");
	}

	v = scdc[0xc0];
	if (v) {
		printf("Test Read Request: 0x%02x\n", v);
		if (v & 0x80)
			printf("\tTestReadRequest\n");
		printf("\tTestReadRequestDelay: %ums\n", v & 0x7f);
	}

	if (scdc[0xd0] || scdc[0xd1] || scdc[0xd2]) {
		printf("Manufacturer Specific:\n");
		printf("\tOUI: %02x-%02x-%02x\n", scdc[0xd2], scdc[0xd1], scdc[0xd0]);
		char s[9] = {};
		memcpy(s, scdc + 0xd3, 8);
		printf("\tDevice_ID_String: '%s'\n", s);
		printf("\tHW Revision: %u.%u SW Revision: %u.%u\n",
		       scdc[0xdb] >> 4, scdc[0xdb] & 0xf,
		       scdc[0xdc], scdc[0xdd]);
	} else {
		printf("Manufacturer Specific: none\n");
	}
}
