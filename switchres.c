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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/wait.h>
#include "switchres.h"
#include "version.h"

void clean_up(MonitorMode *monitorModes, ConfigSettings *cs);
int usage(void);

FILE *logfd = NULL;

int main(int argc, char **argv) {
	int i;
	unsigned int j;
	char *s;
	int badarg = 0;
	ModeLine defaultMode;
	ModeLine bestMode;
	GameInfo gameInfo;
	MonitorMode *monitorMode = NULL;
	MonitorMode monitorModes[MAX_MODES];
	MonitorRange monitorRange[MAX_MODES];
	ConfigSettings *cs = NULL;
	char *home, *path;
	char new_path[255];
	char homecfg[255], syscfg[255], gamecfg[255];
	int lowest_weight = 99999;
	int best_weight = 0;
	int calcres = 0;
	int create_edid = 0;
	char modeline[100000]={'\x00'};
	char mamecmd[255] = "groovyume";
	char messcmd[255] = "groovyume";
	char *mamearg[32];
	int mameargc = 0;
	char *extraarg[32];
	int extraargc = 0;
	pid_t cpid;
	int end_of_args = 0;
	int test_mode = 0;
	char mame_version[8];

	/* Put current working directory into path */
	home = getenv("HOME");
	path = getenv("PATH");
	sprintf(new_path, ".:%s:%s", home, path);
	setenv("PATH", new_path, 1);
	path = getenv("PATH");

	if (home && !IS_WIN)
		sprintf(homecfg, "%s/%s", home, "switchres.conf");
	else
		sprintf(homecfg, "%s", "switchres.conf");
	if (IS_WIN) {
		#ifdef __WIN32__
		sprintf(syscfg, "%s", "C:\\etc\\switchres.conf");
		#else
		sprintf(syscfg, "%s", "/cygdrive/c/etc/switchres.conf");
		#endif
	} else
		sprintf(syscfg, "%s", "/etc/switchres.conf");

	/* Read config file */
	cs = (struct ConfigSettings *) malloc(sizeof(struct ConfigSettings));
	memset(cs, 0, sizeof(struct ConfigSettings));
	cs->interlace  = 1;
	cs->doublescan = 1;
	cs->vsync      = 1;
	cs->switchres    = 1;
	cs->triplebuffer = 1;
	cs->vectorwidth  = 640;
	cs->vectorheight = 480;
	sprintf(cs->emulator, "%s", mamecmd);
	sprintf(cs->aspect, "%s", "4:3");
	cs->is_mame = 1;
	cs->is_mess = 0;
	cs->version = 105;
	if (readConfig(cs, syscfg))
		readConfig(cs, homecfg);
	logfd = cs->logfd;

	/* Game information */
	memset(&gameInfo, 0, sizeof(struct GameInfo));
	sprintf(gameInfo.name, "%s", "");

	memset(&bestMode, 0, sizeof(struct ModeLine));
	memset(&defaultMode, 0, sizeof(struct ModeLine));
	memset(monitorModes, 0, sizeof(struct MonitorMode)*MAX_MODES);
	bestMode.depth = 32;

	for(i = 1; i < argc; i++) {
		if (end_of_args) {
			extraarg[extraargc++] = argv[i];
		} else if (*(s = argv[i]) == '-') {
			switch (*(++s)) {
				case '-': // Long Args
					++s;
					if (!strcmp(s, "help")) {
						usage();
						clean_up(monitorModes,cs);
						return 0;
					} else if (!strcmp(s, "version")) {
						fprintf(stdout, "SwitchRes Version %s by Chris Kennedy (C) 2010\n",
							VERSION);
						clean_up(monitorModes,cs);
						return 0;
					} else if (!strcmp(s, "verbose") || !strcmp(s, "v")) {
						cs->verbose++;
					} else if (!strcmp(s, "test")) {
						test_mode = 1;
					} else if (!strcmp(s, "calc")) {
						calcres = 1;
					} else if (!strcmp(s, "edid")) {
						create_edid = 1;
					} else if (!strcmp(s, "soundsync")) {
						cs->soundsync = 1;
					} else if (!strcmp(s, "cleanstretch")) {
						cs->cleanstretch = 1;
					} else if (!strcmp(s, "redraw")) {
						cs->redraw = 1;
					} else if (!strcmp(s, "ff")) {
						cs->froggerfix = 1;
					} else if (!strcmp(s, "novsync")) {
						cs->vsync = 0;
					} else if (!strcmp(s, "nointerlace")) {
						cs->interlace = 0;
					} else if (!strcmp(s, "nodoublescan")) {
						cs->doublescan = 0;
					} else if (!strcmp(s, "throttle")) {
						cs->always_throttle = 1;
					} else if (!strcmp(s, "threads")) {
						cs->threads = 1;
					} else if (!strcmp(s, "notriplebuffer")) {
						cs->triplebuffer = 0;
					} else if (!strcmp(s, "noswitchres")) {
						cs->switchres = 0;
					} else if (!strcmp(s, "xrandr")) {
						cs->xrandr = 1;
					#ifdef __CYGWIN__
					} else if (!strcmp(s, "soft15khz")) {
						cs->soft15khz = 1;
					#endif
					} else if (!strcmp(s, "vectorwidth")) {
						if ((i+1) < argc && *(s = argv[i+1]) != '-') {
							cs->vectorwidth = atoi(s);
							i++;
						} else {
							usage();
							sr_fprintf(stderr,
								"Error with vectorwidth\n");
							clean_up(monitorModes,cs);
							return 0;
						}
					} else if (!strcmp(s, "vectorheight")) {
						if ((i+1) < argc && *(s = argv[i+1]) != '-') {
							cs->vectorheight = atoi(s);
							i++;
						} else {
							usage();
							sr_fprintf(stderr,
								"Error with vectorheight\n");
							clean_up(monitorModes,cs);
							return 0;
						}
					} else if (!strcmp(s, "rom")) {
						if ((i+1) < argc && *(s = argv[i+1]) != '-') {
							sprintf(cs->ROM, "%s", s);
							i++;
						} else {
							usage();
							sr_fprintf(stderr,
								"Error with ROM\n");
							clean_up(monitorModes,cs);
							return 0;
						}
					} else if (!strcmp(s, "emulator")) {
						if ((i+1) < argc && *(s = argv[i+1]) != '-') {
							sprintf(cs->emulator, "%s", s);
							i++;
						} else {
							usage();
							sr_fprintf(stderr,
								"Error with emulator\n");
							clean_up(monitorModes,cs);
							return 0;
						}
					} else if (!strcmp(s, "crt_range0")) {
						if ((i+1) < argc && *(s = argv[i+1]) != '-') {
							sprintf(cs->crt_range[0], "%s", s);
							i++;
						} else {
							usage();
							sr_fprintf(stderr,
								"Error with monitor range\n");
							clean_up(monitorModes,cs);
							return 0;
						}
					} else if (!strcmp(s, "crt_range1")) {
						if ((i+1) < argc && *(s = argv[i+1]) != '-') {
							sprintf(cs->crt_range[1], "%s", s);
							i++;
						} else {
							usage();
							sr_fprintf(stderr,
								"Error with monitor range\n");
							clean_up(monitorModes,cs);
							return 0;
						}
					} else if (!strcmp(s, "crt_range2")) {
						if ((i+1) < argc && *(s = argv[i+1]) != '-') {
							sprintf(cs->crt_range[2], "%s", s);
							i++;
						} else {
							usage();
							sr_fprintf(stderr,
								"Error with monitor range\n");
							clean_up(monitorModes,cs);
							return 0;
						}
					} else if (!strcmp(s, "crt_range3")) {
						if ((i+1) < argc && *(s = argv[i+1]) != '-') {
							sprintf(cs->crt_range[3], "%s", s);
							i++;
						} else {
							usage();
							sr_fprintf(stderr,
								"Error with monitor range\n");
							clean_up(monitorModes,cs);
							return 0;
						}
					} else if (!strcmp(s, "crt_range4")) {
						if ((i+1) < argc && *(s = argv[i+1]) != '-') {
							sprintf(cs->crt_range[4], "%s", s);
							i++;
						} else {
							usage();
							sr_fprintf(stderr,
								"Error with monitor range\n");
							clean_up(monitorModes,cs);
							return 0;
						}
					} else if (!strcmp(s, "crt_range5")) {
						if ((i+1) < argc && *(s = argv[i+1]) != '-') {
							sprintf(cs->crt_range[5], "%s", s);
							i++;
						} else {
							usage();
							sr_fprintf(stderr,
								"Error with monitor range\n");
							clean_up(monitorModes,cs);
							return 0;
						}
					} else if (!strcmp(s, "crt_range6")) {
						if ((i+1) < argc && *(s = argv[i+1]) != '-') {
							sprintf(cs->crt_range[6], "%s", s);
							i++;
						} else {
							usage();
							sr_fprintf(stderr,
								"Error with monitor range\n");
							clean_up(monitorModes,cs);
							return 0;
						}
					} else if (!strcmp(s, "crt_range7")) {
						if ((i+1) < argc && *(s = argv[i+1]) != '-') {
							sprintf(cs->crt_range[7], "%s", s);
							i++;
						} else {
							usage();
							sr_fprintf(stderr,
								"Error with monitor range\n");
							clean_up(monitorModes,cs);
							return 0;
						}
					} else if (!strcmp(s, "crt_range8")) {
						if ((i+1) < argc && *(s = argv[i+1]) != '-') {
							sprintf(cs->crt_range[8], "%s", s);
							i++;
						} else {
							usage();
							sr_fprintf(stderr,
								"Error with monitor range\n");
							clean_up(monitorModes,cs);
							return 0;
						}
					} else if (!strcmp(s, "crt_range9")) {
						if ((i+1) < argc && *(s = argv[i+1]) != '-') {
							sprintf(cs->crt_range[9], "%s", s);
							i++;
						} else {
							usage();
							sr_fprintf(stderr,
								"Error with monitor range\n");
							clean_up(monitorModes,cs);
							return 0;
						}
					} else if (!strcmp(s, "lcd_range")) {
						if ((i+1) < argc && *(s = argv[i+1]) != '-') {
							sprintf(cs->lcd_range, "%s", s);
							i++;
						} else {
							usage();
							sr_fprintf(stderr,
								"Error with monitor range\n");
							clean_up(monitorModes,cs);
							return 0;
						}
					} else if (!strcmp(s, "monitor")) {
						if ((i+1) < argc && *(s = argv[i+1]) != '-') {
							sprintf(cs->monitor, "%s", s);
							i++;
						} else {
							usage();
							sr_fprintf(stderr,
								"Error with monitor mode\n");
							clean_up(monitorModes,cs);
							return 0;
						}
					} else if (!strcmp(s, "mo")) {
						if ((i+1) < argc && *(s = argv[i+1]) != '-') {
							cs->morientation = atoi(s);
							i++;
						} else {
							usage();
							sr_fprintf(stderr,
								"Error with -mo\n");
							clean_up(monitorModes,cs);
							return 0;
						}
					} else if (!strcmp(s, "aspect")) {
						if ((i+1) < argc && *(s = argv[i+1]) != '-') {
							sprintf(cs->aspect, "%s", s);
							i++;
						} else {
							usage();
							sr_fprintf(stderr,
								"Error with -aspect\n");
							clean_up(monitorModes,cs);
							return 0;
						}
					} else if (!strcmp(s, "dcalign")) {
						if ((i+1) < argc && *(s = argv[i+1]) != '-') {
							cs->dcalign = atoi(s);
							i++;
						} else {
							usage();
							sr_fprintf(stderr,
								"Error with -dcalign\n");
							clean_up(monitorModes,cs);
							return 0;
						}
					} else if (!strcmp(s, "ymin")) {
						if ((i+1) < argc && *(s = argv[i+1]) != '-') {
							cs->ymin = atoi(s);
							i++;
						} else {
							usage();
							sr_fprintf(stderr,
								"Error with -ymin\n");
							clean_up(monitorModes,cs);
							return 0;
						}
					} else if (!strcmp(s, "logfile")) {
						if ((i+1) < argc && *(s = argv[i+1]) != '-') {
							if (cs->logfd)
								fclose(cs->logfd);
                        				sprintf(cs->logfile, "%s", s);
							cs->logfd = fopen(cs->logfile, "a+");
							if (cs->logfd == NULL)
								fprintf(stderr, "Error opening logfile %s\n", cs->logfile);
							logfd = cs->logfd;
							i++;
						} else {
							usage();
							sr_fprintf(stderr,
								"Error with logfile\n");
							clean_up(monitorModes,cs);
							return 0;
						}
					} else if (!strcmp(s, "modesfile")) {
						if ((i+1) < argc && *(s = argv[i+1]) != '-') {
                        				sprintf(cs->modesfile, "%s", s);
							i++;
						} else {
							usage();
							sr_fprintf(stderr,
								"Error with modesfile\n");
							clean_up(monitorModes,cs);
							return 0;
						}
					} else if (!strcmp(s, "resfile")) {
						if ((i+1) < argc && *(s = argv[i+1]) != '-') {
                        				sprintf(cs->resfile[cs->resfilecount],
								"%s", s);
							cs->resfilecount++;
							i++;
						} else {
							usage();
							sr_fprintf(stderr,
								"Error with resfile\n");
							clean_up(monitorModes,cs);
							return 0;
						}
					} else if (!strcmp(s, "args")) {
						end_of_args = 1;
					} else {
						usage();
						sr_fprintf(stderr,
							"Error with arg %s\n",
							s);
						clean_up(monitorModes,cs);
						return 0;
					}
					break;
				case 'h':
					usage();
					clean_up(monitorModes,cs);
					return 0;
				case 'v':
					cs->verbose++;
					break;
				default:
					badarg = 1;
			}
		} else if (argc > (i+2) &&
			    atoi(argv[i]) > 0 &&
			    atoi(argv[i]) < 4096 &&
			    *(argv[i+1]) != '-' &&
			    *(argv[i+2]) != '-' &&
			    *(s = argv[i]))
		{
			/* Height and width input */
			gameInfo.width = gameInfo.o_width = Normalize(atoi(argv[i]), 8);
			gameInfo.height = gameInfo.o_height = Normalize(atoi(argv[i+1]), 8);
			gameInfo.refresh = atof(argv[i+2]);
			gameInfo.aspect = gameInfo.o_aspect =
				(double)gameInfo.width / (double)gameInfo.height;
			if (!strcmp(gameInfo.name, ""))
				calcres = 1;
			if (gameInfo.width < gameInfo.height) {
				gameInfo.orientation = 1;
				gameInfo.width = (4.0/3.0) * ((double)gameInfo.width * (4.0/3.0));
			}
			i+=2;
		} else {
			/* Game input */
			char *romname = NULL;
			if (strstr(argv[i], ".")) {
				romname = strtok(argv[i], ".");
				sprintf(gameInfo.name, "%s", romname);
			} else
				sprintf(gameInfo.name, "%s", argv[i]);
		}
	}

	// Process monitor specs
	get_monitor_specs(cs, monitorRange);

	// Create automatic specs and force resolution for LCD monitors
	if (!strcmp(cs->monitor, "lcd"))
	{
		char desktop_res[32]={'\x00'};
		ModeLine current;
		memset(&current, 0, sizeof(struct ModeLine));

		// Get current resolution
		sr_fprintf(stderr, "SwitchRes: Creating automatic specs for LCD based on VESA GTF\n");
		get_output((char*)"xrandr -q | grep '[0-9]\\*'", desktop_res);
		sscanf(desktop_res, " %s ", desktop_res);
		sscanf(desktop_res, "%dx%d", &current.a_width, &current.a_height);
		sr_fprintf(stderr, "%dx%d\n", current.a_width, current.a_height);
		current.a_vfreq = 60.0;
		gameInfo.width = gameInfo.o_width = current.a_width;
		gameInfo.height = gameInfo.o_height = current.a_height;
		modeline_vesa_gtf(&current);
		monitor_range_from_modeline(monitorRange, &current);
		show_monitor_range(monitorRange);
	}

	// Convert monitor specs to old format
	convert_monitor_specs(monitorModes, monitorRange);

	if (!strcmp(gameInfo.name, "") && !calcres && badarg) {
		execvp(cs->emulator, argv);
                {
                        char *path = NULL;
                        char *envp = getenv("PATH");
                        path = strtok(envp, ":");
                        while(path != NULL) {
                                char executable[255];
                                sprintf(executable, "%s/%s", path, cs->emulator);
                                execvp(executable, argv);
                                if (cs->verbose > 2)
                                        sr_fprintf(stderr, "Failed running %s\n", executable);
                                path = strtok(NULL, ":");
                        }
                }
	} else if (badarg || argc == 1 || (!strcmp(gameInfo.name, "") && !gameInfo.width)) {
		usage();
		sr_fprintf(stderr, "Error with command line args\n");
		clean_up(monitorModes,cs);
		return 0;
	}

	/* Check emulator names */
	if (!strcasestr(cs->emulator, "mess"))
		cs->is_mess = 0;
	else {
		sprintf(messcmd, "%s", cs->emulator);
		cs->is_mess = 1;
	}
	if (!strcasestr(cs->emulator, "mame"))
		cs->is_mame = 0;
	else {
		sprintf(mamecmd, "%s", cs->emulator);
		cs->is_mame = 1;
	}

	/* change game name to lower case */
	for (j = 0; j < strlen(gameInfo.name); j++)
		gameInfo.name[j] = (char)tolower((int)gameInfo.name[j]);

	/* Get game XML */
	if (strcmp(gameInfo.name, "")) {
		if (gameInfo.width == 0 || gameInfo.height == 0 || gameInfo.refresh == 0) {
			if (GetGameXML(cs, &gameInfo, mamecmd))
				if (GetGameXML(cs, &gameInfo, messcmd))
					sr_fprintf(stderr, "Unknown ROM for mame and mess %s\n",
						gameInfo.name);
		}

		/* Check for an .ini file for the game */
		sprintf(gamecfg, "%s/inisw/%s.ini", home, gameInfo.name);
		readIni(cs, &gameInfo, gamecfg);
	} else
		gameInfo.aspect = (double)gameInfo.width / (double)gameInfo.height;

	/* We should have a height width and refresh rate by now */
	if (!gameInfo.width || !gameInfo.height || !gameInfo.refresh) {
		sr_fprintf(stderr, "Could not find height width or refresh rate to use.\n");
		gameInfo.width = gameInfo.o_width = cs->vectorwidth;
		gameInfo.height = gameInfo.o_height = cs->vectorheight;
		gameInfo.aspect = gameInfo.o_aspect = gameInfo.width / gameInfo.height;
		gameInfo.refresh = gameInfo.o_refresh = 60.00;
	}

	defaultMode.hactive = cs->vectorwidth;
	defaultMode.vactive = cs->vectorheight;
	defaultMode.depth = 32;
	defaultMode.vfreq = 60.00;

	/* Get mame version */
	if ((cs->is_mame || cs->is_mess) && !GetMameInfo(cs, "-h", modeline)) {
		sprintf(mame_version, "%c%c%c",
		 	modeline[12], modeline[13], modeline[14]);
		sscanf(mame_version, "%d", &cs->version);
		if (cs->verbose > 4)
			sr_fprintf(stderr, "Mame -h output: %s\n", modeline);
	}

	/* Mame hacks and config settings */
	if ((cs->is_mame || cs->is_mess) && !GetMameInfo(cs, "-showconfig", modeline)) {
		if (strstr(modeline, "cleanstretch"))
			cs->cleanstretch = 1;
		if (strstr(modeline, "frogger") ||
			strstr(cs->emulator, "cabmame"))
				cs->froggerfix = 1;
		if (strstr(modeline, "redraw"))
			cs->redraw = 1;
		if (strstr(modeline, "syncrefresh"))
			cs->syncrefresh = 1;
		if (strstr(modeline, "soundsync") ||
			strstr(cs->emulator, "cabmame"))
				cs->soundsync = 1;
		if (strstr(modeline, "changeres") ||
			strstr(cs->emulator, "cabmame"))
				cs->changeres = 1;
		if (cs->verbose > 4)
			sr_fprintf(stderr, "Mame Config: %s\n", modeline);
	}

	if (cs->verbose > 0)
		sr_fprintf(stderr, "Mame version 0.%d with [%s][%s][%s][%s][%s] hacks enabled\n",
			cs->version,
			cs->cleanstretch?"cleanstretch":"",
			cs->froggerfix?"froggerfix":"",
			cs->redraw?"redraw":"",
			cs->changeres?"changeres":"",
			cs->soundsync?"soundsync":"");

	/* Game stats or mode stats */
	if (!calcres || cs->verbose > 0)
		sr_fprintf(stderr,
			"[%s] \"%s\" %s %dx%d@%.3f (%.3f) --> %dx%d@%.3f (%.3f)\n\n",
			cs->monitor,
			strcmp(gameInfo.name,"")?gameInfo.name:"resolution",
			(gameInfo.orientation)?"vertical":"horizontal",
			gameInfo.o_width, gameInfo.o_height, gameInfo.o_refresh, gameInfo.o_aspect,
			gameInfo.width, gameInfo.height, gameInfo.refresh, gameInfo.aspect);

	/* Get modeline for each monitor mode */
	for (i = 0 ; i < MAX_MODES ; i++) {
		if (monitorModes[i].HfreqMin)
		{
			/* Modeline */
			struct ModeLine *modeLine = NULL;
			modeLine = (struct ModeLine *) malloc(sizeof(struct ModeLine));
			memset(modeLine, 0, sizeof(struct ModeLine));

			monitorMode = &monitorModes[i];
			monitorMode->cs = cs;

			if (monitorMode->HfreqMin == monitorMode->HfreqMax)
				monitorMode->HfreqMin -= .01;

			if (cs->verbose)
				show_monitor_range(&monitorRange[i]);

			/* create modeline */
			ModelineCreate(cs, &gameInfo, monitorMode, modeLine);
			monitorMode->ModeLine = modeLine;
			monitorMode->ModeLine->weight = ModelineResult(modeLine, cs);
			if (cs->verbose) {
			        sr_fprintf(stdout, "# %s [%d] %dx%d@%.2f %.4fKhz\n", gameInfo.name,
					monitorMode->ModeLine->weight,
					monitorMode->ModeLine->hactive, monitorMode->ModeLine->vactive,
					monitorMode->ModeLine->vfreq, monitorMode->ModeLine->hfreq/1000);
				sr_fprintf(stdout, "%s\n", PrintModeline(monitorMode->ModeLine, modeline));
				sr_fprintf(stdout, "\n");
			}
		}
	}

	/* Find best weight */
	for (i = 0 ; i < MAX_MODES ; i++) {
		if (monitorModes[i].HfreqMin)
                {
			monitorMode = &monitorModes[i];
			if (monitorMode->ModeLine->weight < lowest_weight)
			{
				lowest_weight = monitorMode->ModeLine->weight;
				best_weight = i;
			}
		}
	}

	/* Best weight */
	monitorMode = &monitorModes[best_weight];
	gameInfo.ModeLine = monitorMode->ModeLine;

	/* If only printout out resolution then exit */
	if (calcres)
	{
		fprintf(stdout, "# %s %dx%d@%.2f %.4fKhz\n",
			gameInfo.name,
			monitorMode->ModeLine->hactive, monitorMode->ModeLine->vactive,
			monitorMode->ModeLine->vfreq, monitorMode->ModeLine->hfreq/1000);
		fprintf(stdout, "\tModeLine     %s\n", PrintModeline(monitorMode->ModeLine, modeline));
		if (create_edid)
		{
  			FILE * p_edid_file;
  			p_edid_file = fopen("edid.bin","w");
  			if (p_edid_file != NULL)
  			{
  				char edid[128];
				edid_from_monitor_range(monitorMode, monitorMode->ModeLine, edid);
  				fwrite ((const void *)edid, sizeof(char), 128, p_edid_file);
  				fclose (p_edid_file);
  			}
		}
		clean_up(monitorModes,cs);
		return 0;
	}

	/* if given input resolution files find the best match */
	if (cs->switchres && !cs->soft15khz) {
		for (i = 0; i < cs->resfilecount; i++)
			readResolutions(cs, gameInfo.ModeLine, cs->resfile[i], &bestMode);
	} else if (cs->switchres && cs->soft15khz) {
		#ifdef __CYGWIN__
		ModeLine DesktopMode;
		ModeLine AvailableVideoMode[MAX_MODELINES];
		readSoft15KhzResolutions(cs, gameInfo.ModeLine, &bestMode);
		if (cs->verbose && strcmp(bestMode.label, ""))
			sr_fprintf(stderr, "Got Soft15khz registry modeline %s - %s:\n\t%s\n",
				bestMode.resolution, bestMode.label, PrintModeline(&bestMode, modeline));

		if (strcmp(bestMode.label, "")) {
			int dovirt = 0;
			if (gameInfo.height < bestMode.a_height)
				dovirt = 1;
			gameInfo.width = bestMode.a_width;
			gameInfo.height = bestMode.a_height;
			ModelineCreate(cs, &gameInfo, monitorMode, monitorMode->ModeLine);
			monitorMode->ModeLine->weight = ModelineResult(monitorMode->ModeLine, cs);
			if (!SetCustomVideoModes(cs, &bestMode, gameInfo.ModeLine)) {
				GetAvailableVideoModes(AvailableVideoMode, &DesktopMode);
			}
			// Height is not enough to fit original into
			if (dovirt)
				monitorMode->ModeLine->weight |= RESULT_VIRTUALIZE;
		} else
			cs->soft15khz = 0;
		#endif
	} else if (!cs->switchres) {
		cs->soft15khz = 0;
		cs->resfilecount = 0;
	}

	/* Write out to modes file */
	StoreModeline(gameInfo.ModeLine, cs);

	/* Found best resolution from file */
	if ((cs->resfilecount > 0 || cs->soft15khz > 0) && (!bestMode.hactive || !bestMode.vactive || !bestMode.vfreq)) {
		sr_fprintf(stderr, "Warning: no best mode match in resolution files!!!\n");
		cs->resfilecount = -1;
		cs->soft15khz = -1;
	} else {
		if (!strcmp(bestMode.name, "")) {
			if (cs->version > 104 && cs->version > 0)
				sprintf(bestMode.name, "%dx%d@%d", bestMode.hactive, bestMode.vactive, (int)bestMode.vfreq);
			else
				sprintf(bestMode.name, "%dx%d", bestMode.hactive, bestMode.vactive);
		}

		/* check width/height for virtualization */
		if (cs->resfilecount > 0 && (bestMode.hactive != gameInfo.width ||
			bestMode.vactive != gameInfo.height))
		{
			monitorMode->ModeLine->result |= RESULT_VIRTUALIZE;
		}

		/* check refresh for change */
		if (cs->resfilecount > 0 && (bestMode.vfreq < (gameInfo.refresh-0.20) ||
			bestMode.vfreq > (gameInfo.height+0.20)))
		{
			monitorMode->ModeLine->result |= RESULT_VFREQ_CHANGE;
		}
	}

	if (cs->is_mame && cs->redraw) {
		if (monitorMode->ModeLine->result & RESULT_VFREQ_DOUBLE)
			gameInfo.redraw = 1;
	}

	if (cs->is_mame || cs->is_mess) {
		if (!cs->soundsync && monitorMode->ModeLine->result & RESULT_VFREQ_CHANGE)
			gameInfo.throttle = 1;
		if (monitorMode->ModeLine->result & (RESULT_RES_INC|RESULT_RES_DEC))
			gameInfo.stretch = 1;
		/*if (gameInfo.orientation) {
			gameInfo.keepaspect = 1;
			gameInfo.stretch = 1;
		}*/

		if (gameInfo.screens > 1 ||
		    monitorMode->ModeLine->result & RESULT_VIRTUALIZE)
		{
			if (!IS_WIN)
				gameInfo.keepaspect = 1;
			gameInfo.stretch = 1;
		}
		if (cs->changeres) {
			if (gameInfo.stretch)
				gameInfo.changeres = 0;
			else
				gameInfo.changeres = 1;
		}
	}

	/* Setup game parameters to run the emulator with */
	mamearg[mameargc++] = cs->emulator;
	if (cs->is_mame || cs->is_mess) {
		mamearg[mameargc++] = gameInfo.name;
		if (cs->is_mess) {
			mamearg[mameargc++] = "-cart";
			mamearg[mameargc++] = cs->ROM;
		}
		if (cs->switchres) {
			if (cs->resfilecount > 0 || cs->soft15khz > 0) {
				mamearg[mameargc++] = "-resolution";
				mamearg[mameargc++] = bestMode.name;
				mamearg[mameargc++] = "-resolution0";
				mamearg[mameargc++] = bestMode.name;
			} else if (!cs->xrandr && cs->resfilecount >= 0 && cs->soft15khz >= 0) {
				mamearg[mameargc++] = "-resolution";
				mamearg[mameargc++] = gameInfo.ModeLine->resolution;
				mamearg[mameargc++] = "-resolution0";
				mamearg[mameargc++] = gameInfo.ModeLine->resolution;
			}
			if (cs->version < 104 && cs->version > 0) {
				mamearg[mameargc++] = "-refresh";
				char vfreq[32] = "";
				if (IS_WIN) {
					if (cs->resfilecount > 0 || cs->soft15khz > 0) {
						sprintf(vfreq, "%d", (int)bestMode.vfreq);
						mamearg[mameargc++] = vfreq;
					} else {
						sprintf(vfreq, "%d", (int)gameInfo.ModeLine->vfreq);
						mamearg[mameargc++] = vfreq;
					}
				} else {
					if (cs->resfilecount > 0 || cs->soft15khz > 0) {
						sprintf(vfreq, "%.2f", bestMode.vfreq);
						mamearg[mameargc++] = vfreq;
					} else {
						sprintf(vfreq, "%.2f", gameInfo.ModeLine->vfreq);
						mamearg[mameargc++] = vfreq;
					}
				}
			}
		} else
			mamearg[mameargc++] = "-noswitchres";

		// Force sane video output for an Arcade Monitor
		if (IS_WIN) {
			mamearg[mameargc++] = "-video";
			mamearg[mameargc++] = "ddraw";
		} else {
			mamearg[mameargc++] = "-video";
			mamearg[mameargc++] = "opengl";
		}
		// Horizontal oriented monitor
		if (cs->morientation == 0)
			mamearg[mameargc++] = "-rotate";

		// Vertical oriented monitor
		if (cs->morientation == 1) {
			if (gameInfo.orientation == 1) {
				mamearg[mameargc++] = "-rotate";
				mamearg[mameargc++] = "-autorol";
			} else {
				mamearg[mameargc++] = "-rotate";
				mamearg[mameargc++] = "-rol";
			}
		}

		// Vertical and Horizontal rotating monitor
		if (cs->morientation == 2) {
			if (gameInfo.orientation == 1) {
				mamearg[mameargc++] = "-rotate";
				mamearg[mameargc++] = "-autorol";
			} else {
				mamearg[mameargc++] = "-rotate";
			}
		}
	}

	if ((gameInfo.orientation && !cs->morientation) || (!gameInfo.orientation && (cs->morientation == 1))) {
		if (monitorMode->ModeLine->result & RESULT_VIRTUALIZE) {
			mamearg[mameargc++] = "-screen_aspect";
			mamearg[mameargc++] = cs->aspect;
		}
	}

	if (gameInfo.keepaspect)
		mamearg[mameargc++] = "-keepaspect";

	if (gameInfo.stretch) {
		if (cs->cleanstretch)
			mamearg[mameargc++] = "-nocleanstretch";

		if (IS_WIN)
			mamearg[mameargc++] = "-hwstretch";
		else
			mamearg[mameargc++] = "-unevenstretch";
		mamearg[mameargc++] = "-filter";

		if (cs->changeres && !gameInfo.changeres)
			mamearg[mameargc++] = "-nochangeres";
	} else {
		if (cs->cleanstretch)
			mamearg[mameargc++] = "-nocleanstretch";
	}

	if (gameInfo.redraw) {
		mamearg[mameargc++] = "-redraw";
		mamearg[mameargc++] = "auto";
	}
	if (cs->is_mame || cs->is_mess) {
		if (cs->always_throttle || gameInfo.throttle) {
			mamearg[mameargc++] = "-throttle";
			mamearg[mameargc++] = "-mt";
		} else {
			mamearg[mameargc++] = "-nothrottle";
			if (!cs->threads)
				mamearg[mameargc++] = "-nomt";
			else
				mamearg[mameargc++] = "-mt";
			if (cs->syncrefresh)
				mamearg[mameargc++] = "-syncrefresh";
			if (IS_WIN && cs->triplebuffer) {
				mamearg[mameargc++] = "-triplebuffer";
			} else
				mamearg[mameargc++] = "-waitvsync";
		}
	}

	/* Add emulators extra args */
	for (i = 0; i < extraargc; i++)
		mamearg[mameargc++] = extraarg[i];

	/* Add on ROM file */
	if (!cs->is_mame && !cs->is_mess)
		mamearg[mameargc++] = cs->ROM;

	/* Terminate args */
	mamearg[mameargc] = NULL;

	/* Print out command line */
	if (cs->verbose > 1) {
		sr_fprintf(stderr, "Running Emulator: ");
		for (i = 0; i < mameargc; i++)
			sr_fprintf(stderr, "%s ", mamearg[i]);
		sr_fprintf(stderr, "\n");
	}

#ifdef SYS_LINUX
	if (cs->switchres && !cs->resfilecount && !test_mode) {
		/* Get display information with Xrandr */
		GetXrandrDisplay(cs, &defaultMode);

		/* Setup modeline with Xrandr */
		SetXrandrDisplay(cs, monitorMode, &defaultMode);
	}
#endif

	/* Fork and run mame, wait for pid to exit */
	if (!test_mode) {
		cpid = fork();
		switch (cpid) {
			case -1: sr_fprintf(stderr, "Failed running %s\n", cs->emulator);
				break;
			case 0: execvp(cs->emulator, mamearg);
                        	{
                                	char *path = NULL;
                                	char *envp = getenv("PATH");
                                	path = strtok(envp, ":");
                                	while(path != NULL) {
                                        	char executable[255];
                                        	sprintf(executable, "%s/%s", path, cs->emulator);
                                        	execvp(executable, mamearg);
                                        	if (cs->verbose > 2)
                                                	sr_fprintf(stderr, "Failed running %s\n", executable);
                                        	path = strtok(NULL, ":");
                                	}
                        	}

				sr_fprintf(stderr, "Error running %s\n", cs->emulator);
				cpid = -1;
				break;
			default:
				waitpid(cpid, NULL, 0);
				break;
		}
	} else
		cpid = 0;

#ifdef SYS_LINUX
	if (cs->switchres && !cs->resfilecount && !test_mode) {
		/* Remove modeline with Xrandr */
		DelXrandrDisplay(cs, monitorMode, &defaultMode);
	}
#endif
#ifdef __CYGWIN__
	if (cs->switchres && cs->soft15khz && strcmp(bestMode.label, "")) {
		ModeLine DesktopMode;
		ModeLine AvailableVideoMode[MAX_MODELINES];
		if (!SetCustomVideoModes(cs, &bestMode, &bestMode))
			GetAvailableVideoModes(AvailableVideoMode, &DesktopMode);
	}
#endif

	/* Free things and exit */
	clean_up(monitorModes,cs);
	return 0;
}

void clean_up(MonitorMode *monitorModes, ConfigSettings *cs) {
	int i;
	for (i = 0 ; i < MAX_MODES; i++)
		if (monitorModes[i].ModeLine)
			free(monitorModes[i].ModeLine);
	if (cs)
	{
		if (cs->logfd)
			fclose(cs->logfd);
		free(cs);
	}
}

int usage(void) {
	fprintf(stderr, "SwitchRes version %s by Chris Kennedy (C) 2010 - updated by Antonio Giner (C) 2014\n", VERSION);
	fprintf(stderr, "General purpose modeline generator\n\n");
	fprintf(stderr, "Build Date:   %s\n", BUILD_DATE);
	fprintf(stderr, "Compiler:     GCC %s\n", __VERSION__);
	fprintf(stderr, "Build System: %s\n\n", SYSTEM_VERSION);
	fprintf(stderr, "Usage: switchres <gamerom>\n");
	fprintf(stderr, "Usage: switchres <system> --emulator mess --rom <rom> --args <mess cmdline>\n");
	fprintf(stderr, "Usage: switchres <width> <height> <refresh>\n");
	fprintf(stderr, "Usage: switchres --calc <gamerom>\n\n");
	fprintf(stderr, "  --verbose               Increases verbosity, can be specified multiple times\n");
	fprintf(stderr, "  --logfile <file>        Write out all messages to a file\n");
	fprintf(stderr, "  --calc                  Calculate modeline and exit\n");
	fprintf(stderr, "  --edid                  Create edid.bin based on the calculated modeline and monitor ranges\n");
	fprintf(stderr, "  --monitor  <type>       Type of monitor:\n");
	fprintf(stderr, "                          custom          Define your own custom by means of -crt_range lines\n");
	fprintf(stderr, "                          pal             PAL TV - 50 Hz/625\n");
	fprintf(stderr, "                          ntsc            NTSC TV - 60 Hz/525\n");
	fprintf(stderr, "                          generic_15      Generic 15.7 kHz\n");
	fprintf(stderr, "                          arcade_15       Arcade 15.7 kHz - standard resolution\n");
	fprintf(stderr, "                          arcade_15ex     Arcade 15.7-16.5 kHz - extended resolution\n");
	fprintf(stderr, "                          arcade_25       Arcade 25.0 kHz - medium resolution\n");
	fprintf(stderr, "                          arcade_31       Arcade 31.5 kHz - high resolution\n");
	fprintf(stderr, "                          arcade_15_25    Arcade 15.7/25.0 kHz - dual-sync\n");
	fprintf(stderr, "                          arcade_15_25_31 Arcade 15.7/25.0/31.5 kHz - tri-sync\n");
	fprintf(stderr, "                          m2929           Makvision 2929D\n");
	fprintf(stderr, "                          d9200           Wells Gardner D9200\n");
	fprintf(stderr, "                          d9400           Wells Gardner D9400\n");
	fprintf(stderr, "                          d9800           Wells Gardner D9800\n");
	fprintf(stderr, "                          k7000           Wells Gardner K7000\n");
	fprintf(stderr, "                          k7131           Wells Gardner 25K7131\n");
	fprintf(stderr, "                          m3129           Wei-Ya M3129\n");
	fprintf(stderr, "                          h9110           Hantarex MTC 9110\n");
	fprintf(stderr, "                          polo            Hantarex Polo\n");
	fprintf(stderr, "                          pstar           Hantarex Polostar 25\n");
	fprintf(stderr, "                          ms2930          Nanao MS-2930, MS-2931\n");
	fprintf(stderr, "                          ms929           Nanao MS9-29\n");
	fprintf(stderr, "                          r666b           Rodotron 666B-29\n");
	fprintf(stderr, "                          pc_31_120       PC CRT 70kHz/120Hz\n");
	fprintf(stderr, "                          pc_70_120       PC CRT 70kHz/120Hz\n");
	fprintf(stderr, "                          vesa_480        VESA GTF  640 x 480\n");
	fprintf(stderr, "                          vesa_600        VESA GTF  800 x 600\n");
	fprintf(stderr, "                          vesa_768        VESA GTF 1024 x 768\n");
	fprintf(stderr, "                          vesa_1024       VESA GTF 1280 x 1080\n");
	fprintf(stderr, "  --crt_range[0-9]        Custom CRT range format (all values required in the same line):\n");
	fprintf(stderr, "                          HfreqMin-HfreqMax, (values in kHz)\n");
	fprintf(stderr, "                          VfreqMin-VfreqMax, (values in Hz)\n");
	fprintf(stderr, "                          HFrontPorch, HSyncPulse, HBackPorch, (values in microseconds)\n");
	fprintf(stderr, "                          VfrontPorch, VSyncPulse, VBackPorch, (values in miliseconds)\n");
	fprintf(stderr, "                          HSyncPol, VSyncPol, (0 = negative, 1 = positive)\n");
	fprintf(stderr, "                          ProgressiveLinesMin, ProgressiveLinesMax,(number of lines)\n");
	fprintf(stderr, "                          InterlacedLinesMin, InterlacedLinesMax (number of lines)\n");
	fprintf(stderr, "  --lcd_range             Custom LCD range format (only one is allowed):\n");
	fprintf(stderr, "                          VfreqMin-VfreqMax (values in Hz)\n");
	fprintf(stderr, "  --ymin <height>         Minimum height to use\n");
	fprintf(stderr, "  --vectorwidth <width>   Default width to use on Vector games (640)\n");
	fprintf(stderr, "  --vectorheight <height> Default height to use on Vector games (480)\n");
	fprintf(stderr, "  --nodoublescan          Video card can't doublescan\n");
	fprintf(stderr, "  --nointerlace           Video card can't interlace\n");
	fprintf(stderr, "  --throttle              Can't sync to refresh rate, use throttle in mame\n");
	fprintf(stderr, "  --threads               Use multithreading even with vsync, requires special mame build\n");
	fprintf(stderr, "  --novsync               Weight for x/y instead of vertical refresh\n");
	fprintf(stderr, "  --noswitchres           Don't switch resolutions, just use as wrapper to mame, best for LCD's\n");
	fprintf(stderr, "  --notriplebuffer        Use Mame option waitvsync instead of triple buffer (Windows)\n");
	fprintf(stderr, "  --aspect num:den        Method of calculating width on vertical games, default 4:3\n");
	fprintf(stderr, "  --mo [0|1|2]            Monitor Orientation, 0=horizontal, 1=vertical, 2=both/rotateable\n");
	fprintf(stderr, "  --dcalign <HZ>          Align dotclock to Hz (Windows 10000 Linux 1000)\n");
	fprintf(stderr, "  --ff                    Frogger/Galaxian Hack is in mame executable\n");
	fprintf(stderr, "  --redraw                Redraw Hack is in mame executable\n");
	fprintf(stderr, "  --soundsync             Sound Sync Hack is in mame executable\n");
	fprintf(stderr, "  --cleanstretch          Cleanstretch Hack is in mame executable\n");
	fprintf(stderr, "  --modesfile <file>      Write out modelines to a file for Soft15Khz\n");
	fprintf(stderr, "  --resfile <file>        Read modelines from a file, newer ArcadeVGA Card\n");
	#ifdef __CYGWIN__
	fprintf(stderr, "  --soft15khz             Read modelines from registry, Soft15Khz/Older AVGA\n");
	#endif
	fprintf(stderr, "  --test                  Just test what would be done, no res change or game\n");
	fprintf(stderr, "\nOptions for other emulators besides MAME:\n");
	fprintf(stderr, "  --emulator <exe>        Emulator to run, default is mame\n");
	fprintf(stderr, "  --xrandr                Use this if the emulator doesn't switch resolutions\n");
	fprintf(stderr, "  --args <...>            Extra command line args to send to emulator\n");
	fprintf(stderr, "  --rom <file>            ROM file, normal <gamerom> becomes system type\n\n");
	return 0;
}
