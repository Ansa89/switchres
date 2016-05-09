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
#include <errno.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#ifdef SYS_MACOSX
#include <sys/wait.h>
#else
#include <wait.h>
#endif

#include "switchres.h"

int readConfig(ConfigSettings *cs, char *filename) {
	FILE *f = NULL;
	char line[1000], tmp[1000], tmp2[1000];
	struct stat sb;

	if (!filename)
		return 1;
	
	if (stat(filename, &sb) == -1)
		return -1;
	else if (cs->verbose)
		sr_fprintf(stderr, "Opening %s config file\n", filename);

	f = fopen(filename, "r");
	if (f == NULL)
		return -EBADF;
		
	while(!feof(f)) {
		int e= fscanf(f, "%999[^\n]\n", line) - 1;
		if(line[0] == '#' && !e)
			continue;
		e|= sscanf(line, "%999[^=]=%999[^\n]\n", tmp, tmp2) - 2;

		if (e) {
			sr_fprintf(stderr, "%s: Config Invalid syntax: '%s'\n", filename, line);
			continue;
		}

		if (!strcmp(tmp, "interlace")) {
			cs->interlace = atoi(tmp2);
		} else if (!strcmp(tmp, "doublescan")) {
			cs->doublescan = atoi(tmp2);
		} else if (!strcmp(tmp, "vectorwidth")) {
			cs->vectorwidth = atoi(tmp2);
		} else if (!strcmp(tmp, "vectorheight")) {
			cs->vectorheight = atoi(tmp2);
		} else if (!strcmp(tmp, "verbose") || !strcmp(tmp, "v")) {
			cs->verbose = atoi(tmp2);
		} else if (!strcmp(tmp, "ymin")) {
			cs->ymin = atoi(tmp2);
		} else if (!strcmp(tmp, "ff")) {
			cs->froggerfix = atoi(tmp2);
		} else if (!strcmp(tmp, "cleanstretch")) {
			cs->cleanstretch = atoi(tmp2);
		} else if (!strcmp(tmp, "redraw")) {
			cs->redraw = atoi(tmp2);
		} else if (!strcmp(tmp, "throttle")) {
			cs->always_throttle = atoi(tmp2);
		} else if (!strcmp(tmp, "threads")) {
			cs->threads = atoi(tmp2);
		} else if (!strcmp(tmp, "triplebuffer")) {
			cs->triplebuffer = atoi(tmp2);
		} else if (!strcmp(tmp, "switchres")) {
			cs->switchres = atoi(tmp2);
		} else if (!strcmp(tmp, "soundsync")) {
			cs->soundsync = atoi(tmp2);
		} else if (!strcmp(tmp, "mon") || !strcmp(tmp, "monitor")) {
			sprintf(cs->monitor, "%s", tmp2);
		} else if (!strcmp(tmp, "logfile")) {
			sprintf(cs->logfile, "%s", tmp2);
			cs->logfd = fopen(cs->logfile, "a+");
			if (cs->logfd == NULL)
				fprintf(stderr, "Error opening logfile %s\n", cs->logfile);
		} else if (!strcmp(tmp, "modesfile")) {
			sprintf(cs->modesfile, "%s", tmp2);
		} else if (!strcmp(tmp, "resfile")) {
			sprintf(cs->resfile[cs->resfilecount], "%s", tmp2);
			cs->resfilecount++;
		} else if (!strcmp(tmp, "emulator")) {
			sprintf(cs->emulator, "%s", tmp2);
		} else if (!strcmp(tmp, "vsync")) {
			cs->vsync = atoi(tmp2);
		} else if (!strcmp(tmp, "aspect")) {
			sprintf(cs->aspect, "%s", tmp2);
		} else if (!strcmp(tmp, "mo")) {
			cs->morientation = atoi(tmp2);
		} else if (!strcmp(tmp, "dcalign")) {
			cs->dcalign = atoi(tmp2);
		} else if (!strcmp(tmp, "crt_range0")) {
			sprintf(cs->crt_range[0], "%s", tmp2);
		} else if (!strcmp(tmp, "crt_range1")) {
			sprintf(cs->crt_range[1], "%s", tmp2);
		} else if (!strcmp(tmp, "crt_range2")) {
			sprintf(cs->crt_range[2], "%s", tmp2);
		} else if (!strcmp(tmp, "crt_range3")) {
			sprintf(cs->crt_range[3], "%s", tmp2);
		} else if (!strcmp(tmp, "crt_range4")) {
			sprintf(cs->crt_range[4], "%s", tmp2);
		} else if (!strcmp(tmp, "crt_range5")) {
			sprintf(cs->crt_range[5], "%s", tmp2);
		} else if (!strcmp(tmp, "crt_range6")) {
			sprintf(cs->crt_range[6], "%s", tmp2);
		} else if (!strcmp(tmp, "crt_range7")) {
			sprintf(cs->crt_range[7], "%s", tmp2);
		} else if (!strcmp(tmp, "crt_range8")) {
			sprintf(cs->crt_range[8], "%s", tmp2);
		} else if (!strcmp(tmp, "crt_range9")) {
			sprintf(cs->crt_range[9], "%s", tmp2);
		} else if (!strcmp(tmp, "lcd_range")) {
			sprintf(cs->lcd_range, "%s", tmp2);
		} else {
			sr_fprintf(stderr,  "%s: Config setting ignored: '%s', '%s' = '%s'\n",
				filename, line, tmp, tmp2);
		}
	}

	fclose(f);

	return 0;
}

int readIni(ConfigSettings *cs, GameInfo *game, char *filename) {
	FILE *f = NULL;
	char line[1000], tmp[1000], tmp2[1000];
	struct stat sb;

	if (!filename)
		return 1;
	
	if (stat(filename, &sb) == -1)
		return -1;
	else if (cs->verbose)
		sr_fprintf(stderr, "Opening %s file\n", filename);

	f = fopen(filename, "r");
	if (f == NULL)
		return -EBADF;
		
	while(!feof(f)) {
		int e= fscanf(f, "%999[^\n]\n", line) - 1;
		if(line[0] == '#' && !e)
			continue;
		e|= sscanf(line, "%999[^ ] %999[^\n]\n", tmp, tmp2) - 2;

		if (e)
			continue;

		if (!strcmp(tmp, "resolution")) {
			sr_fprintf(stderr, "INI file resolution %s\n", tmp2);	
			e|= sscanf(tmp2, "%dx%dx%d@%lf\n", 
				&game->width, &game->height, &game->depth, &game->refresh);
			if (e) {
				e|= sscanf(tmp2, "%dx%d@%lf", 
					&game->width, &game->height, &game->refresh);
				game->width = Normalize(game->width, 8);
				game->height = Normalize(game->height, 8);
			}
		} else if (!strcmp(tmp, "resolution0")) {
			sr_fprintf(stderr, "INI file resolution0 %s\n", tmp2);	
			e|= sscanf(tmp2, "%dx%dx%d@%lf", 
				&game->width, &game->height, &game->depth, &game->refresh);
			if (e) {
				e|= sscanf(tmp2, "%dx%d@%lf", 
					&game->width, &game->height, &game->refresh);
				game->width = Normalize(game->width, 8);
				game->height = Normalize(game->height, 8);
			}
		}
	}

	fclose(f);

	return 0;
}

int readResolutions(ConfigSettings *cs, ModeLine *mode, char *filename, ModeLine *bestmode) {
        FILE *f = NULL;
        char line[1000], tmp[1000], tmp2[1000], tmp3[1000], tmp4[1000];
        struct stat sb;
	ModeLine modes[255];
	int modecount = 0;
	unsigned int i;

        if (!filename)
                return 1;

        if (stat(filename, &sb) == -1)
                return -1;
        else if (cs->verbose)
                sr_fprintf(stderr, "Opening %s file\n", filename);

        f = fopen(filename, "r");
        if (f == NULL)
                return -EBADF;

	/* Get last best mode if exists */
	if (bestmode->hactive && bestmode->vactive && bestmode->vfreq) {
		modes[modecount].hactive = bestmode->hactive;
		modes[modecount].vactive = bestmode->vactive;
		modes[modecount].vfreq = bestmode->vfreq;
		modes[modecount].a_vfreq = bestmode->vfreq;
		modecount++;
	}

        while(!feof(f)) {
		int count = 4;
                int e= fscanf(f, "%999[^\n]\n", line) - 1;
                if(line[0] == '#' && !e)
                        continue;

        	/* convert commas to periods */
        	for (i = 0; i < strlen(line); i++) {
			if (line[i] == ',')
                		line[i] = '.';
		}

                e|= sscanf(line, "%999[^ ] %999[^ ] %999[^ ] %999[^\n]\n", 
			tmp, tmp2, tmp3, tmp4) - 4;

		if (e) {
                	e= sscanf(line, "%999[^ ] %999[^ ] %999[^\n]\n", 
				tmp, tmp2, tmp3) - 3;
			count = 3;
		}
		
		if (e) {
                	e= sscanf(line, "%999[^ ] %999[^\n]\n", 
				tmp, tmp2) - 2;
			count = 2;
		}

                if (e)
                        continue;

                if (!strcasecmp(tmp, "modeline")) {
			// '240x240-15,750KHz-59,885Hz'
                        e|= sscanf(tmp2, "'%dx%d-%999[^-]-%lfHz'", 
				&modes[modecount].hactive, &modes[modecount].vactive, 
				tmp4, &modes[modecount].vfreq) - 4;
                        if (e) {
                        	e= sscanf(tmp2, "'%dx%d-%999[^-]-%lfiHz'", 
					&modes[modecount].hactive, &modes[modecount].vactive, 
					tmp4, &modes[modecount].vfreq) - 4;
                        }
			if (e) {
                              	e= sscanf(tmp2, "\"%dx%d@%lf\"",
					&modes[modecount].hactive, &modes[modecount].vactive, 
					&modes[modecount].vfreq) - 3;
			}
			if (e) {
                              	e= sscanf(tmp2, "\"%dx%dx%lf\"",
					&modes[modecount].hactive, &modes[modecount].vactive, 
					&modes[modecount].vfreq) - 3;
			}
			if (!e) {
                        	sr_fprintf(stderr, "(%d) Modes file entry '%dx%dx%.2f'\n", 
					(modecount+1), modes[modecount].hactive, 
					modes[modecount].vactive, modes[modecount].vfreq);
				modecount++;
			}
                } else {
                        e|= sscanf(tmp, "%d", &modes[modecount].hactive) - 1;
                        e|= sscanf(tmp2, "%d", &modes[modecount].vactive) - 1;
			if (count == 4)
                        	e|= sscanf(tmp3, "%lfHz", &modes[modecount].vfreq) - 1;
			else if (count == 3)
                        	e|= sscanf(tmp3, "%lfHz\n", &modes[modecount].vfreq) - 1;
			else if (count == 2)
				modes[modecount].vfreq = 60.00;

			if (!e) {
                        	sr_fprintf(stderr, "(%d) ArcadeVGA Modes file entry '%dx%dx%.2f'\n", 
					(modecount+1), modes[modecount].hactive, 
					modes[modecount].vactive, modes[modecount].vfreq);
				modecount++;
			}

                }
        }

        fclose(f);

	sr_fprintf(stderr, "Got %d valid modes\n", modecount);

	findBestMode(1, mode, modes, bestmode, modecount);

	if (bestmode->hactive && bestmode->vactive && bestmode->vfreq) {	
		if (IS_WIN) {
			sprintf(bestmode->name, "%dx%d@%d", 
				bestmode->hactive, bestmode->vactive, (int)round(bestmode->vfreq));
		} else {
			sprintf(bestmode->name, "%dx%dx%d@%.2f", 
				bestmode->hactive, bestmode->vactive, bestmode->depth, bestmode->vfreq);
		}
             	sr_fprintf(stderr, "Best Mode is '%s'\n", 
			bestmode->name);

		bestmode->a_vfreq = bestmode->vfreq;
	}

        return 0;
}

#ifdef __CYGWIN__
int readSoft15KhzResolutions(ConfigSettings *cs, ModeLine *mode, ModeLine *bestMode) {
        //ModeLine AvailableVideoMode[MAX_MODELINES];
        ModeLine CustomVideoMode[MAX_MODELINES];
        //ModeLine DesktopMode;
        int AvailableModeCount = 0, CustomModeCount = 0;
        int i;

        //AvailableModeCount = GetAvailableVideoModes(AvailableVideoMode, &DesktopMode);
        CustomModeCount = GetCustomVideoModes(cs, CustomVideoMode, mode);

	findBestMode(0, mode, CustomVideoMode, bestMode, CustomModeCount);
	//findBestMode(0, mode, AvailableVideoMode, bestMode, AvailableModeCount);

	if (cs->verbose < 2)
		return (CustomModeCount+AvailableModeCount);

        /*for (i = 0; i < AvailableModeCount; i++) {
                sr_fprintf(stderr, "Available Mode %s%dx%d@%d %s\n",
                        (AvailableVideoMode[i].desktop)?"Desktop: ":"",
                        AvailableVideoMode[i].hactive, AvailableVideoMode[i].vactive,
                        (int)AvailableVideoMode[i].vfreq, (AvailableVideoMode[i].interlace)? "interlaced":"");
        }*/

        for (i = 0; i < CustomModeCount; i++) {
                sr_fprintf(stderr, "Custom Mode %s%dx%d@%d %s\n",
                        (CustomVideoMode[i].desktop)?"Desktop: ":"",
                        CustomVideoMode[i].hactive, CustomVideoMode[i].vactive,
                        (int)CustomVideoMode[i].a_vfreq, (CustomVideoMode[i].interlace)? "interlaced":"");
        }

	return (CustomModeCount+AvailableModeCount);
}
#endif

int findBestMode(int checkvfreq, ModeLine *mode, ModeLine modes[MAX_MODELINES], ModeLine *bestmode, int modecount) {
	int i;
	int index = -1;
	int hactive, vactive, vfreq;

	for (i = 0; i < modecount; i++) {
        	if (sscanf(modes[i].label, "DALDTMCRTBCD%dx%dx0x%d", &hactive, &vactive, &vfreq) != 3) {
			hactive = modes[i].hactive;
			vactive = modes[i].vactive;
			vfreq = modes[i].a_vfreq;
        	}

		if ((vactive <= mode->vactive && 
			bestmode->vactive <= Normalize(vactive,8)) &&
			(hactive <= (mode->hactive+16) &&
				bestmode->hactive <= (Normalize(hactive, 8)+16)))
		{
			if (checkvfreq == 0 || (vfreq <= (mode->vfreq+0.20))) {
				index = i;
				memcpy((ModeLine*)bestmode, (ModeLine*)&modes[i], sizeof(ModeLine));
				if (mode->vactive == modes[i].vactive &&
					mode->hactive == modes[i].hactive)
				{
					if (checkvfreq == 0 || (vfreq <= (mode->vfreq+0.20)))
						break;
				}
			}
		} else if (index == -1 && (mode->hactive <= hactive && mode->vactive <= vactive)) {
			index = i;
			memcpy((ModeLine*)bestmode, (ModeLine*)&modes[i], sizeof(ModeLine));
		}
	}

	return index;
}

int GetMameInfo(ConfigSettings *cs, char *mamearg, char *result) {
        int c1=0;
        FILE *pi;
        char temp[100000]={'\x00'};
        char cmd[255];
	int ret = 0;

        /* Windows needs a kludge I guess */
        if (IS_WIN) {
                char* argv[25];
                FILE *fp = NULL;
                char file[255];
                pid_t cpid;

                sprintf(file, "mameusage.txt");

                sprintf(cmd, "%s", cs->emulator);
                argv[0] = cs->emulator;
                argv[1] = mamearg;
                argv[2] = NULL;

                /* Fork and run mame, wait for pid to exit */
                cpid = fork();
                switch (cpid) {
                        case -1: printf("Failed running %s\n", cs->emulator);
			ret = 1;
                        break;
                case 0: fp = freopen(file, "w+", stdout);
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
                        sr_fprintf(stderr, "%s exited or failed running\n", cs->emulator);
			ret = 1;
			exit(1);
                default:
                        waitpid(cpid, NULL, 0);
                        break;
                }
		fp = fopen(file, "r");
                if (fp != NULL) {
                	int i = 0;
        		char c;
                	c=fgetc(fp);
                	while(c != '\xff' && i < 100000) {
                        	temp[c1]=c;
                        	c=fgetc(fp);
                        	c1++;
                	}
                	temp[c1]='\x00';

                        fclose(fp);
                	unlink(file);

			sprintf(result, "%s", temp);
		} else {
			sr_fprintf(stderr, "Error opening output file %s\n", file);
			ret = 1;
		}

                return ret;
        } else
                sprintf(cmd, "%s %s", cs->emulator, mamearg);

        pi=popen(cmd, "r");
        if(pi != NULL) {
                int i = 0;
        	char c;
                c=fgetc(pi);
                while(c != '\xff' && i < 100000) {
                        temp[c1]=c;
                        c=fgetc(pi);
                        c1++;
                }
                temp[c1]='\x00';
                pclose(pi);

		sprintf(result, "%s", temp);
        } else {
		sr_fprintf(stderr, "Error opening output pipe with cmd %s\n", cmd);
		ret = 1;
	}

	return ret;
}
