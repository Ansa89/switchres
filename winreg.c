#ifdef __CYGWIN__

#include <stdio.h>
#include <windows.h>
#include <wingdi.h>
#include <winuser.h>
#include <winnt.h>

#include <ctype.h>
#include <string.h>

#include "switchres.h"

void swap(int *a, int *b);

void swap(int* a, int* b)
{
    int temp = *a;
    *a = *b;
    *b = temp;
}

DWORD RealRes( DWORD x) {
        return (int) (x / 8) * 8;
}

int GetAvailableVideoModes(ModeLine VideoMode[MAX_MODELINES], ModeLine *DesktopMode) {
        int iModeNum = 0, k = 0;
        int i, j;
        HDC hDC;
        DEVMODE lpDevMode;
        LPCTSTR lpDriverName, lpDeviceName;
        int DesktopBPP;

        lpDriverName = "DISPLAY";

        hDC = CreateIC ( lpDriverName, NULL, NULL, NULL );
        DesktopMode->hactive = GetDeviceCaps ( hDC, HORZRES );
        DesktopMode->vactive = GetDeviceCaps ( hDC, VERTRES );
        DesktopBPP = GetDeviceCaps ( hDC, BITSPIXEL );
        DesktopMode->vfreq = GetDeviceCaps ( hDC, VREFRESH );

        lpDevMode.dmSize = sizeof(DEVMODE);
        lpDeviceName = "\\\\.\\Display1";

        while (EnumDisplaySettings( NULL, iModeNum, &lpDevMode ) != 0) {
                if (lpDevMode.dmBitsPerPel == 32) {
                        VideoMode[k].hactive = lpDevMode.dmPelsWidth;
                        VideoMode[k].vactive = lpDevMode.dmPelsHeight;
                        VideoMode[k].vfreq = lpDevMode.dmDisplayFrequency;
                        VideoMode[k].interlace =
                                (lpDevMode.dmDisplayFlags & DM_INTERLACED)?1:0;
                        VideoMode[k].desktop = 0;

                        if (VideoMode[k].hactive == DesktopMode->hactive &&
                                VideoMode[k].vactive == DesktopMode->vactive &&
                                        VideoMode[k].vfreq == DesktopMode->vfreq)
                        {
                                VideoMode[k].desktop = 1;
                        }

                        k++;
                }
                iModeNum++;
        }

        for (i = 0; i < k; i++) {
                for (j = i; j < k; j++) {
                        if (RealRes ( VideoMode[j].hactive ) <
                                RealRes ( VideoMode[i].hactive ) ||
                        ( RealRes ( VideoMode[j].hactive ) == RealRes (
                                VideoMode[i].hactive ) &&
                                        VideoMode[j].vactive <
                                                VideoMode[i].vactive ) ||
                        ( RealRes ( VideoMode[j].hactive ) == RealRes (
                                VideoMode[i].hactive ) &&
                                        VideoMode[j].vactive ==
                                                VideoMode[i].vactive &&
                                 VideoMode[j].vfreq < VideoMode[i].vfreq ) ||
                        ( RealRes ( VideoMode[j].hactive ) == RealRes (
                                        VideoMode[i].hactive ) &&
                                VideoMode[j].vactive == VideoMode[i].vactive &&
                                VideoMode[j].vfreq == VideoMode[i].vfreq &&
                                        VideoMode[j].hactive < VideoMode[i].hactive ))
                        {
                                swap(&VideoMode[j].hactive, &VideoMode[i].hactive);
                                swap(&VideoMode[j].vactive, &VideoMode[i].vactive);
                                swap((int*)&VideoMode[j].vfreq, (int*)&VideoMode[i].vfreq);
                        }
                }
        }

        DeleteDC(hDC);

        return k;
}

int CustomModeDataWord(int i, char *lpData) {
	char out[32] = "";
	int x;

	sprintf(out, "%02X%02X", lpData[i]&0xFF, lpData[i+1]&0xFF);
	sscanf(out, "%d", &x);
	return x;
}

int CustomModeDataWordBCD(int i, char *lpData) {
	char out[32] = "";
	int x;

	sprintf(out, "%02X%02X", lpData[i]&0xFF, lpData[i+1]&0xFF);
	sscanf(out, "%04X", &x);
	return x;
}

void SetCustomModeDataWord (char *DataString, long DataWord, int offset) {
	int DataLow, DataHigh;

	if (DataWord < 65536) {
		DataLow = DataWord % 256;
		DataHigh = DataWord / 256;
		DataString[offset] = DataHigh;
		DataString[offset+1] = DataLow;
	}
}

void SetCustomModeDataWordBCD (char *DataString, long DataWord, int offset) {
	if (DataWord < 10000) {
		int DataLow, DataHigh;
		int a, b;
		char out[32] = "";

		DataLow = DataWord % 100;
		DataHigh = DataWord / 100;

		sprintf(out, "%d %d", DataHigh, DataLow);
		sscanf(out, "%02X %02X", &a, &b);

		DataString[offset] = a;
		DataString[offset+1] = b;
	}
}

int SetCustomVideoModes(ConfigSettings *cs, ModeLine *VideoMode, ModeLine *mode) {
	HKEY hKey;
	LONG lRes;
	char dv[255];
	DWORD dvlen;
	char DefaultVideo[255];
	int i = 0;
	DWORD type;
	char lpValueName[1024];
	char lpData[1024];
	DWORD lpcValueName, lpcData;
	int hhh = 0, hhi = 0, hhf = 0, hht = 0, vvv = 0, vvi = 0, vvf = 0, vvt = 0, interlace = 0;
	double dotclock = 0;
	long checksum;
	int old_checksum;

	if (!strcmp(VideoMode->label, ""))
		return -1;

        if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "HARDWARE\\DEVICEMAP\\VIDEO", 0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS) {
                int j = 0;
                dvlen = sizeof(DefaultVideo);

                lRes = RegQueryValueEx(hKey, "\\Device\\Video0", NULL, NULL, (LPBYTE)&DefaultVideo, &dvlen);
                RegCloseKey(hKey);
                for(i = 18, j = 0;i < strlen(DefaultVideo); i++)
                        dv[j++] = DefaultVideo[i];
                dv[j] = '\0';
	} else {
                sr_fprintf(stderr, "Failed opening DefaultVideo registry\n");
                return -1;
	}

	memcpy(lpData, VideoMode->regdata, VideoMode->regdata_size);

	/*for(i=0; i < VideoMode->regdata_size; i++) {
		sr_fprintf(stderr, "[%02X]", lpData[i]);
	}
	sr_fprintf(stderr, "\n\n");*/

	SetCustomModeDataWordBCD (lpData, (int)mode->pclock/10000, 38);
	SetCustomModeDataWordBCD (lpData, mode->hactive, 10);
	SetCustomModeDataWordBCD (lpData, mode->hbegin, 14);
	SetCustomModeDataWordBCD (lpData, mode->hend - mode->hbegin, 18);
	SetCustomModeDataWordBCD (lpData, mode->htotal, 6);
	SetCustomModeDataWordBCD (lpData, mode->vactive, 26);
	SetCustomModeDataWordBCD (lpData, mode->vbegin, 30);
	SetCustomModeDataWordBCD (lpData, mode->vend - mode->vbegin, 34);
	SetCustomModeDataWordBCD (lpData, mode->vtotal, 22);

        if (mode->interlace)
		lpData[3] = 0x0e;
	else
		lpData[3] = 0x0c;

	dotclock = (double)CustomModeDataWord(38, lpData);
	hhh = CustomModeDataWord(10, lpData);
	hhi = CustomModeDataWord(14, lpData);
	hhf = CustomModeDataWord(18, lpData) + hhi;
	hht = CustomModeDataWord(6, lpData);
	vvv = CustomModeDataWord(26, lpData);
	vvi = CustomModeDataWord(30, lpData);
	vvf = CustomModeDataWord(34, lpData) + vvi;
	vvt = CustomModeDataWord(22, lpData);
	interlace = (lpData[3] == 0x0e)?1:0;

	old_checksum = CustomModeDataWordBCD(66, lpData);

	checksum = 65535 - ((lpData[3] == 0x0e)?0x0e:0x0c) - hht - hhh - hhf - vvt - vvv - vvf - dotclock;

	SetCustomModeDataWord (lpData, checksum, 66);

	/*for(i=0; i < VideoMode->regdata_size; i++) {
		sr_fprintf(stderr, "[%02X]", lpData[i]);
	}
	sr_fprintf(stderr, "\n\n");*/

	if (cs->verbose) { 
		sr_fprintf(stderr, "Setting registery video mode %s with:\n", VideoMode->label);
		sr_fprintf(stderr, "(%d/%d/%ld) Modeline %.6f %d %d %d %d %d %d %d %d%s\n",
			CustomModeDataWordBCD(66, lpData), old_checksum, checksum,
			(double)((double)dotclock * 10000.0)/1000000.0, 
			(int)RealRes (hhh), (int)RealRes (hhi), (int)RealRes (hhf), (int)RealRes (hht), 
			vvv, vvi, vvf, vvt, (interlace)?" interlace":"");
	}

        if ((lRes = RegOpenKeyEx(HKEY_LOCAL_MACHINE, dv, 0, KEY_ALL_ACCESS, &hKey)) == ERROR_SUCCESS) {
                lpcValueName = sizeof( lpValueName );
                lpcData = sizeof( lpData );
		type = REG_BINARY;

		// Write registry entry here	
		RegSetValueEx(hKey, VideoMode->label, 0, type, (LPBYTE)lpData, VideoMode->regdata_size);

                RegCloseKey(hKey);
        } else {
                sr_fprintf(stderr, "Failed opening %s registry entry with error %d\n", dv, (int)lRes);
        }

	return 0;
}

int GetCustomVideoModes(ConfigSettings *cs, ModeLine VideoMode[MAX_MODELINES], ModeLine *mode) {
        HKEY hKey;
        int dwIndex = 0, i = 0, k = 0;
        int  hactive, vactive, vfreq;
        char lpValueName[1024];
        char lpData[1024];
        DWORD lpcValueName, lpcData;
        char DefaultVideo[255];
        LONG lRes;
        char dv[255];
        DWORD dvlen;
	DWORD type;

        if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, "HARDWARE\\DEVICEMAP\\VIDEO", 0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS) {
                int j = 0;
                dvlen = sizeof(DefaultVideo);

                lRes = RegQueryValueEx(hKey, "\\Device\\Video0", NULL, NULL, (LPBYTE)&DefaultVideo, &dvlen);
                RegCloseKey(hKey);
                for(i = 18, j = 0;i < strlen(DefaultVideo); i++)
                        dv[j++] = DefaultVideo[i];
                dv[j] = '\0';
        } else {
                sr_fprintf(stderr, "Failed opening DefaultVideo registry\n");
                return -1;
        }

	if (cs->verbose)
        	sr_fprintf(stderr, "DefaultVideo '%s'\n", dv);

        if ((lRes = RegOpenKeyEx(HKEY_LOCAL_MACHINE, dv, 0, KEY_ALL_ACCESS, &hKey)) == ERROR_SUCCESS) {
                lpcValueName = sizeof( lpValueName );
                lpcData = sizeof( lpData );
		type = 0;

                while (RegEnumValue (hKey, dwIndex, lpValueName, &lpcValueName, NULL, &type, (LPBYTE)&lpData, &lpcData) == ERROR_SUCCESS) {
                        dwIndex++;

                        if (strstr(lpValueName, "DALDTMCRTBCD")) {
				int hhh = 0, hhi = 0, hhf = 0, hht = 0, vvv = 0, vvi = 0, vvf = 0, vvt = 0, interlace = 0;
				double dotclock = 0;
				int checksum;
				if (cs->verbose)
                                	sr_fprintf(stderr, "%s:\n ", lpValueName);

				dotclock = (double)CustomModeDataWord(38, lpData);
				hhh = (int)RealRes(CustomModeDataWord(10, lpData));
				hhi = (int)RealRes(CustomModeDataWord(14, lpData));
				hhf = (int)RealRes(CustomModeDataWord(18, lpData)) + hhi;
				hht = (int)RealRes(CustomModeDataWord(6, lpData));
				vvv = CustomModeDataWord(26, lpData);
				vvi = CustomModeDataWord(30, lpData);
				vvf = CustomModeDataWord(34, lpData) + vvi;
				vvt = CustomModeDataWord(22, lpData);
				interlace = (lpData[3] == 0x0e)?1:0;

				checksum = CustomModeDataWordBCD(66, lpData);

				if (cs->verbose)
					sr_fprintf(stderr, " (%d/%d) Modeline %.6f %d %d %d %d %d %d %d %d%s\n",
						checksum, (int)lpcData, (double)((double)dotclock * 10000.0)/1000000.0, 
						(int)RealRes (hhh), (int)RealRes (hhi), (int)RealRes (hhf), (int)RealRes (hht), 
						vvv, vvi, vvf, vvt, (interlace)?" interlace":"");

                                if (sscanf(lpValueName, "DALDTMCRTBCD%dx%dx0x%d", &hactive, &vactive, &vfreq) != 3) {
                                	if (sscanf(lpValueName, "DALDTMCRTBCD%dX%dX0X%d", &hactive, &vactive, &vfreq) != 3) {
                                        	sr_fprintf(stderr, "Failed getting resolution values from %s\n", lpValueName);
                                        	continue;
					}
                                }

				if (cs->version > 104 || cs->version <= 0)
					sprintf(VideoMode[k].name, "%dx%d@%d", hactive, vactive, vfreq);
				else
					sprintf(VideoMode[k].name, "%dx%d", hactive, vactive);

				VideoMode[k].a_width = hactive;
				VideoMode[k].a_height = vactive;

				sprintf(VideoMode[k].resolution, "%dx%d@%d", hactive, vactive, vfreq);
				VideoMode[k].vfreq = vfreq;
				VideoMode[k].a_vfreq = (double)(dotclock * 10000.0) / (vvt * hht);
                                VideoMode[k].pclock  = dotclock * 10000;
                                VideoMode[k].hactive = hhh;
                                VideoMode[k].hbegin  = hhi;
                                VideoMode[k].hend    = hhf;
                                VideoMode[k].htotal  = hht;
                                VideoMode[k].vactive = vvv;
                                VideoMode[k].vbegin  = vvi;
                                VideoMode[k].vend    = vvf;
                                VideoMode[k].vtotal  = vvt;
                                VideoMode[k].interlace  = interlace;
                                VideoMode[k].doublescan  = 0;
                                VideoMode[k].custom  = 1;

				snprintf(VideoMode[k].label, lpcValueName+1, "%s", lpValueName);
				memcpy(VideoMode[k].regdata, lpData, lpcData+1);
				VideoMode[k].regdata_size = lpcData;
                                k++;
                        }

                        lpcValueName = sizeof( lpValueName );
                        lpcData = sizeof( lpData );
                }

                RegCloseKey(hKey);
        } else {
                sr_fprintf(stderr, "Failed opening %s registry entry with error %d\n", dv, (int)lRes);
        }

        return k;
}

#endif
