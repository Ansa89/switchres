/**************************************************************
 
   modeline.c - Modeline generation and scoring routines
   
   ---------------------------------------------------------
 
   SwitchRes   Modeline generation engine for emulation
               (C) 2010 Chris Kennedy
               (C) 2012 Antonio Giner          
 
 **************************************************************/

#include "switchres.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <errno.h>
#include <sys/stat.h>
#include <string.h>

#define max(a,b)({ __typeof__ (a) _a = (a);__typeof__ (b) _b = (b);_a > _b ? _a : _b; })
#define min(a,b)({ __typeof__ (a) _a = (a);__typeof__ (b) _b = (b);_a < _b ? _a : _b; })

int round_near (double number);

int ModelineCreate(ConfigSettings *cs, GameInfo *game, MonitorMode *monitor, ModeLine *mode) {
	double margin = 0;
	double interlace = 1;
	double VBlankLines = 0;

	/* Hfreq range */
	sprintf(mode->label, 
		"%.3fKhz -> %.3fKhz", 
		((double)monitor->HfreqMin/1000), ((double)monitor->HfreqMax/1000));

	mode->hactive = game->width;
	mode->vactive = game->height;
	mode->vfreq   = game->refresh;

	/* Vertical blanking */
	monitor->VerticalBlank = round((monitor->VfrontPorch * 1000) +
					(monitor->VsyncPulse * 1000) +
					(monitor->VbackPorch * 1000))/1000000;

	VBlankLines = round(monitor->HfreqMax * monitor->VerticalBlank);

	/* Height limits */
	monitor->YresMin = 
		Normalize((monitor->HfreqMin / monitor->VfreqMax) - VBlankLines, 8);
	if (monitor->cs->doublescan)
		monitor->YresMin /= 2;
	monitor->YresMax = 
		Normalize((monitor->HfreqMax / monitor->VfreqMin) - VBlankLines, 8);
	if (monitor->cs->interlace)
		monitor->YresMax *= 2;

	monitor->XresMin = 184;

	/* increase to minimum xres */
	if (mode->hactive < monitor->XresMin) {
		mode->hactive = monitor->XresMin;
		mode->result |= RESULT_RES_INC;
		if (monitor->cs->verbose > 1)
			sr_fprintf(stderr, "Increased horizontal size to %d\n", mode->hactive);
	}
	/* user minimum height without doublescan */
	if (monitor->cs->ymin > 0 && monitor->YresMin < monitor->cs->ymin && 
	     monitor->cs->ymin < monitor->YresMax) 
	{
		monitor->YresMin = monitor->cs->ymin;
		if (monitor->cs->verbose > 2)
			sr_fprintf(stderr, "User input for minimum vertical size %d\n", 
				monitor->YresMin);
	}

	if (monitor->cs->verbose > 2)
		sr_fprintf(stderr, "Setup monitor limits min=%dx%d max=%dx%d\n",
			monitor->XresMin, monitor->YresMin, 0, monitor->YresMax);

	/* check vertical resolution */
	if (mode->vactive < monitor->YresMin) {
		if ((mode->vactive > (monitor->ActiveLinesLimit / 2)) && monitor->cs->interlace) {
			interlace = 2;
			mode->interlace = 1;
			ResVirtualize(mode, monitor);
			mode->result |= RESULT_INTERLACE | RESULT_VIRTUALIZE;
		} else if (monitor->cs->doublescan) {
			interlace = 0.5;
			mode->doublescan = 1;
			mode->result |= RESULT_DOUBLESCAN;
			if (monitor->cs->verbose > 1)
				sr_fprintf(stderr, "Using doublescan\n");
		} else {
			mode->vactive = monitor->YresMin;
			mode->result |= RESULT_RES_INC;
			if (monitor->cs->verbose > 1)
				sr_fprintf(stderr, "Increased vertical lines to %d\n", 
					mode->vactive);
		}
	}

	/* Vertical Refresh */
	if (mode->vfreq < monitor->VfreqMin) {
		if ((2 * mode->vfreq) <= monitor->VfreqMax)
			mode->vfreq *= 2;
		else
			mode->vfreq = monitor->VfreqMin;
		if (monitor->cs->verbose > 1)
			sr_fprintf(stderr, "Increased vertical frequency to %.3f\n", 
				mode->vfreq);
	} else if (mode->vfreq > monitor->VfreqMax) {
		mode->vfreq = monitor->VfreqMax;
		if (monitor->cs->verbose > 1)
			sr_fprintf(stderr, "Decreased vertical frequency to %.3f\n", 
				mode->vfreq);
	}

	/* check vertical active lines */
	if (mode->vactive > monitor->ActiveLinesLimit) {
		if (monitor->cs->interlace && mode->vactive <= monitor->VirtualLinesLimit) {
			interlace = 2;
			mode->interlace = 1;
			ResVirtualize(mode, monitor);
			mode->result |= RESULT_INTERLACE | RESULT_VIRTUALIZE;
		} else {
			if (monitor->cs->interlace) {
				interlace = 2;
				mode->interlace = 1;
				mode->result |= RESULT_INTERLACE;
				if (monitor->cs->verbose > 1)
					sr_fprintf(stderr, "Using interlace\n");
			} else {
				mode->vactive = monitor->ActiveLinesLimit;
				ResVirtualize(mode, monitor);
				mode->result |= RESULT_VIRTUALIZE;
			}
		}
	}

	/* Horizontal frequency */
	mode->hfreq = mode->vfreq * mode->vactive / 
			(interlace * (1.0 - mode->vfreq * monitor->VerticalBlank));

	if (cs->verbose > 1)
		sr_fprintf(stderr, "Starting with Horizontal freq of %.3f and Vertical refresh of %.2f\n",
			mode->hfreq/1000, mode->vfreq);

	/* Minimum horizontal frequency */
	if (mode->hfreq < monitor->HfreqMin) {
		if (monitor->cs->verbose > 1)
			sr_fprintf(stderr, "Increased horizontal frequency from %.3f to %.3f\n", 
				mode->hfreq/1000, monitor->HfreqMin/1000);
		mode->hfreq = monitor->HfreqMin;
		mode->result |= RESULT_HFREQ_CHANGE;
	} else if (mode->hfreq > monitor->HfreqMax) {
		if (monitor->cs->verbose > 1)
			sr_fprintf(stderr, "Horizontal frequency too high %.3f vfreq %.3f\n", 
				mode->hfreq/1000, mode->vfreq);
		if (mode->vactive > monitor->ActiveLinesLimit && monitor->cs->interlace)
		{
			interlace = 2;
			mode->interlace = 1;
			ResVirtualize(mode, monitor);
			mode->result |= RESULT_INTERLACE | RESULT_VIRTUALIZE;
		} else {
			double old_vfreq = mode->vfreq;
			if (monitor->cs->verbose > 1)
				sr_fprintf(stderr, "Lowered horizontal frequency to  %.3f from %.3f\n", 
					monitor->HfreqMax, mode->hfreq/1000);
			mode->hfreq = monitor->HfreqMax;
			VBlankLines = round(mode->hfreq * monitor->VerticalBlank);
			mode->vfreq = mode->hfreq / (mode->vactive / interlace + VBlankLines);
			mode->result |= RESULT_HFREQ_CHANGE;
			if (monitor->cs->verbose > 1)
				sr_fprintf(stderr, "Vertical frequency changed to %.3f from %.3f\n", 
					mode->vfreq, old_vfreq);
		}
	}

	/* Get total vertical lines */
	mode->vtotal = round(mode->hfreq / mode->vfreq);
	if (interlace == 2)
		mode->vtotal += 0.5;
	while ((mode->vfreq * mode->vtotal) < monitor->HfreqMin && (mode->vfreq * (mode->vtotal+1.0)) < monitor->HfreqMax) {
		if (monitor->cs->verbose > 1) 
			sr_fprintf(stderr, "Increasing 1 line from horizontal freq %.3f to %.3f\n",
				(mode->vfreq * mode->vtotal), (mode->vfreq * (mode->vtotal+1)));
		mode->vtotal = mode->vtotal + 1.0;
	}

	while (((mode->vfreq+0.001) * mode->vtotal) < monitor->HfreqMin) 
		mode->vfreq += 0.001;

	if (cs->dcalign) {
		int i;
		int newPclock = 0;
		int vIncr = 0;
		double newDiff = 0, Diff = 0;
		ModeLine newMode;
		memcpy(&newMode, mode, sizeof(struct ModeLine));
		if (cs->verbose > 3)
			sr_fprintf(stderr, "Vfreq = %f Game Refresh = %f\n", mode->vfreq, game->refresh);
		for (i = 0; i < 5; i++) {
			/* calculate new horizontal frequency */
			mode->hfreq = mode->vfreq * (mode->vtotal + i);
			if (mode->hfreq <= monitor->HfreqMax) {
				/* Fill horizontal part of modeline */
				newMode.hfreq = mode->hfreq;
				newMode.hactive = mode->hactive;
				ModelineGetLineParams(&newMode, monitor);
				newMode.pclock = mode->hfreq * newMode.htotal;
				if (fabs(Normalize(newMode.pclock-cs->dcalign, cs->dcalign)-newMode.pclock) < fabs(Normalize(newMode.pclock, cs->dcalign)-newMode.pclock))
					newMode.pclock = Normalize(newMode.pclock, cs->dcalign) - cs->dcalign;
				else	
					newMode.pclock = Normalize(newMode.pclock, cs->dcalign);
				newMode.vfreq = newMode.pclock / ((mode->vtotal + i) * newMode.htotal);
				newDiff = fabs(newMode.vfreq - mode->vfreq);
				if (newDiff < Diff || Diff == 0) {
					if (cs->verbose > 3)
						sr_fprintf(stderr, "[%d] Pclock: %d newVfreq: %f - Vfreq: %f newDiff = %f < Diff = %f\n", 
							i, newMode.pclock, newMode.vfreq, mode->vfreq, newDiff, Diff);
					Diff = newDiff;
					vIncr = i;
					newPclock = newMode.pclock;	
					mode->hactive = newMode.hactive;
					mode->hbegin = newMode.hbegin;
					mode->hend = newMode.hend;
					mode->htotal = newMode.htotal;
					if (newDiff < 0.01)
						break;
				}
			}
		}
		if (newPclock) {
			mode->vtotal += vIncr;
			mode->pclock = newPclock;
		} else {
			mode->hfreq = mode->vfreq * mode->vtotal;
			ModelineGetLineParams(mode, monitor);
			mode->pclock = mode->htotal * mode->hfreq;
		}
	} else {
		/* calculate new horizontal frequency */
		mode->hfreq = mode->vfreq * mode->vtotal;

		/* Fill horizontal part of modeline */
		ModelineGetLineParams(mode, monitor);

		/* recalculate dotclock */
		mode->pclock = mode->htotal * mode->hfreq;
	}
	mode->vfreq = mode->pclock / (mode->vtotal * mode->htotal);
	mode->hfreq = mode->vfreq * mode->vtotal;

	/* Check for vertical refresh rate match to original */
	if (mode->vfreq < (game->refresh - 0.02) ||
		mode->vfreq > (game->refresh + 0.02)) 
	{
		if (monitor->cs->verbose)
			sr_fprintf(stderr, "Original Vref %f != %f\n", 
				game->refresh, mode->vfreq);
		if (round(game->refresh*2) != round(mode->vfreq) || !cs->redraw)
			mode->result |= RESULT_VFREQ_CHANGE;
		if (round(game->refresh*2) == round(mode->vfreq))
			mode->result |= RESULT_VFREQ_DOUBLE;
	}

	/* game name */
	sprintf(mode->game, "%s", game->name);

	/* mame resolution */
#ifdef SYS_LINUX
	/* Mode name */
	sprintf(mode->name, "%dx%dx%.2f", mode->hactive, mode->vactive, mode->vfreq);

	if (cs->version > 104 || cs->version == 0) {
		sprintf(mode->resolution, "%dx%dx%d@%.2f", 
			mode->hactive, mode->vactive, 32, mode->vfreq); 
	} else {
		sprintf(mode->resolution, "%dx%dx%d", 
			mode->hactive, mode->vactive, 32); 
	}
#else
	/* Mode name */
	sprintf(mode->name, "%dx%dx%d", 
		mode->hactive, mode->vactive, (int)round(mode->vfreq));

	if (cs->version > 104 || cs->version == 0) {
		sprintf(mode->resolution, "%dx%d@%d", 
			mode->hactive, mode->vactive, (int)round(mode->vfreq)); 
	} else {
		sprintf(mode->resolution, "%dx%d", 
			mode->hactive, mode->vactive); 
	}
#endif

	/* Vertical blanking */
	VBlankLines = round(mode->hfreq * monitor->VerticalBlank);
	if (interlace == 2)
		VBlankLines += 0.5;
	margin = (round(mode->vtotal * interlace) - 
			mode->vactive - (VBlankLines * interlace)) / 2;
	if (margin) {
		mode->result |= RESULT_PADDING;
		mode->vpad = (margin*2);
		if (monitor->cs->verbose > 1) 
			sr_fprintf(stderr, "Using %d lines padding\n", 
				mode->vpad);
	}

	mode->vbegin = mode->vactive + round(((mode->hfreq/1000)*monitor->VfrontPorch) * 
			interlace + margin);
	mode->vend   = mode->vbegin + round(((mode->hfreq/1000)*monitor->VsyncPulse) *
			interlace);

	mode->vtotal = round(mode->vtotal * interlace);

	mode->hfreq = mode->vfreq * (mode->vtotal/interlace);
	if (!cs->dcalign)
		mode->pclock = mode->htotal * mode->hfreq;

	if (monitor->HsyncPolarity)
		mode->hsync = 1;

	if (monitor->VsyncPolarity)
		mode->vsync = 1;

	return 0;
}

int ModelineGetLineParams(ModeLine *mode, MonitorMode *monitor) {
	int hhh, hhi, hhf, hht;
	int hh, hs, he, ht;
	double LineTime, CharTime, NewCharTime;
	double HfrontPorchMin, HsyncPulseMin, HbackPorchMin;

	HfrontPorchMin = monitor->HfrontPorch - .20;
	HsyncPulseMin  = monitor->HsyncPulse  - .20;
	HbackPorchMin  = monitor->HbackPorch  - .20;

	LineTime = 1 / mode->hfreq * 1000000;
	
	hh = round(mode->hactive / 8);
	hs = 1;
	he = 1;
	ht = 1;

	do {
		CharTime = LineTime / (hh + hs + he + ht);
		if (hs * CharTime < HfrontPorchMin ||
		    abs((hs + 1) * CharTime - monitor->HfrontPorch) < 
		     abs(hs * CharTime - monitor->HfrontPorch)) 
		{
			hs++;
		}
		if (he * CharTime < HsyncPulseMin ||
		    abs((he + 1) * CharTime - monitor->HsyncPulse) < 
		     abs(he * CharTime - monitor->HsyncPulse)) 
		{
			he++;
		}
		if (ht * CharTime < HbackPorchMin ||
		    abs((ht + 1) * CharTime - monitor->HbackPorch) < 
		     abs(ht * CharTime - monitor->HbackPorch)) 
		{
			ht++;
		}
		NewCharTime = LineTime / (hh + hs + he + ht);
	} while (NewCharTime != CharTime);

	hhh = hh * 8;
	hhi = (hh + hs) * 8;
	hhf = (hh + hs + he) * 8;
	hht = (hh + hs + he + ht) * 8;

	mode->hactive = hhh;
	mode->hbegin  = hhi;
	mode->hend    = hhf;
	mode->htotal  = hht;

	return 0;
}

int ResVirtualize(ModeLine *mode, MonitorMode *monitor) {
	double interlace = 1;
	int xresNew, VBlankLines, ActiveLinesMax, ActiveLinesMin;
	double vfreqlimit = 0;
	int i;

	if (mode->interlace)
		interlace = 2;
	else if (mode->doublescan)
		interlace = 0.5;
	
	VBlankLines    = monitor->HfreqMax * monitor->VerticalBlank;
	ActiveLinesMax = monitor->HfreqMax / monitor->VfreqMin - VBlankLines;
	ActiveLinesMin = monitor->HfreqMin / monitor->VfreqMax - VBlankLines;

	if (ActiveLinesMin > monitor->YresMin) 
		ActiveLinesMin = monitor->YresMin;
	if (ActiveLinesMax > monitor->YresMax) 
		ActiveLinesMax = monitor->YresMax;

	if (interlace == 1 && 
	     mode->vactive > (ActiveLinesMin * interlace) && 
	      mode->vactive < ActiveLinesMax)
			ActiveLinesMax = mode->vactive;

	for (i = (Normalize(ActiveLinesMax, 8) * interlace);
		i >= (Normalize(ActiveLinesMin, 16) * interlace); i -= 16) 
	{
		mode->vactive = i;
		VBlankLines = 
			(monitor->HfreqMax - 50) * monitor->VerticalBlank;
		vfreqlimit = 
			(monitor->HfreqMax - 50) / 
				(mode->vactive + VBlankLines * interlace) * 
					interlace;
		if (vfreqlimit >= mode->vfreq)
			break;
	}

	if (!vfreqlimit)
		return 1;

	mode->vfreq = vfreqlimit;

	xresNew = Normalize(((4.0/3.0) * mode->vactive), 8);
	if (mode->hactive < xresNew || interlace < 2)
		mode->hactive = xresNew;
	mode->hfreq = (mode->vactive / interlace + VBlankLines) * mode->vfreq;

	if (monitor->cs->verbose > 1) 
		sr_fprintf(stderr, "Virtualized to %dx%d@%.2f %.4fKhz\n",
			mode->hactive, mode->vactive, mode->vfreq,
			mode->hfreq/1000);

	return 0;
}

char * PrintModeline(ModeLine *mode, char *modeline) {
	sprintf(modeline, "     \"%s\" %.6f %d %d %d %d %d %d %d %.0f %s %s%s%s",
		mode->name, ((double)mode->pclock/1000000), 
		mode->hactive, mode->hbegin, mode->hend, mode->htotal,
		mode->vactive, mode->vbegin, mode->vend, mode->vtotal,
		mode->hsync?"+HSync":"-HSync", mode->vsync?"+VSync":"-VSync",
		mode->doublescan?" doublescan":"", mode->interlace?" interlace":"");

	return modeline;
}

int ModelineResult(ModeLine *mode, ConfigSettings *cs) {
	int weight = 0;

	if (cs->verbose) {
		if (mode->result)
			sr_fprintf(stderr, "# %s: (", mode->label);
		else
			sr_fprintf(stderr, "# %s: ( Perfect Resolution )\n", mode->label);
	}
	if (mode->result & RESULT_RES_INC) {
		if (cs->verbose)
			sr_fprintf(stderr, " | Increased Size");
		if (cs->vsync)
			weight += 1;
		else
			weight += 5;
	}
	if (mode->result & RESULT_RES_DEC) {
		if (cs->verbose)
			sr_fprintf(stderr, " | Reduced Size");
		if (cs->vsync)
			weight += 1;
		else
			weight += 5;
	}
	if (mode->result & RESULT_HFREQ_CHANGE) {
		if (cs->verbose)
			sr_fprintf(stderr, " | Hfreq Change");
		weight += 1;
	}
	if (mode->result & RESULT_VFREQ_DOUBLE) {
		if (cs->verbose)
			sr_fprintf(stderr, " | Vref Change");
		if (!cs->redraw)
			weight += 10;
	}
	if (mode->result & RESULT_VFREQ_CHANGE) {
		if (cs->verbose)
			sr_fprintf(stderr, " | Vref Change");
		if (cs->vsync)
			weight += 10;
		else
			weight += 1;
	}
	if (mode->result & RESULT_INTERLACE) {
		if (cs->verbose)
			sr_fprintf(stderr, " | Interlace");
		weight += 3;
	}
	if (mode->result & RESULT_DOUBLESCAN) {
		if (cs->verbose)
			sr_fprintf(stderr, " | Doublescan");
		weight += 3;
	}
	if (mode->result & RESULT_VIRTUALIZE) {
		if (cs->verbose)
			sr_fprintf(stderr, " | Virtualize");
		weight += 6;
	}
	if (mode->result & RESULT_PADDING) {
		if (cs->verbose)
			sr_fprintf(stderr, " | Vpad +%d lines", mode->vpad);
		weight += 1;
		weight += round(mode->vpad/8);
	}
	if (cs->verbose && weight)
		sr_fprintf(stderr, " | )\n");

	return weight;
}

int StoreModeline(ModeLine *mode, ConfigSettings *cs) {
        FILE *f = NULL;
        char line[1000] = "", tmp[1000] = "", tmp2[1000] = "";
	int found = 0;
	char modeline[255];
	char mcheck[255];

        if (!cs->modesfile)
                return 1;

        if (cs->verbose)
                sr_fprintf(stderr, "Opening %s modes file for mode %s\n", 
			cs->modesfile, mode->resolution);

        f = fopen(cs->modesfile, "a+");
        if (f == NULL)
                return -EBADF;
	fseek(f, 0L, SEEK_SET);

	sprintf(mcheck,"%s", PrintModeline(mode, modeline));

	/* search for duplicate modeline */
        while(!feof(f)) {
                int e= fscanf(f, "%999[^\n]\n", line) - 1;
                if((line[0] == '#' || line[0] == '\n') && !e)
                        continue;
                e|= sscanf(line, "%999[^ ] %999[^\n]\n", tmp2, tmp) - 2;

                if (e) {
                        sr_fprintf(stderr, 
				"%s: Modes file Invalid syntax: '%s' %d\n", 
					cs->modesfile, line, e);
                        continue;
                }

                if (strstr(mcheck, tmp)) {
			found = 1;
			break; // Already exists
		}
	}

	/* write to modes file */
	if (!found) {
		fseek(f, 0L, SEEK_END);
		fprintf(f, "# %s %dx%d@%.2f %.4fKhz\n", mode->game,
                	mode->hactive, mode->vactive, mode->vfreq, mode->hfreq/1000);
		fprintf(f, "Modeline%s\n", mcheck);
	}

	fclose(f);
	if (found)
		return 1;
	return 0;	
}

//============================================================
//  modeline_vesa_gtf
//  Based on the VESA GTF spreadsheet by Andy Morrish 1/5/97
//============================================================

int modeline_vesa_gtf(ModeLine *m)
{
	int C, M;
	int v_sync_lines, v_porch_lines_min, v_front_porch_lines, v_back_porch_lines, v_sync_v_back_porch_lines, v_total_lines;
	int h_sync_width_percent, h_sync_width_pixels, h_blanking_pixels, h_front_porch_pixels, h_total_pixels;
	float v_freq, v_freq_est, v_freq_real, v_sync_v_back_porch;
	float h_freq, h_period, h_period_real, h_ideal_blanking;
	float pixel_freq, interlace;

	// Check if there's a value defined for vfreq. We're assuming input vfreq is the total field vfreq regardless interlace
	v_freq = m->vfreq? m->vfreq : m->a_vfreq;

	// These values are GTF defined defaults
	v_sync_lines = 3;
	v_porch_lines_min = 1;
	v_front_porch_lines = v_porch_lines_min;
	v_sync_v_back_porch = 550;
	h_sync_width_percent = 8;
	M = 128.0 / 256 * 600;
	C = ((40 - 20) * 128.0 / 256) + 20;

	// GTF calculation
	interlace = m->interlace?0.5:0;
	h_period = ((1.0 / v_freq) - (v_sync_v_back_porch / 1000000)) / ((float)m->a_height + v_front_porch_lines + interlace) * 1000000;
	v_sync_v_back_porch_lines = round_near(v_sync_v_back_porch / h_period);
	v_back_porch_lines = v_sync_v_back_porch_lines - v_sync_lines;
	v_total_lines = m->a_height + v_front_porch_lines + v_sync_lines + v_back_porch_lines;
	v_freq_est = (1.0 / h_period) / v_total_lines * 1000000;
	h_period_real = h_period / (v_freq / v_freq_est);
	v_freq_real = (1.0 / h_period_real) / v_total_lines * 1000000;
	h_ideal_blanking = (float)(C - (M * h_period_real / 1000));
	h_blanking_pixels = round_near(m->a_width * h_ideal_blanking /(100 - h_ideal_blanking) / (2 * 8)) * (2 * 8);
	h_total_pixels = m->a_width + h_blanking_pixels;
	pixel_freq = h_total_pixels / h_period_real * 1000000;
	h_freq = 1000000 / h_period_real;
	h_sync_width_pixels = round_near(h_sync_width_percent * h_total_pixels / 100 / 8) * 8;
	h_front_porch_pixels = (h_blanking_pixels / 2) - h_sync_width_pixels; 

	// Results
	m->hactive = m->a_width;
	m->hbegin = m->hactive + h_front_porch_pixels;
	m->hend = m->hbegin + h_sync_width_pixels;
	m->htotal = h_total_pixels;
	m->vactive = m->a_height;
	m->vbegin = m->vactive + v_front_porch_lines;
	m->vend = m->vbegin + v_sync_lines;
	m->vtotal = v_total_lines;
	m->hfreq = h_freq;
	m->vfreq = v_freq_real;
	m->pclock = pixel_freq;
	m->hsync = 0;
	m->vsync = 1;

	return 1;
}                                                         

//============================================================
//  round_near
//============================================================

int round_near(double number)
{
    return number < 0.0 ? ceil(number - 0.5) : floor(number + 0.5);
}
