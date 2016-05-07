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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#ifdef SYS_MACOSX
#include <sys/wait.h>
#else
#include <wait.h>
#endif
#include "libxml/xmlmemory.h"
#include "libxml/parser.h"
#include "switchres.h"

int ParseXML(xmlDocPtr xmlbuffer, GameInfo *game, ConfigSettings *cs);

int GetGameXML(ConfigSettings *cs, GameInfo *game, char *emulator) {
	FILE *pi;
	char temp[64*1024]={'\x00'};
	char c;
	char *ptr;
	xmlDocPtr xmlbuffer = NULL;
	char cmd[255];

	if (!strcmp(game->name, ""))
		return 1;

	/* Windows needs a kludge I guess */
	if (IS_WIN) {
		char* argv[25];
		FILE *fp = NULL;
		char file[255];
        	pid_t cpid;

		sprintf(file, "%s.xml", game->name);
		
		//system(cmd);
		sprintf(cmd, "%s", emulator);
		argv[0] = emulator;
		argv[1] = "-listxml";
		argv[2] = game->name;
		argv[3] = NULL;

        	/* Fork and run mame, wait for pid to exit */
        	cpid = fork();
        	switch (cpid) {
                	case -1: printf("Failed running %s\n", emulator);
                	break;
        	case 0:	fp = freopen(file, "w", stdout);
                	execvp(emulator, argv);
			{
				char *path = NULL;
				char *envp = getenv("PATH");
				path = strtok(envp, ":");
				while(path != NULL) {
					char executable[255];
					sprintf(executable, "%s/%s", path, emulator);
                			execvp(executable, argv);
					if (cs->verbose > 2)
						sr_fprintf(stderr, "Failed running %s\n", executable);
					path = strtok(NULL, ":");
				}
			}
			sr_fprintf(stderr, "%s exited or failed running with PATH=%s\n", emulator, getenv("PATH"));
                	exit(1); // Failed
        	default:
                	waitpid(cpid, NULL, 0);
                	break;
        	}
		if (fp)
			fclose(fp);

		xmlbuffer = xmlParseFile(file);
		unlink(file);
		if (xmlbuffer == NULL)
			return -1;
		ParseXML(xmlbuffer, game, cs);
		xmlFreeDoc(xmlbuffer);
		if (game->width <= 0 || game->height <= 0 || game->refresh <= 0)
			return -1;
		return 0;
	} else
		sprintf(cmd, "%s -listxml %s", emulator, game->name);

	pi=popen(cmd, "r");
	if(pi != NULL) {
		int i = 0;
        	c=fgetc(pi);
        	while(c != '\xff' && i < 64*1024) {
                	temp[i]=c;
                	c=fgetc(pi);
                	i++;
                }
        	temp[i]='\x00';
        	pclose(pi);

		if (cs->verbose > 3)
			sr_fprintf(stderr, "Game XML is: %s\n", temp);

		/* Turn into an XML buffer */
		ptr = (char *) &temp;
		xmlbuffer = xmlParseDoc(xmlCharStrdup(ptr));
		if (xmlbuffer == NULL) {
			sr_fprintf(stderr, "Error getting XML info for %s from %s\n",
				emulator, game->name);
			return -1;
		}
		ParseXML(xmlbuffer, game, cs);

		xmlFreeDoc(xmlbuffer);
        }

	if (game->width <= 0 || game->height <= 0 || game->refresh <= 0) {
		if (cs->verbose > 2)
			sr_fprintf(stderr, "Didn't get width/height/refresh from mame\n");
		return -1;
	}

	return 0;
}

int ParseXML(xmlDocPtr xmlbuffer, GameInfo *game, ConfigSettings *cs) {
	xmlNode *root = NULL;
	xmlNode *node = NULL;
	xmlNode *child = NULL;
	int count;

	root = xmlDocGetRootElement(xmlbuffer);	
	if (root == NULL) {
		sr_fprintf(stderr, "Empty xml buffer\n");
		return -1;
	}

	for(node = root->children; node != NULL; node = node->next) {
		if (((node->type == XML_ELEMENT_NODE)  &&
		    ((xmlStrcmp(node->name, (const xmlChar *) "game") == 0) ||
		     (xmlStrcmp(node->name, (const xmlChar *) "machine") == 0))))
		{
			char *type = NULL;
			char *width = NULL;
			char *height = NULL;
			char *refresh = NULL;
			char *orientation = NULL;

			count++;
			for(child = node->children; child != NULL; child = child->next) {
				if ((node->type == XML_ELEMENT_NODE)  &&
				    (xmlStrcmp(child->name, (const xmlChar *) "display") == 0))
				{
					type = (char *)xmlGetProp(child, (const xmlChar *)"type");
					orientation = (char *)xmlGetProp(child, (const xmlChar *)"rotate");
					width = (char *)xmlGetProp(child, (const xmlChar *)"width");
					height = (char *)xmlGetProp(child, (const xmlChar *)"height");
					refresh = (char *)xmlGetProp(child, (const xmlChar *)"refresh");

					game->screens++;

					/* type */
					if (type != NULL && !strcmp(type, "vector"))
						game->vector = 1;
					else
						game->vector = 0;

					/* Width */
					if (width != NULL) {
						if (sscanf(width, "%d", &game->width) != 1)
							game->width = cs->vectorwidth;
					} else
						game->width = cs->vectorwidth;

					/* Height */
					if (height != NULL) {
						if (sscanf(height, "%d", &game->height) != 1)
							game->height = cs->vectorheight;
					} else
						game->height = cs->vectorheight;

					/* Refresh */
					if (refresh != NULL) {
						if (sscanf(refresh, "%lf", &game->refresh) != 1)
							game->refresh = 60.00;
					} else
						game->refresh = 60.00;

					if (cs->froggerfix) {
						if (game->width == 224 && game->height == 768)
							game->height = 256;
						else if (game->height == 224 && game->width == 768)
							game->width = 256;
					}

					game->o_width = game->width;
					game->o_height = game->height;
					game->o_refresh = game->refresh;
					game->o_aspect = (double)game->width / (double)game->height;

					/* Orientation */
					if (orientation != NULL && !game->vector) {
       						if ((strcmp(orientation, "90") == 0) ||
               						(strcmp(orientation, "270") == 0))
       						{       // orientation = vertical
       							int w = game->width;
       							int h = game->height;
							game->orientation = 1;
							if (cs->morientation == 0) {
								game->width = h;
								game->height = w;
							}
       						} else { // horizontal
							game->orientation = 0;
							if (cs->morientation == 1) {
       								int w = game->width;
       								int h = game->height;
								game->width = h;
								game->height = w;
							}
						}
						if ((game->orientation && !cs->morientation) || (!game->orientation &&  (cs->morientation == 1))) {
							double num, den;
							sscanf(cs->aspect, "%lf:%lf", &den, &num);

							game->width = Normalize((double)game->width * (4.0/3.0) / (num/den), 8);
						}
					} else {
						game->orientation = 0;
					}

					game->aspect = (double)game->width / (double)game->height;

					/* Dual screen games */
					if (game->screens > 1) {
						game->height *= 2;
						game->width = game->aspect * (double)game->height;
					}

					game->width = Normalize(game->width, 8);
					game->height = Normalize(game->height, 8);
				}
			}
		}
	
	}
	return 0;
}
