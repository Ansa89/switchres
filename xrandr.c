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

#include "switchres.h"

int GetXrandrDisplay(ConfigSettings *cs, ModeLine *defaultMode) {
        char cmd[64*1024]={'\x00'};
        char *cmdptr = cmd;

        /* Get Xrandr display information */
        sprintf(cmd, "xrandr --current | grep ' connected' | cut -d' ' -f1");
        RunProgram(cs, cmdptr);
        sscanf(cmd, "%s", (char *)&defaultMode->label);

        /* Get Xrandr default resolution information */
        sprintf(cmd, "xrandr --current | grep '*'");
        RunProgram(cs, cmdptr);
	defaultMode->vfreq = 0;
        if (sscanf(cmd, "   %dx%dx%lf   %lf*",
                &defaultMode->hactive, &defaultMode->vactive, &defaultMode->vfreq,
                &defaultMode->a_vfreq) != 4 || !defaultMode->vfreq) 
	{
        	sscanf(cmd, "   %dx%d   %lf*",
                	&defaultMode->hactive, &defaultMode->vactive, &defaultMode->a_vfreq);
        	sprintf(defaultMode->name, "%dx%d",
                	defaultMode->hactive, defaultMode->vactive);
		defaultMode->vfreq = defaultMode->a_vfreq;
	} else {
        	sprintf(defaultMode->name, "%dx%dx%.2f",
                	defaultMode->hactive, defaultMode->vactive, defaultMode->vfreq);
	}

        if (cs->verbose)
                sr_fprintf(stderr, "Got display %s '%s' with %dx%d@%.2f resolution\n",
                        defaultMode->label, defaultMode->name,
                        defaultMode->hactive, defaultMode->vactive, defaultMode->vfreq);
	return 0;
}

int SetXrandrDisplay(ConfigSettings *cs, MonitorMode *monitorMode, ModeLine *defaultMode) {
        char cmd[64*1024]={'\x00'};
        char modeline[255]={'\x00'};
        char *cmdptr = cmd;

        /* Add new modeline */
        sprintf(cmd, "xrandr --nograb --newmode %s",
                PrintModeline(monitorMode->ModeLine, modeline));
        RunProgram(cs, cmdptr);

        /* Add modeline to interface */
        sprintf(cmd, "xrandr --nograb --addmode %s \"%s\"",
                defaultMode->label, monitorMode->ModeLine->name);
        RunProgram(cs, cmdptr);

        if (cs->xrandr) {
                /* Switch to mode */
                sprintf(cmd, "xrandr --nograb --output %s --mode \"%s\"",
                        defaultMode->label, monitorMode->ModeLine->name);
                RunProgram(cs, cmdptr);
        }
	return 0;
}

//int DelXrandrDisplay(ConfigSettings *cs, MonitorMode *monitorMode, ModeLine *defaultMode, int cpid) {
int DelXrandrDisplay(ConfigSettings *cs, MonitorMode *monitorMode, ModeLine *defaultMode) {
        char cmd[64*1024]={'\x00'};
        char *cmdptr = cmd;

        //if (cs->xrandr || cpid == -1) {
                /* Switch to mode */
                sprintf(cmd, "xrandr --nograb --output %s --mode \"%s\"",
                        defaultMode->label, defaultMode->name);
                RunProgram(cs, cmdptr);
        //}

        /* Delete  modeline from interface */
        sprintf(cmd, "xrandr --nograb --delmode %s \"%s\"",
                defaultMode->label, monitorMode->ModeLine->name);
        RunProgram(cs, cmdptr);

        /* Print out modeline information */
        if (cs->verbose > 1) {
                sprintf(cmd, "xrandr --nograb --current");
                RunProgram(cs, cmdptr);
                sr_fprintf(stderr, "%s", cmd);
        }

        /* remove modeline */
        sprintf(cmd, "xrandr --nograb --rmmode \"%s\"",
                monitorMode->ModeLine->name);
        RunProgram(cs, cmdptr);

	return 0;
}
