/*
 *
 *   SwitchRes (C) 2010 Chris Kennedy <c@groovy.org>
 * 
 *   This program is Free Software under the GNU Public License
 *
 *   SwitchRes is a modeline generator based off of Calamity's 
 *   Methods of generating modelines
 *
 */

#ifndef __SWITCHRES_H__
#define __SWITCHRES_H__

#include <stdio.h>
#include <stdlib.h>

#ifdef __CYGWIN__  /* if Cygwin windows */
	#define IS_WIN 1
#else
	#ifdef __WIN32__  /* if Regular windows */
		#define IS_WIN 1
	#else
		#define IS_WIN 0
	#endif
#endif

#define MAX_MODES 10
#define MAX_MODELINES 512
#define MONITOR_TYPES "[cga ega vga multi ntsc pal h9110 d9200 d9800]"
#define STANDARD_CRT_ASPECT 4.0/3.0

#define RESULT_RES_INC      0x00000001
#define RESULT_RES_DEC      0x00000002
#define RESULT_INTERLACE    0x00000004
#define RESULT_DOUBLESCAN   0x00000008
#define RESULT_VIRTUALIZE   0x00000010
#define RESULT_HFREQ_CHANGE 0x00000020
#define RESULT_PADDING      0x00000040
#define RESULT_VFREQ_CHANGE 0x00000080
#define RESULT_VFREQ_DOUBLE 0x00000100

typedef struct ConfigSettings {
	int    verbose;
	char   monitor[256];
	int    interlace;
	int    doublescan;
	char   crt_range[MAX_MODES][256];
	char   lcd_range[256];
	int    monitorcount;
	int    dotclockmin;
	int    dotclockmax;
	int    changeres;
	int    froggerfix;
	int    redraw;
	int    cleanstretch;
	int    soundsync;
	int    vsync;
	int    ymin;
	int    vectorwidth;
	int    vectorheight;
	char   modesfile[255];
	char   logfile[255];
	FILE   *logfd;
	char   resfile[32][255];
	int    resfilecount;
	int    soft15khz;
	char   emulator[255];
	int    is_mame;
	int    is_mess;
	int    always_throttle;
	int    xrandr;
	char   ROM[256];
	int    morientation;
	char   aspect[32];
	int    dcalign;
	int    threads;
	int    triplebuffer;
	int    syncrefresh;
	int    switchres;
	int    version;
} ConfigSettings;

typedef struct MonitorMode {
	double HfreqMin;
	double HfreqMax;
	double VfreqMin;
	double VfreqMax;
	double HfrontPorch;
	double HsyncPulse;
	double HbackPorch;
	double VfrontPorch;
	double VsyncPulse;
	double VbackPorch;
	int    HsyncPolarity;
	int    VsyncPolarity;
	//
	double ActiveLinesLimit;
	int    VirtualLinesLimit;
	//
	int    XresMin;
	int    YresUserMin;
	int    YresMin;
	int    YresMax;
	//
	double VerticalBlank;
	//
	struct ModeLine *ModeLine;
	struct ConfigSettings *cs;
} MonitorMode;

typedef struct MonitorRange
{
	double HfreqMin;
	double HfreqMax;
	double VfreqMin;
	double VfreqMax;
	double HfrontPorch;
	double HsyncPulse;
	double HbackPorch;
	double VfrontPorch;
	double VsyncPulse;
	double VbackPorch;
	int    HsyncPolarity;
	int    VsyncPolarity;
	int    ProgressiveLinesMin;
	int    ProgressiveLinesMax;
	int    InterlacedLinesMin;
	int    InterlacedLinesMax;
	double VerticalBlank;
} MonitorRange;

typedef struct ModeLine {
	char   name[256];
	int    pclock;
	int    hactive;
	int    hbegin;
	int    hend;
	int    htotal;
	int    vactive;
	int    vbegin;
	int    vend;
	double vtotal;
	int    interlace;
	int    doublescan;
	int    hsync;
	int    vsync;
	//
	int    result;
	int    weight;
	int    vpad;
	char   label[256];
	char   resolution[256];
	int    depth;
	//
	double vfreq;
	int    a_width;
	int    a_height;
	double a_vfreq;
	double hfreq;
	//
	char   game[256];
	//
	int    desktop;
	int    custom;
	//
	char   regdata[256];
	int    regdata_size;
} ModeLine;

typedef struct GameInfo {
	char   name[256];
	int    o_width;
	int    o_height;
	double o_refresh;
	double o_aspect;
	int    width;
	int    height;
	int    depth;
	double refresh;
	double aspect;
	int    orientation;
	int    screens;
	int    vector;;
	//
	int    throttle;
	int    changeres;
	int    redraw;
	int    keepaspect;
	int    stretch;
	int    switchres;
	//
	struct ModeLine *ModeLine;
} GameInfo;

/* Modeline Creation */
int ModelineCreate(ConfigSettings *cs, GameInfo *game, MonitorMode *monitor, ModeLine *mode);
int ModelineGetLineParams(ModeLine *mode, MonitorMode *monitor);
int ResVirtualize(ModeLine *mode, MonitorMode *monitor);
int StoreModeline(ModeLine *mode, ConfigSettings *cs);
int modeline_vesa_gtf(ModeLine *m);

/* Modeline Utilities */
char * PrintModeline(ModeLine *mode, char *modeline);
int ModelineResult(ModeLine *mode, ConfigSettings *cs);
int findBestMode(int checkvfreq, ModeLine *mode, ModeLine modes[MAX_MODELINES], ModeLine *bestmode, int modecount);

/* Monitor */
int get_monitor_specs(ConfigSettings *cs, MonitorRange *range);
int convert_monitor_specs(MonitorMode *m, MonitorRange *range);
int show_monitor_range(MonitorRange *range);
int monitor_range_from_modeline(MonitorRange *range, ModeLine *mode);

/* Utilities */
double Normalize(double a, double b);
int RunProgram(ConfigSettings *cs, char *temp);
int get_output(char *command, char *output);
void sr_fprintf(FILE *fd, const char *fmt, ...);

/* config and ini file */
int readConfig(ConfigSettings *cs, char *filename);
int readIni(ConfigSettings *cs, GameInfo *game, char *filename);
int readResolutions(ConfigSettings *cs, ModeLine *mode, char *filename, ModeLine *bestmode);
int readSoft15KhzResolutions(ConfigSettings *cs, ModeLine *mode, ModeLine *bestMode);
int GetMameInfo(ConfigSettings *cs, char *mamearg, char *result);

/* XML File reading */
int GetGameXML(ConfigSettings *cs, GameInfo *game, char *emulator);

/* Xrandr commands */
int GetXrandrDisplay(ConfigSettings *cs, ModeLine *defaultMode);
int SetXrandrDisplay(ConfigSettings *cs, MonitorMode *monitorMode, ModeLine *defaultMode);
//int DelXrandrDisplay(ConfigSettings *cs, MonitorMode *monitorMode, ModeLine *defaultMode, int cpid);
int DelXrandrDisplay(ConfigSettings *cs, MonitorMode *monitorMode, ModeLine *defaultMode);

/* windows registry */
#ifdef __CYGWIN__
int GetAvailableVideoModes(ModeLine VideoMode[MAX_MODELINES], ModeLine *DesktopMode);
int GetCustomVideoModes(ConfigSettings *cs, ModeLine VideoMode[MAX_MODELINES], ModeLine *mode);
int SetCustomVideoModes(ConfigSettings *cs, ModeLine *VideoMode, ModeLine *mode);
#endif

/* edid */
int edid_from_monitor_range(MonitorMode *range, ModeLine *mode, char *edid);

extern FILE *logfd;

#endif
