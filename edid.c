/**************************************************************

   edid.c - Basic EDID generation
   (based on edid.S: EDID data template by Carsten Emde)


   ---------------------------------------------------------

   SwitchRes   Modeline generation engine for emulation
               (C) 2010 Chris Kennedy
               (C) 2014 Antonio Giner

 **************************************************************/

#include <stdio.h>
#include <string.h>
#include "switchres.h"

int edid_from_monitor_range(MonitorMode *range, ModeLine *mode, char *edid)
{
	if (!edid) return 0;

	// header
	edid[0] = 0x00;
	edid[1] = 0xff;
	edid[2] = 0xff;
	edid[3] = 0xff;
	edid[4] = 0xff;
	edid[5] = 0xff;
	edid[6] = 0xff;
	edid[7] = 0x00;

	// Manufacturer ID = "SWR"
	edid[8] = 0x4e;
	edid[9] = 0xf2;

	// Manufacturer product code
	edid[10] = 0x00;
	edid[11] = 0x00;

	// Serial number
	edid[12] = 0x00;
	edid[13] = 0x00;
	edid[14] = 0x00;
	edid[15] = 0x00;

	// Week of manufacture
	edid[16] = 5;

	// Year of manufacture
	edid[17] = 2014 - 1999;

	// EDID version and revision
	edid[18] = 1;
	edid[19] = 3;

	// video params
	edid[20] = 0x6d;

	// Maximum H & V size in cm
	edid[21] = 48;
	edid[22] = 36;

	// Gamma
	edid[23] = 120;

	// Display features
	edid[24] = 0x0A;

	// Chromacity coordinates;
	edid[25] = 0x5e;
	edid[26] = 0xc0;
	edid[27] = 0xa4;
	edid[28] = 0x59;
	edid[29] = 0x4a;
	edid[30] = 0x98;
	edid[31] = 0x25;
	edid[32] = 0x20;
	edid[33] = 0x50;
	edid[34] = 0x54;

	// Established timings
	edid[35] = 0x00;
	edid[36] = 0x00;
	edid[37] = 0x00;

	// Standard timing information
	edid[38] = 0x01;
	edid[39] = 0x01;
	edid[40] = 0x01;
	edid[41] = 0x01;
	edid[42] = 0x01;
	edid[43] = 0x01;
	edid[44] = 0x01;
	edid[45] = 0x01;
	edid[46] = 0x01;
	edid[47] = 0x01;
	edid[48] = 0x01;
	edid[49] = 0x01;
	edid[50] = 0x01;
	edid[51] = 0x01;
	edid[52] = 0x01;
	edid[53] = 0x01;

	// Pixel clock in 10 kHz units. (0.-655.35 MHz, little-endian)
	edid[54] = (mode->pclock / 10000) & 0xff;
	edid[55] = (mode->pclock / 10000) >> 8;

	int h_active = mode->hactive;
	int h_blank = mode->htotal - mode->hactive;
	int h_offset = mode->hbegin - mode->hactive;
	int h_pulse = mode->hend - mode->hbegin;

	int v_active = mode->vactive;
	int v_blank = (int)mode->vtotal - mode->vactive;
	int v_offset = mode->vbegin - mode->vactive;
	int v_pulse = mode->vend - mode->vbegin;

	// Horizontal active pixels 8 lsbits (0-4095)
	edid[56] = h_active & 0xff;

	// Horizontal blanking pixels 8 lsbits (0-4095)
	edid[57] = h_blank & 0xff;

	// Bits 7-4 Horizontal active pixels 4 msbits
	// Bits 3-0 Horizontal blanking pixels 4 msbits
	edid[58] = (((h_active >> 8) & 0x0f) << 4) + ((h_blank >> 8) & 0x0f);

	// Vertical active lines 8 lsbits (0-4095)
	edid[59] = v_active & 0xff;

	// Vertical blanking lines 8 lsbits (0-4095)
	edid[60] = v_blank & 0xff;

	// Bits 7-4 Vertical active lines 4 msbits
	// Bits 3-0 Vertical blanking lines 4 msbits
	edid[61] = (((v_active >> 8) & 0x0f) << 4) + ((v_blank >> 8) & 0x0f);

	// Horizontal sync offset pixels 8 lsbits (0-1023) From blanking start
	edid[62] = h_offset & 0xff;

	// Horizontal sync pulse width pixels 8 lsbits (0-1023)
	edid[63] = h_pulse & 0xff;

	// Bits 7-4 Vertical sync offset lines 4 lsbits 0-63)
	// Bits 3-0 Vertical sync pulse width lines 4 lsbits 0-63)
	edid[64] = ((v_offset & 0x0f) << 4) + (v_pulse & 0x0f);

	// Bits 7-6 	Horizontal sync offset pixels 2 msbits
	// Bits 5-4 	Horizontal sync pulse width pixels 2 msbits
	// Bits 3-2 	Vertical sync offset lines 2 msbits
	// Bits 1-0 	Vertical sync pulse width lines 2 msbits
	edid[65] = (((h_offset >> 8) & 0x03) << 6) +
			   (((h_pulse >> 8) & 0x03) << 4) +
			   (((v_offset >> 8) & 0x03) << 2) +
			   ((v_pulse >> 8) & 0x03);

	// Horizontal display size, mm, 8 lsbits (0-4095 mm, 161 in)
	edid[66] = 485 & 0xff;

	// Vertical display size, mm, 8 lsbits (0-4095 mm, 161 in)
	edid[67] = 364 & 0xff;

	// Bits 7-4 Horizontal display size, mm, 4 msbits
	// Bits 3-0 Vertical display size, mm, 4 msbits
	edid[68] = (((485 >> 8) & 0x0f) << 4) + ((364 >> 8) & 0x0f);

	// Horizontal border pixels (each side; total is twice this)
	edid[69] = 0;

	// Vertical border lines (each side; total is twice this)
	edid[70] = 0;

	// Features bitmap
	edid[71] = ((mode->interlace & 0x01) << 7) + 0x18 + (mode->vsync << 2) + (mode->hsync << 2);


	// Descriptor: monitor serial number
	edid[72] = 0;
	edid[73] = 0;
	edid[74] = 0;
	edid[75] = 0xff;
	edid[76] = 0;
	edid[77] = 'S';
	edid[78] = 'w';
	edid[79] = 'i';
	edid[80] = 't';
	edid[81] = 'c';
	edid[82] = 'h';
	edid[83] = 'r';
	edid[84] = 'e';
	edid[85] = 's';
	edid[86] = '#';
	edid[87] = '0';
	edid[88] = '0';
	edid[89] = 0x0a;

	// Descriptor: monitor range limits
	edid[90] = 0;
	edid[91] = 0;
	edid[92] = 0;
	edid[93] = 0xfd;
	edid[94] = 0;
	edid[95] = ((int)range->VfreqMin) & 0xff;
	edid[96] = ((int)range->VfreqMax) & 0xff;
	edid[97] = ((int)range->HfreqMin/1000) & 0xff;
	edid[98] = ((int)range->HfreqMax/1000) & 0xff;
	edid[99] = 0xff;
	edid[100] = 0;
	edid[101] = 0x0a;
	edid[102] = 0x20;
	edid[103] = 0x20;
	edid[104] = 0x20;
	edid[105] = 0x20;
	edid[106] = 0x20;
	edid[107] = 0x20;

	// Descriptor: text
	edid[108] = 0;
	edid[109] = 0;
	edid[110] = 0;
	edid[111] = 0xfc;
	edid[112] = 0;
	edid[113] = 'S';
	edid[114] = 'w';
	edid[115] = 'i';
	edid[116] = 't';
	edid[117] = 'c';
	edid[118] = 'h';
	edid[119] = 'r';
	edid[120] = 'e';
	edid[121] = 's';
	edid[122] = 0x0a;
	edid[123] = 0x20;
	edid[124] = 0x20;
	edid[125] = 0x20;

	// Compute checksum
	char checksum = 0;
	int i;
	for (i = 0; i <= 126; i++)
		checksum += edid[i];
	edid[127] = 256 - checksum;

	return 1;
}
