/**************************************************************
 
   monitor.c - Monitor presets and custom monitor definition
   
   ---------------------------------------------------------
 
   SwitchRes   Modeline generation engine for emulation
               (C) 2010 Chris Kennedy
               (C) 2012 Antonio Giner          
     
 **************************************************************/

#include <stdio.h>
#include <string.h>
#include "switchres.h"

//============================================================
//  CONSTANTS
//============================================================

#define HFREQ_MIN  14000
#define HFREQ_MAX  100000
#define VFREQ_MIN  40
#define VFREQ_MAX  200
#define PROGRESSIVE_LINES_MIN 128
#define max(a,b)({ __typeof__ (a) _a = (a);__typeof__ (b) _b = (b);_a > _b ? _a : _b; })
#define min(a,b)({ __typeof__ (a) _a = (a);__typeof__ (b) _b = (b);_a < _b ? _a : _b; })

//============================================================
//  PROTOTYPES
//============================================================

int get_monitor_specs(ConfigSettings *cs, MonitorRange *range);
int convert_monitor_specs(MonitorMode *m, MonitorRange *range);
int fill_monitor_range(MonitorRange *range, const char *specs_line);
int show_monitor_range(MonitorRange *range);
int set_monitor_preset(char *type, MonitorRange *range);
int fill_lcd_range(MonitorRange *range, const char *specs_line);
int fill_vesa_gtf(MonitorRange *range, const char *max_lines);
int fill_vesa_range(MonitorRange *range, int lines_min, int lines_max);
int evaluate_monitor_range(MonitorRange *range);
int monitor_range_from_modeline(MonitorRange *range, ModeLine *mode);

//============================================================
//  get_monitor_specs
//============================================================

int get_monitor_specs(ConfigSettings *cs, MonitorRange *range)
{
	char default_monitor[] = "generic_15";

	memset(&range[0], 0, sizeof(struct MonitorRange) * MAX_RANGES);

	if (!strcmp(cs->monitor, "custom"))
	{
		fill_monitor_range(&range[0],cs->crt_range[0]);
		fill_monitor_range(&range[1],cs->crt_range[1]);
		fill_monitor_range(&range[2],cs->crt_range[2]);
		fill_monitor_range(&range[3],cs->crt_range[3]);
		fill_monitor_range(&range[4],cs->crt_range[4]);
		fill_monitor_range(&range[5],cs->crt_range[5]);
		fill_monitor_range(&range[6],cs->crt_range[6]);
		fill_monitor_range(&range[7],cs->crt_range[7]);
		fill_monitor_range(&range[8],cs->crt_range[8]);
		fill_monitor_range(&range[9],cs->crt_range[9]);
	}
	else if (!strcmp(cs->monitor, "lcd"))
		fill_lcd_range(&range[0],cs->lcd_range);

	else if (set_monitor_preset(cs->monitor, range) == 0)
		set_monitor_preset(default_monitor, range);

	return 0;
}

//============================================================
//  convert_monitor_specs
//============================================================

int convert_monitor_specs(MonitorMode *m, MonitorRange *range)
{
	int i;
	for (i = 0 ; i < MAX_MODES ; i++)
	{
		if (range[i].HfreqMin)
		{
			m[i].HfreqMin = range[i].HfreqMin;
			m[i].HfreqMax = range[i].HfreqMax;
			m[i].VfreqMin = range[i].VfreqMin;
			m[i].VfreqMax = range[i].VfreqMax;
			m[i].HfrontPorch = range[i].HfrontPorch;
			m[i].HsyncPulse = range[i].HsyncPulse;
			m[i].HbackPorch = range[i].HbackPorch;
			m[i].VfrontPorch = range[i].VfrontPorch * 1000;
			m[i].VsyncPulse = range[i].VsyncPulse * 1000;
			m[i].VbackPorch = range[i].VbackPorch * 1000;
			m[i].HsyncPolarity = range[i].HsyncPolarity;
			m[i].VsyncPolarity = range[i].VsyncPolarity;

			m[i].ActiveLinesLimit = range[i].ProgressiveLinesMax;
			m[i].VirtualLinesLimit = max(range[i].InterlacedLinesMin, range[i].ProgressiveLinesMin * 2);
		}
	}
		
	return 1;
}

//============================================================
//  fill_monitor_range
//============================================================

int fill_monitor_range(MonitorRange *range, const char *specs_line)
{
	MonitorRange new_range;
	
	if (strcmp(specs_line, "")) {
		int e = sscanf(specs_line, "%lf-%lf,%lf-%lf,%lf,%lf,%lf,%lf,%lf,%lf,%d,%d,%d,%d,%d,%d",
			&new_range.HfreqMin, &new_range.HfreqMax, 
			&new_range.VfreqMin, &new_range.VfreqMax,
			&new_range.HfrontPorch, &new_range.HsyncPulse, &new_range.HbackPorch,
			&new_range.VfrontPorch, &new_range.VsyncPulse, &new_range.VbackPorch,
			&new_range.HsyncPolarity, &new_range.VsyncPolarity,
			&new_range.ProgressiveLinesMin, &new_range.ProgressiveLinesMax,
			&new_range.InterlacedLinesMin, &new_range.InterlacedLinesMax);

		if (e != 16) {
			sr_fprintf(stderr, "SwitchRes: Error trying to fill monitor range with\n  %s\n", specs_line);
			return -1;
		}
		
		new_range.VfrontPorch /= 1000;
		new_range.VsyncPulse /= 1000;
		new_range.VbackPorch /= 1000;
		new_range.VerticalBlank = (new_range.VfrontPorch + new_range.VsyncPulse + new_range.VbackPorch);

		if (evaluate_monitor_range(&new_range))
		{
			sr_fprintf(stderr, "SwitchRes: Error in monitor range (ignoring): %s\n", specs_line);
			return -1;
		}
		else
			memcpy(range, &new_range, sizeof(struct MonitorRange));
	}
	return 0;
}

//============================================================
//  fill_lcd_range
//============================================================

int fill_lcd_range(MonitorRange *range, const char *specs_line)
{
	if (strcmp(specs_line, ""))
	{
		if (sscanf(specs_line, "%lf-%lf", &range->VfreqMin, &range->VfreqMax) == 2)
		{
			sr_fprintf(stderr, "SwitchRes: LCD vfreq range set by user as %f-%f\n", range->VfreqMin, range->VfreqMax);
			return 1;
		}
		else
			sr_fprintf(stderr, "SwitchRes: Error trying to fill LCD range with\n  %s\n", specs_line);
	}
	// Use default values
	range->VfreqMin = range->VfreqMax = 60;
	sr_fprintf(stderr, "SwitchRes: Using default vfreq range for LCD %f-%f\n", range->VfreqMin, range->VfreqMax);
	
	return 0;
}

//============================================================
//  fill_vesa_gtf
//============================================================

int fill_vesa_gtf(MonitorRange *range, const char *max_lines)
{
	int lines = 0;
	sscanf(max_lines, "vesa_%d", &lines);

	if (!lines)
		return 0;

	int i = 0;
	if (lines >= 480)
		i += fill_vesa_range(&range[i], 384, 480);
	if (lines >= 600)
		i += fill_vesa_range(&range[i], 480, 600);
	if (lines >= 768)
		i += fill_vesa_range(&range[i], 600, 768);
	if (lines >= 1024)
		i += fill_vesa_range(&range[i], 768, 1024);

	return i;
}

//============================================================
//  fill_vesa_range
//============================================================

int fill_vesa_range(MonitorRange *range, int lines_min, int lines_max)
{
	ModeLine mode;
	memset(&mode, 0, sizeof(ModeLine));
	
	mode.a_width = Normalize(STANDARD_CRT_ASPECT * lines_max, 8);
	mode.a_height = lines_max;
	mode.a_vfreq = 60;
	range->VfreqMin = 50;
	range->VfreqMax = 65;

	modeline_vesa_gtf(&mode);
	monitor_range_from_modeline(range, &mode);
	
	range->ProgressiveLinesMin = lines_min;
	range->HfreqMin = mode.hfreq - 500;
	range->HfreqMax = mode.hfreq + 500;

	return 1;
}

//============================================================
//  show_monitor_range
//============================================================

int show_monitor_range(MonitorRange *range)
{
	sr_fprintf(stderr, "SwitchRes: Monitor range %.2f-%.2f,%.2f-%.2f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%d,%d,%d,%d,%d,%d\n",
		range->HfreqMin, range->HfreqMax, 
		range->VfreqMin, range->VfreqMax,
		range->HfrontPorch, range->HsyncPulse, range->HbackPorch,
		range->VfrontPorch * 1000, range->VsyncPulse * 1000, range->VbackPorch * 1000,
		range->HsyncPolarity, range->VsyncPolarity,
		range->ProgressiveLinesMin, range->ProgressiveLinesMax,
		range->InterlacedLinesMin, range->InterlacedLinesMax);

	return 0;
}

//============================================================
//  set_monitor_preset
//============================================================

int set_monitor_preset(char *type, MonitorRange *range)
{
	// PAL TV - 50 Hz/625
	if (!strcmp(type, "pal"))
	{
		fill_monitor_range(&range[0], "15625.00-15625.00, 50.00-50.00, 1.500, 4.700, 5.800, 0.064, 0.160, 1.056, 0, 0, 192, 288, 448, 576");
		return 1;
	}
	// NTSC TV - 60 Hz/525
	else if (!strcmp(type, "ntsc"))
	{
		fill_monitor_range(&range[0], "15734.26-15734.26, 59.94-59.94, 1.500, 4.700, 4.700, 0.191, 0.191, 0.953, 0, 0, 192, 240, 448, 480");
		return 1;
	}
	// Generic 15.7 kHz
	else if (!strcmp(type, "generic_15"))
	{
		fill_monitor_range(&range[0], "15625-15750, 49.50-65.00, 2.000, 4.700, 8.000, 0.064, 0.192, 1.024, 0, 0, 192, 288, 448, 576");
		return 1;
	}
	// Arcade 15.7 kHz - standard resolution
	else if (!strcmp(type, "arcade_15"))
	{
		fill_monitor_range(&range[0], "15625-16200, 49.50-65.00, 2.000, 4.700, 8.000, 0.064, 0.192, 1.024, 0, 0, 192, 288, 448, 576");
		return 1;
	}
	// Arcade 15.7-16.5 kHz - extended resolution
	else if (!strcmp(type, "arcade_15ex"))
	{
		fill_monitor_range(&range[0], "15625-16500, 49.50-65.00, 2.000, 4.700, 8.000, 0.064, 0.192, 1.024, 0, 0, 192, 288, 448, 576");
		return 1;
	}
	// Arcade 25.0 kHz - medium resolution
	else if (!strcmp(type, "arcade_25"))
	{
		fill_monitor_range(&range[0], "24960-24960, 49.50-65.00, 0.800, 4.000, 3.200, 0.080, 0.200, 1.000, 0, 0, 320, 384, 640, 768");
		return 1;
	}
	// Arcade 31.5 kHz - medium resolution
	else if (!strcmp(type, "arcade_31"))
	{
		fill_monitor_range(&range[0], "31400-31500, 49.50-65.00, 0.940, 3.770, 1.890, 0.349, 0.064, 1.017, 0, 0, 384, 480, 0, 0");
		return 1;
	}
	// Arcade 15.7/25.0 kHz - dual-sync
	else if (!strcmp(type, "arcade_15_25"))
	{
		fill_monitor_range(&range[0], "15625-16200, 49.50-65.00, 2.000, 4.700, 8.000, 0.064, 0.192, 1.024, 0, 0, 192, 288, 448, 576");
		fill_monitor_range(&range[1], "24960-24960, 49.50-65.00, 0.800, 4.000, 3.200, 0.080, 0.200, 1.000, 0, 0, 320, 384, 640, 768");
		return 2;
	}
	// Arcade 15.7/25.0/31.5 kHz - tri-sync
	else if (!strcmp(type, "arcade_15_25_31"))
	{
		fill_monitor_range(&range[0], "15625-16200, 49.50-65.00, 2.000, 4.700, 8.000, 0.064, 0.192, 1.024, 0, 0, 192, 288, 448, 576");
		fill_monitor_range(&range[1], "24960-24960, 49.50-65.00, 0.800, 4.000, 3.200, 0.080, 0.200, 1.000, 0, 0, 320, 384, 640, 768");
		fill_monitor_range(&range[2], "31400-31500, 49.50-65.00, 0.940, 3.770, 1.890, 0.349, 0.064, 1.017, 0, 0, 384, 480, 0, 0");
		return 3;
	}
	// Makvision 2929D
	else if (!strcmp(type, "m2929"))
	{
		fill_monitor_range(&range[0], "30000-40000, 47.00-90.00, 0.600, 2.500, 2.800, 0.032, 0.096, 0.448, 0, 0, 384, 640, 0, 0");
		return 1;
	}
	// Wells Gardner D9800, D9400
	else if (!strcmp(type, "d9800") || !strcmp(type, "d9400"))
	{
		fill_monitor_range(&range[0], "15250-18000, 40-80, 2.187, 4.688, 6.719, 0.190, 0.191, 1.018, 0, 0, 224, 288, 448, 576");
		fill_monitor_range(&range[1], "18001-19000, 40-80, 2.187, 4.688, 6.719, 0.140, 0.191, 0.950, 0, 0, 288, 320, 0, 0");
		fill_monitor_range(&range[2], "20501-29000, 40-80, 2.910, 3.000, 4.440, 0.451, 0.164, 1.048, 0, 0, 320, 384, 0, 0");
		fill_monitor_range(&range[3], "29001-32000, 40-80, 0.636, 3.813, 1.906, 0.318, 0.064, 1.048, 0, 0, 384, 480, 0, 0");
		fill_monitor_range(&range[4], "32001-34000, 40-80, 0.636, 3.813, 1.906, 0.020, 0.106, 0.607, 0, 0, 480, 576, 0, 0");
		fill_monitor_range(&range[5], "34001-38000, 40-80, 1.000, 3.200, 2.200, 0.020, 0.106, 0.607, 0, 0, 576, 600, 0, 0");
		return 6;
	}
	// Wells Gardner D9200
	else if (!strcmp(type, "d9200"))
	{
		fill_monitor_range(&range[0], "15250-16500, 40-80, 2.187, 4.688, 6.719, 0.190, 0.191, 1.018, 0, 0, 224, 288, 448, 576");
		fill_monitor_range(&range[1], "23900-24420, 40-80, 2.910, 3.000, 4.440, 0.451, 0.164, 1.048, 0, 0, 320, 384, 0, 0");
		fill_monitor_range(&range[2], "31000-32000, 40-80, 0.636, 3.813, 1.906, 0.318, 0.064, 1.048, 0, 0, 480, 512, 0, 0");
		fill_monitor_range(&range[3], "37000-38000, 40-80, 1.000, 3.200, 2.200, 0.020, 0.106, 0.607, 0, 0, 512, 600, 0, 0");
		return 4;
	}
	// Wells Gardner K7000
	else if (!strcmp(type, "k7000"))
	{
		fill_monitor_range(&range[0], "15625-15800, 49.50-63.00, 2.000, 4.700, 8.000, 0.064, 0.160, 1.056, 0, 0, 192, 288, 448, 576");
		return 1;
	}
	// Wells Gardner 25K7131
	else if (!strcmp(type, "k7131"))
	{
		fill_monitor_range(&range[0], "15625-16670, 49.5-65, 2.000, 4.700, 8.000, 0.064, 0.160, 1.056, 0, 0, 192, 288, 448, 576");
		return 1;
	}
	// Wei-Ya M3129
	else if (!strcmp(type, "m3129"))
	{
		fill_monitor_range(&range[0], "15250-16500, 40-80, 2.187, 4.688, 6.719, 0.190, 0.191, 1.018, 1, 1, 224, 288, 448, 576");
		fill_monitor_range(&range[1], "23900-24420, 40-80, 2.910, 3.000, 4.440, 0.451, 0.164, 1.048, 1, 1, 320, 384, 0, 0");
		fill_monitor_range(&range[2], "31000-32000, 40-80, 0.636, 3.813, 1.906, 0.318, 0.064, 1.048, 1, 1, 480, 512, 0, 0");
		return 3;
	}
	// Hantarex MTC 9110
	else if (!strcmp(type, "h9110") || !strcmp(type, "polo"))
	{
		fill_monitor_range(&range[0], "15625-16670, 49.5-65, 2.000, 4.700, 8.000, 0.064, 0.160, 1.056, 0, 0, 192, 288, 448, 576");
		return 1;
	}
	// Hantarex Polostar 25
	else if (!strcmp(type, "pstar"))
	{
		fill_monitor_range(&range[0], "15700-15800, 50-65, 1.800, 0.400, 7.400, 0.064, 0.160, 1.056, 0, 0, 192, 256, 0, 0");
		fill_monitor_range(&range[1], "16200-16300, 50-65, 0.200, 0.400, 8.000, 0.040, 0.040, 0.640, 0, 0, 256, 264, 512, 528");
		fill_monitor_range(&range[2], "25300-25400, 50-65, 0.200, 0.400, 8.000, 0.040, 0.040, 0.640, 0, 0, 384, 400, 768, 800");
		fill_monitor_range(&range[3], "31500-31600, 50-65, 0.170, 0.350, 5.500, 0.040, 0.040, 0.640, 0, 0, 400, 512, 0, 0");
		return 4;
	}
	// Nanao MS-2930, MS-2931
	else if (!strcmp(type, "ms2930"))
	{
		fill_monitor_range(&range[0], "15450-16050, 50-65, 3.190, 4.750, 6.450, 0.191, 0.191, 1.164, 0, 0, 192, 288, 448, 576");
		fill_monitor_range(&range[1], "23900-24900, 50-65, 2.870, 3.000, 4.440, 0.451, 0.164, 1.148, 0, 0, 320, 384, 0, 0");
		fill_monitor_range(&range[2], "31000-32000, 50-65, 0.330, 3.580, 1.750, 0.316, 0.063, 1.137, 0, 0, 576, 768, 0, 0");
		return 3;
	}
	// Nanao MS9-29
	else if (!strcmp(type, "ms929"))
	{
		fill_monitor_range(&range[0], "15450-16050, 50-65, 3.910, 4.700, 6.850, 0.190, 0.191, 1.018, 0, 0, 192, 288, 448, 576");
		fill_monitor_range(&range[1], "23900-24900, 50-65, 2.910, 3.000, 4.440, 0.451, 0.164, 1.048, 0, 0, 320, 384, 0, 0");
		return 2;
	}
	// Rodotron 666B-29
 	else if (!strcmp(type, "r666b"))
 	{
 		fill_monitor_range(&range[0], "15450-16050, 50-65, 3.190, 4.750, 6.450, 0.191, 0.191, 1.164, 0, 0, 192, 288, 448, 576");
 		fill_monitor_range(&range[1], "23900-24900, 50-65, 2.870, 3.000, 4.440, 0.451, 0.164, 1.148, 0, 0, 384, 400, 0, 0");
 		fill_monitor_range(&range[2], "31000-32500, 50-65, 0.330, 3.580, 1.750, 0.316, 0.063, 1.137, 0, 0, 400, 512, 0, 0");
 		return 3;
 	}
 	// PC CRT 70kHz/120Hz
 	else if (!strcmp(type, "pc_31_120"))
 	{
 		fill_monitor_range(&range[0], "31400-31600, 100-130, 0.671, 2.683, 3.353, 0.034, 0.101, 0.436, 0, 0, 200, 256, 0, 0");
 		fill_monitor_range(&range[1], "31400-31600, 50-65, 0.671, 2.683, 3.353, 0.034, 0.101, 0.436, 0, 0, 400, 512, 0, 0");
 		return 2;
 	}
 	// PC CRT 70kHz/120Hz
 	else if (!strcmp(type, "pc_70_120"))
 	{
 		fill_monitor_range(&range[0], "30000-70000, 100-130, 2.201, 0.275, 4.678, 0.063, 0.032, 0.633, 0, 0, 192, 320, 0, 0");
 		fill_monitor_range(&range[1], "30000-70000, 50-65, 2.201, 0.275, 4.678, 0.063, 0.032, 0.633, 0, 0, 400, 1024, 0, 0");
 		return 2;
 	}
	// VESA GTF
	else if (!strcmp(type, "vesa_480") || !strcmp(type, "vesa_600") || !strcmp(type, "vesa_768") || !strcmp(type, "vesa_1024"))
	{
		return fill_vesa_gtf(&range[0], type);
	}

	sr_fprintf(stderr, "SwitchRes: Monitor type unknown: %s\n", type);
	return 0;
}

//============================================================
//  evaluate_monitor_range
//============================================================

int evaluate_monitor_range(MonitorRange *range)
{
	// First we check that all frequency ranges are reasonable
	if (range->HfreqMin < HFREQ_MIN || range->HfreqMin > HFREQ_MAX)
	{
		sr_fprintf(stderr, "SwitchRes: HfreqMin %.2f out of range\n", range->HfreqMin);
		return 1;
	}
	if (range->HfreqMax < HFREQ_MIN || range->HfreqMax < range->HfreqMin || range->HfreqMax > HFREQ_MAX)
	{
		sr_fprintf(stderr, "SwitchRes: HfreqMax %.2f out of range\n", range->HfreqMax);
		return 1;
	}			
	if (range->VfreqMin < VFREQ_MIN || range->VfreqMin > VFREQ_MAX)
	{
		sr_fprintf(stderr, "SwitchRes: VfreqMin %.2f out of range\n", range->VfreqMin);
		return 1;
	}			
	if (range->VfreqMax < VFREQ_MIN || range->VfreqMax < range->VfreqMin || range->VfreqMax > VFREQ_MAX)
	{
		sr_fprintf(stderr, "SwitchRes: VfreqMax %.2f out of range\n", range->VfreqMax);
		return 1;
	}			
	
	// LineTime in µs. We check that no horizontal value is longer than a whole line
	double LineTime = 1 / range->HfreqMax * 1000000;

	if (range->HfrontPorch <= 0 || range->HfrontPorch > LineTime)
	{	
		sr_fprintf(stderr, "SwitchRes: HfrontPorch %.3f out of range\n", range->HfrontPorch);
		return 1;
	}
	if (range->HsyncPulse <= 0 || range->HsyncPulse > LineTime)
	{	
		sr_fprintf(stderr, "SwitchRes: HsyncPulse %.3f out of range\n", range->HsyncPulse);
		return 1;
	}
	if (range->HbackPorch <= 0 || range->HbackPorch > LineTime)
	{	
		sr_fprintf(stderr, "SwitchRes: HbackPorch %.3f out of range\n", range->HbackPorch);
		return 1;
	}

	// FrameTime in ms. We check that no vertical value is longer than a whole frame
	double FrameTime = 1 / range->VfreqMax * 1000;

	if (range->VfrontPorch <= 0 || range->VfrontPorch > FrameTime)
	{	
		sr_fprintf(stderr, "SwitchRes: VfrontPorch %.3f out of range\n", range->VfrontPorch);
		return 1;
	}
	if (range->VsyncPulse <= 0 || range->VsyncPulse > FrameTime)
	{	
		sr_fprintf(stderr, "SwitchRes: VsyncPulse %.3f out of range\n", range->VsyncPulse);
		return 1;
	}
	if (range->VbackPorch <= 0 || range->VbackPorch > FrameTime)
	{	
		sr_fprintf(stderr, "SwitchRes: VbackPorch %.3f out of range\n", range->VbackPorch);
		return 1;
	}

	// Now we check sync polarities
	if (range->HsyncPolarity != 0 && range->HsyncPolarity != 1)
	{	
		sr_fprintf(stderr, "SwitchRes: Hsync polarity can be only 0 or 1\n");
		return 1;
	}
	if (range->VsyncPolarity != 0 && range->VsyncPolarity != 1)
	{	
		sr_fprintf(stderr, "SwitchRes: Vsync polarity can be only 0 or 1\n");
		return 1;
	}

	// Finally we check that the line limiters are reasonable
	// Progressive range:
	if (range->ProgressiveLinesMin > 0 && range->ProgressiveLinesMin < PROGRESSIVE_LINES_MIN)
	{	
		sr_fprintf(stderr, "SwitchRes: ProgressiveLinesMin must be greater than %d\n", PROGRESSIVE_LINES_MIN);
		return 1;
	}
	if ((range->ProgressiveLinesMin + range->HfreqMax * range->VerticalBlank) * range->VfreqMin > range->HfreqMax)
	{	
		sr_fprintf(stderr, "SwitchRes: ProgressiveLinesMin %d out of range\n", range->ProgressiveLinesMin);
		return 1;
	}
	if (range->ProgressiveLinesMax < range->ProgressiveLinesMin)
	{	
		sr_fprintf(stderr, "SwitchRes: ProgressiveLinesMax must greater than ProgressiveLinesMin\n");
		return 1;
	}
	if ((range->ProgressiveLinesMax + range->HfreqMax * range->VerticalBlank) * range->VfreqMin > range->HfreqMax)
	{	
		sr_fprintf(stderr, "SwitchRes: ProgressiveLinesMax %d out of range\n", range->ProgressiveLinesMax);
		return 1;
	}

	// Interlaced range:
	if (range->InterlacedLinesMin != 0)
	{
		if (range->InterlacedLinesMin < range->ProgressiveLinesMax)
		{
			sr_fprintf(stderr, "SwitchRes: InterlacedLinesMin must greater than ProgressiveLinesMax\n");
			return 1;
		}
		if (range->InterlacedLinesMin < PROGRESSIVE_LINES_MIN * 2)
		{	
			sr_fprintf(stderr, "SwitchRes: InterlacedLinesMin must be greater than %d\n", PROGRESSIVE_LINES_MIN * 2);
			return 1;
		}
		if ((range->InterlacedLinesMin / 2 + range->HfreqMax * range->VerticalBlank) * range->VfreqMin > range->HfreqMax)
		{	
			sr_fprintf(stderr, "SwitchRes: InterlacedLinesMin %d out of range\n", range->InterlacedLinesMin);
			return 1;
		}
		if (range->InterlacedLinesMax < range->InterlacedLinesMin)
		{	
			sr_fprintf(stderr, "SwitchRes: InterlacedLinesMax must greater than InterlacedLinesMin\n");
			return 1;
		}
		if ((range->InterlacedLinesMax / 2 + range->HfreqMax * range->VerticalBlank) * range->VfreqMin > range->HfreqMax)
		{	
			sr_fprintf(stderr, "SwitchRes: InterlacedLinesMax %d out of range\n", range->InterlacedLinesMax);
			return 1;
		}
	}
	else
	{
		if (range->InterlacedLinesMax != 0)
		{	
			sr_fprintf(stderr, "SwitchRes: InterlacedLinesMax must be zero if InterlacedLinesMin is not defined\n");
			return 1;
		}
	}
	return 0;
}

//============================================================
//  monitor_range_from_modeline
//============================================================

int monitor_range_from_modeline(MonitorRange *range, ModeLine *mode)
{
	// This routine assumes VfreqMin-VfreqMax are defined
	float LineTime = 1 / mode->hfreq;
	float PixelTime = LineTime / mode->htotal * 1000000;

	range->HfrontPorch = PixelTime * (mode->hbegin - mode->hactive);
	range->HsyncPulse = PixelTime * (mode->hend - mode->hbegin);
	range->HbackPorch = PixelTime * (mode->htotal - mode->hend);
	
	range->VfrontPorch = LineTime * (mode->vbegin - mode->vactive);
	range->VsyncPulse = LineTime * (mode->vend - mode->vbegin);
	range->VbackPorch = LineTime * (mode->vtotal - mode->vend);
	range->VerticalBlank = range->VfrontPorch + range->VsyncPulse + range->VbackPorch;
	
	range->HsyncPolarity = mode->hsync;
	range->VsyncPolarity = mode->vsync;
	
	range->ProgressiveLinesMin = mode->vactive;
	range->ProgressiveLinesMax = mode->vactive;
	range->InterlacedLinesMin = 0;
	range->InterlacedLinesMax= 0;

	range->HfreqMin = range->VfreqMin * mode->vtotal;
	range->HfreqMax = range->VfreqMax * mode->vtotal;

	return 1;
}
	
