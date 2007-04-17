/*
 * Copyright 2006-2007 Red Hat, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
/* Author: Adam Jackson <ajax@nwnk.net> */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>

int claims_one_point_oh = 0;
int claims_one_point_two = 0;
int claims_one_point_three = 0;
int claims_one_point_four = 0;
int nonconformant_digital_display = 0;
int did_detailed_timing = 0;
int has_name_descriptor = 0;
int name_descriptor_terminated = 0;
int has_range_descriptor = 0;
int has_preferred_timing = 0;
int has_valid_checksum = 0;
int has_valid_week = 0;
int has_valid_year = 0;
int has_valid_detailed_blocks = 0;
int has_valid_extension_count = 0;
int has_valid_descriptor_ordering = 1;
int manufacturer_name_well_formed = 0;
int seen_non_detailed_descriptor = 0;

int conformant = 1;

static char *manufacturer_name(unsigned char *x)
{
    static char name[4];

    name[0] = ((x[0] & 0x7C) >> 2) + '@';
    name[1] = ((x[0] & 0x03) << 3) + ((x[1] & 0xE0) >> 5) + '@';
    name[2] = (x[1] & 0x1F) + '@';
    name[3] = 0;

    if (isupper(name[0]) && isupper(name[1]) && isupper(name[2]))
	manufacturer_name_well_formed = 1;

    return name;
}

/* 1 means valid data */
/* XXX need to check that timings are first */
static int
detailed_block(unsigned char *x)
{
    static unsigned char name[53];
    int ha, hbl, hso, hspw, hborder, va, vbl, vso, vspw, vborder;
    char phsync, pvsync;

    if (!x[0] && !x[1] && !x[2] && !x[4]) {
	seen_non_detailed_descriptor = 1;
	if (x[3] >= 0x0 && x[3] <= 0xF) {
	    /* XXX in principle we could decode these if we ever found them */
	    printf("Manufacturer-specified data, tag %d\n", x[3]);
	    return 1;
	}
	switch (x[3]) {
	case 0x10:
	    /* XXX check: 5 through 17 filled with 0x0 */
	    printf("Dummy block\n");
	    return 1;
	case 0xF7:
	    /* TODO */
	    printf("Established timings III\n");
	    return 1;
	case 0xF8:
	    /* TODO */
	    printf("CVT 3 byte code descriptor\n");
	    return 1;
	case 0xF9:
	    /* TODO */
	    printf("Color management data\n");
	    return 1;
	case 0xFA:
	    /* TODO */
	    printf("More standard timings\n");
	    return 1;
	case 0xFB:
	    /* TODO */
	    printf("Color point\n");
	    return 1;
	case 0xFC:
	    /* XXX should check for spaces after the \n */
	    /* XXX check: terminated with 0x0A, padded with 0x20 */
	    has_name_descriptor = 1;
	    if (strchr((char *)name, '\n')) return 1;
	    strncat((char *)name, (char *)x + 5, 13);
	    if (strchr((char *)name, '\n')) {
		name_descriptor_terminated = 1;
		printf("Monitor name: %s", name);
	    }
	    return 1;
	case 0xFD:
	    /* 
	     * XXX todo: implement offsets, feature flags, vtd blocks
	     * XXX check: ranges are well-formed; block termination if no vtd
	     * XXX check: required if continuous frequency
	     */
	    has_range_descriptor = 1;
	    printf("Monitor ranges: %d-%dHZ vertical, %d-%dkHz horizontal",
		   x[5], x[6], x[7], x[8]);
	    if (x[9] != 255)
		printf(", max dotclock %dMHz\n", x[9] * 10);
	    else
		printf("\n");
	    return 1;
	case 0xFE:
	    /* XXX check: terminated with 0x0A, padded with 0x20 */
	    printf("ASCII string: %s", x+5);
	    return 1;
	case 0xFF:
	    /* XXX check: terminated with 0x0A, padded with 0x20 */
	    printf("Serial number: %s", x+5);
	    return 1;
	default:
	    printf("Unknown monitor description type %d\n", x[3]);
	    return 0;
	}
    }

    if (seen_non_detailed_descriptor) {
	has_valid_descriptor_ordering = 0;
    }

    did_detailed_timing = 1;
    ha = (x[2] + ((x[4] & 0xF0) << 4));
    hbl = (x[3] + ((x[4] & 0x0F) << 8));
    hso = (x[8] + ((x[11] & 0xC0) << 2));
    hspw = (x[9] + ((x[11] & 0x30) << 4));
    hborder = x[15];
    va = (x[5] + ((x[7] & 0xF0) << 4));
    vbl = (x[6] + ((x[7] & 0x0F) << 8));
    vso = ((x[10] >> 4) + ((x[11] & 0x0C) << 2));
    vspw = ((x[10] & 0x0F) + ((x[11] & 0x03) << 4));
    vborder = x[16];
    phsync = (x[17] & (1 << 2)) ? '+' : '-';
    pvsync = (x[17] & (1 << 1)) ? '+' : '-';

    printf("Detailed mode: Clock %.3f MHz, %d mm x %d mm\n"
	   "               %4d %4d %4d %4d hborder %d\n"
	   "               %4d %4d %4d %4d vborder %d\n"
	   "               %chsync %cvsync\n",
	    (x[0] + (x[1] << 8)) / 100.0,
	    (x[12] + ((x[14] & 0xF0) << 4)),
	    (x[13] + ((x[14] & 0x0F) << 8)),
	   ha, ha + hso, ha + hso + hspw, ha + hbl, hborder,
	   va, va + vso, va + vso + vspw, va + vbl, vborder,
	   phsync, pvsync
	  );
    /* XXX flag decode */
    
    return 1;
}

static unsigned char *
extract_edid(int fd)
{
    struct stat buf;
    unsigned char *ret = NULL;
    unsigned char *start, *c;
    unsigned char *out = NULL;
    int state = 0;
    int lines = 0;
    int i;
    int out_index = 0;

    if (fstat(fd, &buf))
	return NULL;

    ret = calloc(1, buf.st_size);
    if (!ret)
	return NULL;

    read(fd, ret, buf.st_size);

    /* wait, is this a log file? */
    for (i = 0; i < 8; i++) {
	if (!isascii(ret[i]))
	    return ret;
    }

    /* I think it is, let's go scanning */
    if (!(start = strstr(ret, "EDID (in hex):")))
	return ret;
    if (!(start = strstr(start, "(II)")))
	return ret;

    for (c = start; *c; c++) {
	if (state == 0) {
	    /* skip ahead to the : */
	    if (!(c = strstr(c, ": \t")))
		break;
	    /* and find the first number */
	    while (!isxdigit(c[1]))
		c++;
	    state = 1;
	    lines++;
	    out = realloc(out, lines * 16);
	} else if (state == 1) {
	    char buf[3];
	    /* Read a %02x from the log */
	    if (!isxdigit(*c)) {
		state = 0;
		continue;
	    }
	    buf[0] = c[0];
	    buf[1] = c[1];
	    buf[2] = 0;
	    out[out_index++] = strtol(buf, NULL, 16);
	    c++;
	}
    }

    free(ret);

    return out;
}

int main(int argc, char **argv)
{
    int fd;
    unsigned char *edid;
    time_t the_time;
    struct tm *ptm;
    if (argc != 2) {
	printf("Need a file name\n");
	return 1;
    }

    if ((fd = open(argv[1], O_RDONLY)) == -1) {
	printf("Open failed\n");
	return 1;
    }

    edid = extract_edid(fd);
    close(fd);

    if (!edid || memcmp(edid, "\x00\xFF\xFF\xFF\xFF\xFF\xFF\x00", 8)) {
	printf("No header found\n");
	return 1;
    }

    printf("Manufacturer: %s Model %x Serial Number %u\n",
	    manufacturer_name(edid + 0x08),
	    (unsigned short)(edid[0x0A] + (edid[0x0B] << 8)),
	    (unsigned int)(edid[0x0C] + (edid[0x0D] << 8)
			   + (edid[0x0E] << 16) + (edid[0x0F] << 24)));
    /* XXX need manufacturer ID table */

    time(&the_time);
    ptm = localtime(&the_time);
    if (edid[0x10] < 55 || edid[0x10] == 0xff) {
	has_valid_week = 1;
	if (edid[0x11] > 0x0f) {
	    if (edid[0x10] == 0xff) {
		has_valid_year = 1;
		printf("Made week %hd of model year %hd\n", edid[0x10],
		       edid[0x11]);
	    } else if (edid[0x11] + 90 <= ptm->tm_year) {
		has_valid_year = 1;
		printf("Made week %hd of %hd\n", edid[0x10], edid[0x11] + 1990);
	    }
	}
    }

    printf("EDID version: %hd.%hd\n", edid[0x12], edid[0x13]);
    if (edid[0x12] == 1) {
	if (edid[0x13] > 4) {
	    printf("Claims > 1.4, assuming 1.4 conformance\n");
	    edid[0x13] = 4;
	}
	switch (edid[0x13]) {
	case 4:
	    claims_one_point_four = 1;
	case 3:
	    claims_one_point_three = 1;
	case 2:
	    claims_one_point_two = 1;
	default:
	    break;
	}
	claims_one_point_oh = 1;
    }

    /* display section */

    if (edid[0x14] & 0x80) {
	int conformance_mask;
	printf("Digital display\n");
	if (claims_one_point_four) {
	    conformance_mask = 0x80;
	    if (edid[0x14] & 0x70 == 0x00) {
		printf("Color depth is undefined\n");
	    else if (edid[0x14] & 0x70 == 0x70)
		nonconformant_digital_display = 1;
	    else
		printf("%d bits per primary color channel\n",
		       edid[0x14] >> 3 + 2);
	    }
	    switch (edid[0x14] & 0x0f) {
	    case 0x00: printf("Digital interface is not defined\n"); break;
	    case 0x01: printf("DVI interface\n"); break;
	    case 0x02: printf("HDMI-a interface\n"); break;
	    case 0x03: printf("HDMI-b interface\n"); break;
	    case 0x04: printf("MDDI interface\n"); break;
	    case 0x05: printf("DisplayPort interface\n"); break;
	    default:
		nonconformant_digital_display = 1;
	    }
	} else if (claims_one_point_two) {
	    conformance_mask = 0x7E;
	    if (edid[0x14] & 0x01) {
		printf("DFP 1.x compatible TMDS\n");
	    }
	} else conformance_mask = 0x7F;
	if (!nonconformant_digital_display)
	    nonconformant_digital_display = edid[0x14] & conformance_mask;
    } else {
	int voltage = (edid[0x14] & 0x60) >> 5;
	int sync = (edid[0x14] & 0x0F);
	printf("Analog display, Input voltage level: %s V\n",
	       voltage == 3 ? "0.7/0.7" :
	       voltage == 2 ? "1.0/0.4" :
	       voltage == 1 ? "0.714/0.286" :
	       "0.7/0.3");

	/* XXX verify this, not sure what X means by configurable levels */
	if (claims_one_point_four) {
	    if (edid[0x14] & 0x10)
		printf("Blank-to-black setup/pedestal\n");
	    else
		printf("Blank level equals black level\n");
	} else if (edid[0x14] & 0x10) {
	    printf("Configurable signal levels\n");
	}

	printf("Sync: %s%s%s%s\n", sync & 0x08 ? "Separate " : "",
	       sync & 0x04 ? "Composite " : "",
	       sync & 0x02 ? "SyncOnGreen " : "",
	       sync & 0x01 ? "Serration " : "");
    }

    if (edid[0x15] && edid[0x16])
	printf("Maximum image size: %d cm x %d cm\n", edid[0x15], edid[0x16]);
    else if (claims_one_point_four && (edid[0x15] || edid[0x16])) {
	/* XXX this might be a conformance failure for earlier revs */
	if (edid[0x15])
	    printf("Aspect ratio is %f (landscape)\n", 100.0/(edid[0x16] + 99));
	else
	    printf("Aspect ratio is %f (portrait)\n", 100.0/(edid[0x15] + 99));
    } else
	printf("Image size is variable\n");

    if (edid[0x17] == 0xff) {
	if (claims_one_point_four) /* XXX might be 1.3 too */
	    printf("Gamma is defined in an extension block\n");
	else printf("Gamma: 1.0\n");
    } else printf("Gamma: %.2f\n", ((edid[0x17] + 100.0) / 100.0));

    if (edid[0x18] & 0xE0) {
	printf("DPMS levels:");
	if (edid[0x18] & 0x80) printf(" Standby");
	if (edid[0x18] & 0x40) printf(" Suspend");
	if (edid[0x18] & 0x20) printf(" Off");
	printf("\n");
    }

    /* FIXME: all four are valid combos in 1.4, and this is analog only */
    if (edid[0x18] & 0x10)
	printf("Non-RGB color display\n");
    else if (edid[0x18] & 0x08)
	printf("RGB color display\n");
    else
	printf("Monochrome or grayscale display\n");
    /* FIXME: digital displays can do color encoding here */

    if (edid[0x18] & 0x04)
	printf("Default (sRGB) color space is primary color space\n");
    if (edid[0x18] & 0x02) {
	printf("First detailed timing is preferred timing\n");
	has_preferred_timing = 1;
    }
    if (edid[0x18] & 0x01)
	printf("Supports GTF timings within operating range\n");

    /* XXX color section */

    /* XXX established timings */

    /* XXX standard timings */

    /* detailed timings */
    has_valid_detailed_blocks = detailed_block(edid + 0x36);
    if (has_preferred_timing && !did_detailed_timing)
	has_preferred_timing = 0; /* not really accurate... */
    has_valid_detailed_blocks &= detailed_block(edid + 0x48);
    has_valid_detailed_blocks &= detailed_block(edid + 0x5A);
    has_valid_detailed_blocks &= detailed_block(edid + 0x6C);

    /* check this, 1.4 verification guide says otherwise */
    if (edid[0x7e]) {
	printf("Has %d extension blocks\n", edid[0x7e]);
	/* 2 is impossible because of the block map */
	if (edid[0x7e] != 2)
	    has_valid_extension_count = 1;
    } else {
	has_valid_extension_count = 1;
    }

    printf("Checksum: 0x%hx\n", edid[0x7f]);
    {
	unsigned char sum = 0;
	int i;
	for (i = 0; i < 128; i++)
	    sum += edid[i];
	has_valid_checksum = !sum;
    }

    if (claims_one_point_three) {
	if (nonconformant_digital_display ||
	    !has_name_descriptor ||
	    !name_descriptor_terminated ||
	    !has_preferred_timing ||
	    !has_range_descriptor)
	    conformant = 0;
	if (!conformant)
	    printf("EDID block does NOT conform to EDID 1.3!\n");
	if (nonconformant_digital_display)
	    printf("\tDigital display field contains garbage: %x\n",
		   nonconformant_digital_display);
	if (!has_name_descriptor)
	    printf("\tMissing name descriptor\n");
	else if (!name_descriptor_terminated)
	    printf("\tName descriptor not terminated with a newline\n");
	if (!has_preferred_timing)
	    printf("\tMissing preferred timing\n");
	if (!has_range_descriptor)
	    printf("\tMissing monitor ranges\n");
    } else if (claims_one_point_two) {
	if (nonconformant_digital_display ||
	    (has_name_descriptor && !name_descriptor_terminated))
	    conformant = 0;
	if (!conformant)
	    printf("EDID block does NOT conform to EDID 1.2!\n");
	if (nonconformant_digital_display)
	    printf("\tDigital display field contains garbage: %x\n",
		   nonconformant_digital_display);
	if (has_name_descriptor && !name_descriptor_terminated)
	    printf("\tName descriptor not terminated with a newline\n");
    } else if (claims_one_point_oh) {
	if (seen_non_detailed_descriptor)
	    conformant = 0;
	if (!conformant)
	    printf("EDID block does NOT conform to EDID 1.0!\n");
	if (seen_non_detailed_descriptor)
	    printf("\tHas descriptor blocks other than detailed timings\n");
    }

    if (!has_valid_checksum ||
	!has_valid_year ||
	!has_valid_week ||
	!has_valid_detailed_blocks ||
	!has_valid_extension_count ||
	!manufacturer_name_well_formed) {
	printf("EDID block does not conform at all!\n");
	if (!has_valid_checksum)
	    printf("\tBlock has broken checksum\n");
	if (!has_valid_year)
	    printf("\tBad year of manufacture\n");
	if (!has_valid_week)
	    printf("\tBad week of manufacture\n");
	if (!has_valid_detailed_blocks)
	    printf("\tDetailed blocks filled with garbage\n");
	if (!has_valid_extension_count)
	    printf("\tImpossible extension block count\n");
	if (!manufacturer_name_well_formed)
	    printf("\tManufacturer name field contains garbage\n");
    }

    /* Not sure which chunk of spec exactly requires this. See E-EDID guide
     * section 3.7.3.
     */
    if (!has_valid_descriptor_ordering) {
	printf("EDID block has detailed timing descriptors after other "
	       "descriptors!\n");
    }

    free(edid);

    return 0;
}
