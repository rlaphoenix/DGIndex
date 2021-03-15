/* 
 *  Mutated into DGIndex. Modifications Copyright (C) 2004-2008, Donald Graft
 * 
 *	Copyright (C) Chia-chen Kuo - April 2001
 *
 *  This file is part of DVD2AVI, a free MPEG-2 decoder
 *	
 *  DVD2AVI is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  DVD2AVI is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *
 */
#define _WIN32_WINNT 0x0501 // Needed for WM_MOUSEWHEEL

#include <windows.h>
#include "resource.h"
#include "Shlwapi.h"
#include <lm.h>

#define GLOBAL
#include "global.h"

static char Version[] = "DGIndex 2.0.0.5";

#define TRACK_HEIGHT	32
#define INIT_WIDTH		480
#define INIT_HEIGHT		270
#define MIN_WIDTH		160
#define MIN_HEIGHT		32

#define MASKCOLOR		RGB(0, 6, 0)

#define	SAVE_D2V		1
#define SAVE_WAV		2
#define	OPEN_D2V		3
#define OPEN_VOB		4
#define OPEN_WAV		5
#define SAVE_BMP		6
#define OPEN_AVS		7
#define OPEN_TXT		8

#define PRIORITY_HIGH		1
#define PRIORITY_NORMAL		2
#define PRIORITY_LOW		3

bool PopFileDlg(PTSTR, HWND, int);
ATOM MyRegisterClass(HINSTANCE);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK SelectProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK Info(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK VideoList(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK Cropping(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK Luminance(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK Speed(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK Normalization(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK SelectTracks(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK SelectDelayTrack(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK SetPids(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK AVSTemplate(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK BMPPath(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK DetectPids(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK About(HWND, UINT, WPARAM, LPARAM);
static void ShowInfo(bool);
static void SaveBMP(void);
static void CopyBMP(void);
static void OpenVideoFile(HWND);
static void OpenAudioFile(HWND);
void OutputProgress(int);

static void StartupEnables(void);
static void FileLoadedEnables(void);
static void RunningEnables(void);

static int INIT_X, INIT_Y, Priority_Flag, Edge_Width, Edge_Height;

static FILE *INIFile;
static char szPath[DG_MAX_PATH], szTemp[DG_MAX_PATH], szWindowClass[DG_MAX_PATH];

static HINSTANCE hInst;
static HANDLE hProcess, hThread;
static HWND hLeftButton, hLeftArrow, hRightArrow, hRightButton;

static DWORD threadId;
static RECT wrect, crect, info_wrect;
static int SoundDelay[MAX_FILE_NUMBER];
static char Outfilename[MAX_FILE_NUMBER][DG_MAX_PATH];

extern int fix_d2v(HWND hWnd, char *path, int test_only);
extern int parse_d2v(HWND hWnd, char *path);
extern int analyze_sync(HWND hWnd, char *path, int track);
extern unsigned char *Rdbfr;

bool GetWindowsVersion(DWORD& major, DWORD& minor)
{
	LPBYTE pinfoRawData;
	if (NERR_Success == NetWkstaGetInfo(NULL, 100, &pinfoRawData))
	{
		WKSTA_INFO_100 * pworkstationInfo = (WKSTA_INFO_100 *)pinfoRawData;
		major = pworkstationInfo->wki100_ver_major;
		minor = pworkstationInfo->wki100_ver_minor;
		::NetApiBufferFree(pinfoRawData);
		return true;
	}
	return false;
}

int MessageBoxTimeout(HWND hWnd, LPCSTR lpText, LPCSTR lpCaption, UINT uType, WORD wLanguageId, DWORD dwMilliseconds)
{
	typedef int(__stdcall* MSGBOXAAPI)(IN HWND hWnd, IN LPCSTR lpText, IN LPCSTR lpCaption, IN UINT uType, IN WORD wLanguageId, IN DWORD dwMilliseconds);
	static MSGBOXAAPI MsgBoxTOA = NULL;

	if (!MsgBoxTOA)
	{
		HMODULE hUser32 = GetModuleHandle("user32.dll");
		if (hUser32)
		{
			MsgBoxTOA = (MSGBOXAAPI)GetProcAddress(hUser32, "MessageBoxTimeoutA");
			//fall through to 'if (MsgBoxTOA)...'
		}
		else
		{
			//Stuff happened, add code to handle it here (possibly just call MessageBox())
			return 0;
		}
	}

	if (MsgBoxTOA)
	{
		return MsgBoxTOA(hWnd, lpText, lpCaption, uType, wLanguageId, dwMilliseconds);
	}

	return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	MSG msg;
	HACCEL hAccel;

	int i;
	char *ptr;
	char ucCmdLine[4096];
	char prog[DG_MAX_PATH];
	char cwd[DG_MAX_PATH];
	DWORD major, minor;

	if (GetWindowsVersion(major, minor))
	bIsWindowsXPorLater = (major > 5) || ((major == 5) && (minor >= 1));

    if(bIsWindowsXPorLater)
	{
		// Prepare status output (console/file).
		if (GetStdHandle(STD_OUTPUT_HANDLE) == (HANDLE)0)
		{
			// No output handle. We'll try to attach a console.
			AttachConsole( ATTACH_PARENT_PROCESS );
		}
		else
		{
			if (FlushFileBuffers(GetStdHandle(STD_OUTPUT_HANDLE)))
			{
				// Flush succeeded -> We are NOT writing to console (output redirected to file, etc.). No action required.
			}
			else
			{
				// Flush failed -> We are writing to console. AttachConsole to enable it.
				AttachConsole( ATTACH_PARENT_PROCESS );
			}
		}
	}

	// Get the path to the DGIndex executable.
	GetModuleFileName(NULL, ExePath, DG_MAX_PATH);

	// Find first char after last backslash.
    if ((ptr = strrchr(ExePath,'\\')) != 0) ptr++;
    else ptr = ExePath;
	*ptr = 0;

	// Load INI
	strcpy(prog, ExePath);
	strcat(prog, "DGIndex.ini");
	if ((INIFile = fopen(prog, "r")) == NULL)
	{
NEW_VERSION:
		INIT_X = INIT_Y = 100;
		info_wrect.left = info_wrect.top = 100;
		iDCT_Flag = IDCT_SKAL;
		Scale_Flag = true;
		setRGBValues();
		FO_Flag = FO_NONE;
		Method_Flag = AUDIO_DEMUXALL;
        strcpy(Track_List, "");
		DRC_Flag = DRC_NORMAL;
		DSDown_Flag = false;
		SRC_Flag = SRC_NONE;
		Norm_Ratio = 100;
		Priority_Flag = PRIORITY_NORMAL;
		PlaybackSpeed = SPEED_NORMAL;
		ForceOpenGops = 0;
		// Default the AVS template path.
		// Get the path to the DGIndex executable.
		GetModuleFileName(NULL, AVSTemplatePath, 255);
		// Find first char after last backslash.
		if ((ptr = strrchr(AVSTemplatePath,'\\')) != 0) ptr++;
		else ptr = AVSTemplatePath;
		*ptr = 0;
		strcat(AVSTemplatePath, "template.avs");
		FullPathInFiles = 1;
        LoopPlayback = 0;
		FusionAudio = 0;
        HDDisplay = HD_DISPLAY_SHRINK_BY_HALF;
        for (i = 0; i < 4; i++)
            mMRUList[i][0] = 0;
        InfoLog_Flag = 1;
        BMPPathString[0] = 0;
        UseMPAExtensions = 0;
        NotifyWhenDone = 0;
    }
	else
	{
		char line[DG_MAX_PATH], *p;
        unsigned int audio_id;

		fgets(line, DG_MAX_PATH - 1, INIFile);
		line[strlen(line)-1] = 0;
		p = line;
		while (*p != '=' && *p != 0) p++;
		if (*p == 0 || strcmp(++p, Version))
		{
			fclose(INIFile);
			goto NEW_VERSION;
		}

		fscanf(INIFile, "Window_Position=%d,%d\n", &INIT_X, &INIT_Y);
		fscanf(INIFile, "Info_Window_Position=%d,%d\n", &info_wrect.left, &info_wrect.top);

		fscanf(INIFile, "iDCT_Algorithm=%d\n", &iDCT_Flag);
		fscanf(INIFile, "YUVRGB_Scale=%d\n", &Scale_Flag);
		setRGBValues();
		fscanf(INIFile, "Field_Operation=%d\n", &FO_Flag);
		fscanf(INIFile, "Output_Method=%d\n", &Method_Flag);
		fgets(line, DG_MAX_PATH - 1, INIFile);
		line[strlen(line)-1] = 0;
		p = line;
		while (*p++ != '=');
		strcpy (Track_List, p);
        for (i = 0; i < 0xc8; i++)
            audio[i].selected_for_demux = false;
        while ((*p >= '0' && *p <= '9') || (*p >= 'a' && *p <= 'f') || (*p >= 'A' && *p <= 'F'))
        {
            sscanf(p, "%x", &audio_id);
            if (audio_id > 0xc7)
                break;
            audio[audio_id].selected_for_demux = true;
            while (*p != ',' && *p != 0) p++;
            if (*p == 0)
                break;
            p++;
        }
		fscanf(INIFile, "DR_Control=%d\n", &DRC_Flag);
		fscanf(INIFile, "DS_Downmix=%d\n", &DSDown_Flag);
		fscanf(INIFile, "SRC_Precision=%d\n", &SRC_Flag);
		fscanf(INIFile, "Norm_Ratio=%d\n", &Norm_Ratio);
		fscanf(INIFile, "Process_Priority=%d\n", &Priority_Flag);
		fscanf(INIFile, "Playback_Speed=%d\n", &PlaybackSpeed);
		fscanf(INIFile, "Force_Open_Gops=%d\n", &ForceOpenGops);
		fgets(line, DG_MAX_PATH - 1, INIFile);
		line[strlen(line)-1] = 0;
		p = line;
		while (*p++ != '=');
		strcpy (AVSTemplatePath, p);
        fscanf(INIFile, "Full_Path_In_Files=%d\n", &FullPathInFiles);
        fscanf(INIFile, "Fusion_Audio=%d\n", &FusionAudio);
        fscanf(INIFile, "Loop_Playback=%d\n", &LoopPlayback);
		fscanf(INIFile, "HD_Display=%d\n", &HDDisplay);
        for (i = 0; i < 4; i++)
        {
		    fgets(line, DG_MAX_PATH - 1, INIFile);
		    line[strlen(line)-1] = 0;
		    p = line;
		    while (*p++ != '=');
		    strcpy(mMRUList[i], p);
        }
        fscanf(INIFile, "Enable_Info_Log=%d\n", &InfoLog_Flag);
		fgets(line, DG_MAX_PATH - 1, INIFile);
		line[strlen(line)-1] = 0;
		p = line;
		while (*p++ != '=');
		strcpy(BMPPathString, p);
        fscanf(INIFile, "Use_MPA_Extensions=%d\n", &UseMPAExtensions);
        fscanf(INIFile, "Notify_When_Done=%d\n", &NotifyWhenDone);
		fclose(INIFile);
	}

    // Allocate stream buffer.
    Rdbfr = (unsigned char *) malloc(BUFFER_SIZE);
    if (Rdbfr == NULL)
    {
		MessageBox(hWnd, "Couldn't allocate stream buffer using configured size.\nTrying default size.", NULL, MB_OK | MB_ICONERROR);
        Rdbfr = (unsigned char *) malloc(32 * SECTOR_SIZE);
        if (Rdbfr == NULL)
        {
		    MessageBox(hWnd, "Couldn't allocate stream buffer using default size.\nExiting.", NULL, MB_OK | MB_ICONERROR);
		    exit(0);
        }
    }

	// Perform application initialization
	hInst = hInstance;

	// Load accelerators
	hAccel = LoadAccelerators(hInst, (LPCTSTR)IDR_ACCELERATOR);

	// Initialize global strings
	LoadString(hInst, IDC_GUI, szWindowClass, DG_MAX_PATH);
	MyRegisterClass(hInst);

	hWnd = CreateWindow(szWindowClass, "DGIndex", WS_OVERLAPPEDWINDOW & ~(WS_THICKFRAME|WS_MAXIMIZEBOX),
		CW_USEDEFAULT, 0, INIT_WIDTH, INIT_HEIGHT, NULL, NULL, hInst, NULL);

	// Test CPU
	__asm
	{
		mov			eax, 1
		cpuid
		test		edx, 0x00800000		// STD MMX
		jz			TEST_SSE
		mov			[cpu.mmx], 1
TEST_SSE:
		test		edx, 0x02000000		// STD SSE
		jz			TEST_SSE2
		mov			[cpu.ssemmx], 1
		mov			[cpu.ssefpu], 1
TEST_SSE2:
		test		edx, 0x04000000		// SSE2	
		jz			TEST_3DNOW
		mov			[cpu.sse2], 1
TEST_3DNOW:
		mov			eax, 0x80000001
		cpuid
		test		edx, 0x80000000		// 3D NOW
		jz			TEST_SSEMMX
		mov			[cpu._3dnow], 1
TEST_SSEMMX:
		test		edx, 0x00400000		// SSE MMX
		jz			TEST_END
		mov			[cpu.ssemmx], 1
TEST_END:
	}

	if (!cpu.sse2)
	{
		DeleteMenu(hMenu, IDM_IDCT_SSE2MMX, 0);
	}

	if (!cpu.ssemmx)
	{
		DeleteMenu(hMenu, IDM_IDCT_SSEMMX, 0);
		DeleteMenu(hMenu, IDM_IDCT_SKAL, 0);
	}

	if (cpu.mmx)
		CheckMenuItem(hMenu, IDM_MMX, MF_CHECKED);
	else
		DestroyWindow(hWnd);

	if (cpu.ssemmx)
		CheckMenuItem(hMenu, IDM_SSEMMX, MF_CHECKED);

	if (cpu.sse2)
		CheckMenuItem(hMenu, IDM_SSE2, MF_CHECKED);

	if (cpu._3dnow)
		CheckMenuItem(hMenu, IDM_3DNOW, MF_CHECKED);

	if (cpu.ssefpu)
		CheckMenuItem(hMenu, IDM_SSEFPU, MF_CHECKED);

	// Create control
	hTrack = CreateWindow(TRACKBAR_CLASS, NULL,
		WS_CHILD | WS_VISIBLE | WS_DISABLED | TBS_NOTICKS | TBS_TOP,
		0, INIT_HEIGHT, INIT_WIDTH-4*TRACK_HEIGHT, TRACK_HEIGHT, hWnd, (HMENU) ID_TRACKBAR, hInst, NULL);
	SendMessage(hTrack, TBM_SETRANGE, (WPARAM) true, (LPARAM) MAKELONG(0, TRACK_PITCH));

	hLeftButton = CreateWindow("BUTTON", "[",
		WS_CHILD | WS_VISIBLE | WS_DLGFRAME | WS_DISABLED,
		INIT_WIDTH-4*TRACK_HEIGHT, INIT_HEIGHT,
		TRACK_HEIGHT, TRACK_HEIGHT, hWnd, (HMENU) ID_LEFT_BUTTON, hInst, NULL);

	hLeftArrow = CreateWindow("BUTTON", "<",
		WS_CHILD | WS_VISIBLE | WS_DLGFRAME | WS_DISABLED,
		INIT_WIDTH-3*TRACK_HEIGHT, INIT_HEIGHT,
		TRACK_HEIGHT, TRACK_HEIGHT, hWnd, (HMENU) ID_LEFT_ARROW, hInst, NULL);

	hRightArrow = CreateWindow("BUTTON", ">",
		WS_CHILD | WS_VISIBLE | WS_DLGFRAME | WS_DISABLED,
		INIT_WIDTH-2*TRACK_HEIGHT, INIT_HEIGHT,
		TRACK_HEIGHT, TRACK_HEIGHT, hWnd, (HMENU) ID_RIGHT_ARROW, hInst, NULL);

	hRightButton = CreateWindow("BUTTON", "]",
		WS_CHILD | WS_VISIBLE | WS_DLGFRAME | WS_DISABLED,
		INIT_WIDTH-TRACK_HEIGHT, INIT_HEIGHT,
		TRACK_HEIGHT, TRACK_HEIGHT, hWnd, (HMENU) ID_RIGHT_BUTTON, hInst, NULL);

	ResizeWindow(INIT_WIDTH, INIT_HEIGHT);
	MoveWindow(hWnd, INIT_X, INIT_Y, INIT_WIDTH+Edge_Width, INIT_HEIGHT+Edge_Height+TRACK_HEIGHT+TRACK_HEIGHT/3, true);

	MPEG2_Transport_VideoPID = 2;
	MPEG2_Transport_AudioPID = 2;
	MPEG2_Transport_PCRPID = 2;

	// Command line init.
	strcpy(ucCmdLine, lpCmdLine);
	_strupr(ucCmdLine);

	// Show window normal, minimized, or hidden as appropriate.
	if (*lpCmdLine == 0)
		WindowMode = SW_SHOW;
	else
	{
		if (strstr(ucCmdLine,"-MINIMIZE"))
			WindowMode = SW_MINIMIZE;
		else if (strstr(ucCmdLine,"-HIDE"))
			WindowMode = SW_HIDE;
		else
			WindowMode = SW_SHOW;
	}
	ShowWindow(hWnd, WindowMode);

	StartupEnables();
	CheckFlag();
	CLIActive = 0;

	// First check whether we have "Open With" invocation.
	if (*lpCmdLine != 0)
	{
		#define OUT_OF_FILE 0
		#define IN_FILE_QUOTED 1
		#define IN_FILE_BARE 2
		#define MAX_CMD 2048
		int n, i, j, k;
		FILE* tmp;
		int state, ndx;
		char *swp;

//		MessageBox(hWnd, lpCmdLine, NULL, MB_OK);
		ptr = lpCmdLine;
		// Look at the first non-white-space character.
		// If it is a '-' we have CLI invocation, else
		// we have "Open With" invocation.
		while (*ptr == ' ' && *ptr == '\t') ptr++;
		if (*ptr != '-')
		{
			// "Open With" invocation.
			NumLoadedFiles = 0;
			// Pick up all the filenames.
			// The command line will look like this (with the quotes!):
			// "c:\my dir\file1.vob" c:\dir\file2.vob ...
			// The paths with spaces have quotes; those without do not.
			// This is tricky to parse, so use a state machine.
			state = OUT_OF_FILE;
			for (k = 0; k < MAX_CMD; k++)
			{
				if (state == OUT_OF_FILE)
				{
					if (*ptr == 0)
					{
						break;
					}
					else if (*ptr == ' ')
					{
					}
					else if (*ptr == '"')
					{
						state = IN_FILE_QUOTED;
						ndx = 0;
					}
					else
					{
						state = IN_FILE_BARE;
						ndx = 0;
						cwd[ndx++] = *ptr;
					}
				}
				else if (state == IN_FILE_QUOTED)
				{
					if (*ptr == '"')
					{
						cwd[ndx] = 0;
						if ((tmp = fopen(cwd, "rb")) != NULL)
						{
//							MessageBox(hWnd, "Open OK", NULL, MB_OK);
							strcpy(Infilename[NumLoadedFiles], cwd);
							Infile[NumLoadedFiles] = tmp;
							NumLoadedFiles++;
						}
						state = OUT_OF_FILE;
					}
					else
					{
						cwd[ndx++] = *ptr;
					}
				}
				else if (state == IN_FILE_BARE)
				{
					if (*ptr == 0)
					{
						cwd[ndx] = 0;
						if ((tmp = fopen(cwd, "rb")) != NULL)
						{
//							MessageBox(hWnd, "Open OK", NULL, MB_OK);
							strcpy(Infilename[NumLoadedFiles], cwd);
							Infile[NumLoadedFiles] = tmp;
							NumLoadedFiles++;
						}
						break;
					}
					else if (*ptr == ' ')
					{
						cwd[ndx] = 0;
						if ((tmp = fopen(cwd, "rb")) != NULL)
						{
//							MessageBox(hWnd, "Open OK", NULL, MB_OK);
							strcpy(Infilename[NumLoadedFiles], cwd);
							Infile[NumLoadedFiles] = tmp;
							NumLoadedFiles++;
						}
						state = OUT_OF_FILE;
					}
					else
					{
						cwd[ndx++] = *ptr;
					}
				}
				ptr++;
			}
			// Sort the filenames.
			n = NumLoadedFiles;
			for (i = 0; i < n - 1; i++)
			{
				for (j = 0; j < n - 1 - i; j++)
				{
					if (strverscmp(Infilename[j+1], Infilename[j]) < 0)
					{
						swp = Infilename[j];
						Infilename[j] = Infilename[j+1];
						Infilename[j+1] = swp;
						tmp = Infile[j];
						Infile[j] = Infile[j+1];
						Infile[j+1] = tmp;
					}
				}
			}
			// Start everything up with these files.
			Recovery();
			RefreshWindow(true);
			// Force the following CLI processing to be skipped.
			*lpCmdLine = 0;
			if (NumLoadedFiles)
			{
				// Start a LOCATE_INIT thread. When it kills itself, it will start a
				// LOCATE_RIP thread by sending a WM_USER message to the main window.
				process.rightfile = NumLoadedFiles-1;
				process.rightlba = (int)(Infilelength[NumLoadedFiles-1]/SECTOR_SIZE);
				process.end = Infiletotal - SECTOR_SIZE;
				process.endfile = NumLoadedFiles - 1;
				process.endloc = (Infilelength[NumLoadedFiles-1]/SECTOR_SIZE - 1)*SECTOR_SIZE;
				process.locate = LOCATE_INIT;
				if (!threadId || WaitForSingleObject(hThread, INFINITE)==WAIT_OBJECT_0)
				  hThread = CreateThread(NULL, 0, MPEG2Dec, 0, 0, &threadId);
			}
		}
	}

	if (*lpCmdLine)
	{
		// CLI invocation.
		if (parse_cli(lpCmdLine, ucCmdLine) != 0)
			exit(EXIT_FAILURE);
		if (NumLoadedFiles)
		{
			// Start a LOCATE_INIT thread. When it kills itself, it will start a
			// LOCATE_RIP thread by sending a WM_USER message to the main window.
			PlaybackSpeed = SPEED_MAXIMUM;
			// If the project range wasn't set with the RG option, set it to the entire timeline.
			if (!hadRGoption)
			{
				process.rightfile = NumLoadedFiles-1;
				process.rightlba = (int)(Infilelength[NumLoadedFiles-1]/SECTOR_SIZE);
				process.end = Infiletotal - SECTOR_SIZE;
				process.endfile = NumLoadedFiles - 1;
				process.endloc = (Infilelength[NumLoadedFiles-1]/SECTOR_SIZE - 1)*SECTOR_SIZE;
			}
			process.locate = LOCATE_INIT;
			if (!threadId || WaitForSingleObject(hThread, INFINITE)==WAIT_OBJECT_0)
				hThread = CreateThread(NULL, 0, MPEG2Dec, 0, 0, &threadId);
		}
	}

    UpdateMRUList();

	// Main message loop
	while (GetMessage(&msg, NULL, 0, 0)) 
	{
		if (!TranslateAccelerator(hWnd, hAccel, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	return msg.wParam;
}

HBITMAP splash = NULL;

// Processes messages for the main window
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	DWORD wmId, wmEvent;

	HDC hdc;
	PAINTSTRUCT ps;
	char prog[DG_MAX_PATH];
	char path[DG_MAX_PATH];
	char avsfile[DG_MAX_PATH];
	LPTSTR path_p, prog_p;
	FILE *tplate, *avs;

	int i, j;

	WNDCLASS rwc = {0};

	switch (message)
	{
		case CLI_RIP_MESSAGE:
			// The CLI-invoked LOCATE_INIT thread is finished.
			// Kick off a LOCATE_RIP thread.
			if (CLIPreview)
				goto preview;
			else
				goto proceed;

		case CLI_PREVIEW_DONE_MESSAGE:
			// Destroy the Info dialog to generate the info log file.
			DestroyWindow(hDlg);
			if (ExitOnEnd)
				exit(0);
			break;

		case D2V_DONE_MESSAGE:
			// Make an AVS file if it doesn't already exist and a template exists.
			strcpy(avsfile, D2VFilePath);
			path_p = strrchr(avsfile, '.');
			if (strstr(AVSTemplatePath, ".vpy") != NULL)
				strcpy(++path_p, "vpy");
			else
				strcpy(++path_p, "avs");
			if (*AVSTemplatePath && !fopen(avsfile, "r") && (tplate = fopen(AVSTemplatePath, "r")))
			{
				avs = fopen(avsfile, "w");
				if (avs)
				{
					while (fgets(path, 1023, tplate))
					{
						path_p = path;
						prog_p = prog;
						while (1)
						{
							if (*path_p == 0)
							{
								*prog_p = 0;
								break;
							}
							else if (path_p[0] == '_' && path_p[1] == '_' && path_p[2] == 'v' &&
								path_p[3] == 'i' && path_p[4] == 'd' && path_p[5] == '_' && path_p[6] == '_')
							{
								// Replace __vid__ macro.
								*prog_p = 0;
								if (FullPathInFiles)
								{
									strcat(prog_p, D2VFilePath);
									prog_p = &prog[strlen(prog)];
									path_p += 7;
								}
								else
								{
									char *p;
									if ((p = strrchr(D2VFilePath,'\\')) != 0) p++;
									else p = D2VFilePath;
									strcat(prog_p, p);
									prog_p = &prog[strlen(prog)];
									path_p += 7;

								}
							}
							else if (path_p[0] == '_' && path_p[1] == '_' && path_p[2] == 'a' &&
								path_p[3] == 'u' && path_p[4] == 'd' && path_p[5] == '_' && path_p[6] == '_')
							{
								// Replace __aud__ macro.
								*prog_p = 0;
								if (FullPathInFiles)
								{
									strcat(prog_p, AudioFilePath);
									prog_p = &prog[strlen(prog)];
									path_p += 7;
								}
								else
								{
									char *p;
									if ((p = strrchr(AudioFilePath,'\\')) != 0) p++;
									else p = AudioFilePath;
									strcat(prog_p, p);
									prog_p = &prog[strlen(prog)];
									path_p += 7;
								}
							}
							else if (AudioFilePath && path_p[0] == '_' && path_p[1] == '_' && path_p[2] == 'd' &&
								path_p[3] == 'e' && path_p[4] == 'l' && path_p[5] == '_' && path_p[6] == '_')
							{
								// Replace __del__ macro.
								char *d = &AudioFilePath[strlen(AudioFilePath)-3];
								int delay;
								float fdelay;
								char fdelay_str[32];
								while (d > AudioFilePath)
								{
									if (d[0] == 'm' && d[1] == 's' && d[2] == '.')
										break;
									d--;
								}
								if (d > AudioFilePath)
								{
									while ((d > AudioFilePath) && d[0] != ' ') d--;
									if (d[0] == ' ')
									{
										sscanf(d, "%d", &delay);
										fdelay = (float) 0.001 * delay;
										sprintf(fdelay_str, "%.3f", fdelay);
										*prog_p = 0;
										strcat(prog_p, fdelay_str);
										prog_p = &prog[strlen(prog)];
										path_p += 7;
									}
									else
										*prog_p++ = *path_p++;
								}
								else
									*prog_p++ = *path_p++;
							}
							else
							{
								*prog_p++ = *path_p++;
							}
						}
						fputs(prog, avs);
					}
					fclose(tplate);
					fclose(avs);
				}
			}
			if (ExitOnEnd)
			{
				if (Info_Flag)
					DestroyWindow(hDlg);
				exit(0);
			}
			else CLIActive = 0;
			break;

		case PROGRESS_MESSAGE: 
			OutputProgress(wParam);
			break;

		case WM_CREATE:
			PreScale_Ratio = 1.0;

			process.trackleft = 0;
			process.trackright = TRACK_PITCH;

			rwc.lpszClassName = TEXT("SelectControl");
			rwc.hbrBackground = GetSysColorBrush(COLOR_BTNFACE);
			rwc.style         = CS_HREDRAW;
			rwc.lpfnWndProc   = SelectProc;
			RegisterClass(&rwc);

			hwndSelect = CreateWindowEx(0, TEXT("SelectControl"), NULL, 
				WS_CHILD | WS_VISIBLE, 12, 108, 370, TRACK_HEIGHT/3, hWnd, NULL, NULL, NULL);

            hDC = GetDC(hWnd);
			hMenu = GetMenu(hWnd);
			hProcess = GetCurrentProcess();

			// Load the splash screen from the file dgindex.bmp if it exists.
			strcpy(prog, ExePath);
			strcat(prog, "dgindex.bmp");
			splash = (HBITMAP) ::LoadImage (0, prog, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
//			if (splash == 0)
//			{
//				// No splash file. Use the built-in default splash screen.
//				splash = LoadBitmap(GetModuleHandle(NULL), MAKEINTRESOURCE(IDB_SPLASH));
//			}

			for (i=0; i<MAX_FILE_NUMBER; i++)
				Infilename[i] = (char*)malloc(DG_MAX_PATH);

			for (i=0; i<8; i++)
				block[i] = (short *)_aligned_malloc(sizeof(short)*64, 64);

			Initialize_FPU_IDCT();

			// register VFAPI
			HKEY key; DWORD trash;

			if (RegCreateKeyEx(HKEY_CURRENT_USER, "Software\\VFPlugin", 0, "",
				REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &key, &trash) == ERROR_SUCCESS)
			{
				if (_getcwd(szBuffer, DG_MAX_PATH)!=NULL)
				{
					if (szBuffer[strlen(szBuffer)-1] != '\\')
						strcat(szBuffer, "\\");

					strcpy(szPath, szBuffer);

					struct _finddata_t vfpfile;
					if (_findfirst("DGVfapi.vfp", &vfpfile) != -1L)
					{
						strcat(szBuffer, "DGVfapi.vfp");

						RegSetValueEx(key, "DGIndex", 0, REG_SZ, (LPBYTE)szBuffer, strlen(szBuffer));
						CheckMenuItem(hMenu, IDM_VFAPI, MF_CHECKED);
					}

					RegCloseKey(key);
				}
			}

		case WM_COMMAND:
        {
            int show_info;

            show_info = true;
			wmId    = LOWORD(wParam);
			wmEvent = HIWORD(wParam);

			// parse the menu selections
			switch (wmId)
			{
                case ID_MRU_FILE0:
                case ID_MRU_FILE1:
                case ID_MRU_FILE2:
                case ID_MRU_FILE3:
                    {
                        FILE *tmp;

                        NumLoadedFiles = 0;
                        if ((tmp = fopen(mMRUList[wmId - ID_MRU_FILE0], "rb")) != NULL)
						{
                            strcpy(Infilename[NumLoadedFiles], mMRUList[wmId - ID_MRU_FILE0]);
					        Infile[NumLoadedFiles] = tmp;
                            NumLoadedFiles = 1;
 			                Recovery();
							MPEG2_Transport_VideoPID = 2;
							MPEG2_Transport_AudioPID = 2;
							MPEG2_Transport_PCRPID = 2;
			                RefreshWindow(true);
				            // Start a LOCATE_INIT thread. When it kills itself, it will start a
				            // LOCATE_RIP thread by sending a WM_USER message to the main window.
				            process.rightfile = NumLoadedFiles-1;
				            process.rightlba = (int)(Infilelength[NumLoadedFiles-1]/SECTOR_SIZE);
				            process.end = Infiletotal - SECTOR_SIZE;
				            process.endfile = NumLoadedFiles - 1;
				            process.endloc = (Infilelength[NumLoadedFiles-1]/SECTOR_SIZE - 1)*SECTOR_SIZE;
				            process.locate = LOCATE_INIT;
				            if (!threadId || WaitForSingleObject(hThread, INFINITE)==WAIT_OBJECT_0)
				                hThread = CreateThread(NULL, 0, MPEG2Dec, 0, 0, &threadId);
                        }
                        else
                        {
                            MessageBox(hWnd, "Cannot open the specified file", "Open file error", MB_OK | MB_ICONERROR);
                            DeleteMRUList(wmId - ID_MRU_FILE0);
                        }
                        break;
                    }

 				case IDM_OPEN:
					if (Info_Flag)
					{
						DestroyWindow(hDlg);
						Info_Flag = false;
					}
					DialogBox(hInst, (LPCTSTR)IDD_FILELIST, hWnd, (DLGPROC)VideoList);
					break;

				case IDM_CLOSE:
					if (Info_Flag)
					{
						DestroyWindow(hDlg);
						Info_Flag = false;
					}
					while (NumLoadedFiles)
					{
						NumLoadedFiles--;
						fclose(Infile[NumLoadedFiles]);
					}
					Recovery();
					MPEG2_Transport_VideoPID = 2;
					MPEG2_Transport_AudioPID = 2;
					MPEG2_Transport_PCRPID = 2;
					break;

				case IDM_PREVIEW_NO_INFO:
                    show_info = false;
                    wmId = IDM_PREVIEW;
					goto skip;
preview:
					show_info = true;
                    wmId = IDM_PREVIEW;
skip:
				case IDM_PREVIEW:
				case IDM_PLAY:
					if (!Check_Flag)
					{
						MessageBox(hWnd, "No data. Check your PIDS.", "Preview/Play", MB_OK | MB_ICONWARNING);
					}
					else if (IsWindowEnabled(hTrack))
					{
						RunningEnables();
						Display_Flag = true;
						// Initialize for single stepping.
						RightArrowHit = false;
						if (show_info == true)
                            ShowInfo(true);

						if (SystemStream_Flag == TRANSPORT_STREAM)
						{
							MPEG2_Transport_AudioType =
										pat_parser.GetAudioType(Infilename[0], MPEG2_Transport_AudioPID);
						}

						if (wmId == IDM_PREVIEW)
							process.locate = LOCATE_RIP;
						else
							process.locate = LOCATE_PLAY;

						if (CLIPreview || WaitForSingleObject(hThread, INFINITE)==WAIT_OBJECT_0)
							hThread = CreateThread(NULL, 0, MPEG2Dec, 0, 0, &threadId);
					}
					break;

				case IDM_DEMUX_AUDIO:
					if (Method_Flag == AUDIO_NONE)
					{
						MessageBox(hWnd, "Audio demuxing is disabled.\nEnable it first in the Audio/Output Method menu.", NULL, MB_OK | MB_ICONERROR);
						break;
					}
					process.locate = LOCATE_DEMUX_AUDIO;
					RunningEnables();
					ShowInfo(true);
					if (CLIActive || WaitForSingleObject(hThread, INFINITE)==WAIT_OBJECT_0)
					{
						hThread = CreateThread(NULL, 0, MPEG2Dec, 0, 0, &threadId);
					}
					break;

				case IDM_SAVE_D2V_AND_DEMUX:
					MuxFile = (FILE *) 0;
					goto proceed;

				case IDM_SAVE_D2V:
					// No video demux.
					MuxFile = (FILE *) 0xffffffff;
proceed:
					if (!Check_Flag)
					{
						MessageBox(hWnd, "No data. Check your PIDS.", "Save Project", MB_OK | MB_ICONWARNING);
						break;
					}
					if (!CLIActive && (FO_Flag == FO_FILM) && ((mpeg_type == IS_MPEG1) || ((int) (frame_rate * 1000) != 29970)))
					{
						char buf[255];
						sprintf(buf, "It is almost always wrong to run Force Film on an MPEG1 stream,\n"
							"or a stream with a frame rate other than 29.970.\n"
							"Your stream is %s and has a frame rate of %.3f.\n"
							"Do you want to do it anyway?",
							mpeg_type == IS_MPEG1 ? "MPEG1" : "MPEG2", frame_rate); 
						if (MessageBox(hWnd, buf, "Force Film Warning", MB_YESNO | MB_ICONWARNING) != IDYES)
							break;
					}
					if (CLIActive || PopFileDlg(szOutput, hWnd, SAVE_D2V))
					{
						sprintf(szBuffer, "%s.d2v", szOutput);
						if (CLIActive)
						{
							if ((D2VFile = fopen(szBuffer, "w")) == 0)
							{
								MessageBoxTimeout(hWnd, "Couldn't open D2V file. Is it read-only?", NULL, MB_OK | MB_ICONERROR, 0, CLIActive ? CLITimeout : 0xffffffff);
								exit(EXIT_FAILURE);
							}
							strcpy(D2VFilePath, szBuffer);
						}
						else 
						{
							if (D2VFile = fopen(szBuffer, "r"))
							{
								char line[255];

                                fclose(D2VFile);
							    sprintf(line, "%s already exists.\nDo you want to replace it?", szBuffer);
							    if (MessageBox(hWnd, line, "Save D2V",
								    MB_YESNO | MB_ICONWARNING) != IDYES)
								    break;
							}
							D2VFile = fopen(szBuffer, "w");
							if (D2VFile == NULL)
							{
								MessageBoxTimeout(hWnd, "Couldn't open D2V file for writing. Is it read-only?", NULL, MB_OK | MB_ICONERROR, 0, CLIActive ? CLITimeout : 0xffffffff);
							}
							strcpy(D2VFilePath, szBuffer);
						}

						if (D2VFile != 0)
						{
							if (LogQuants_Flag)
							{
								// Initialize quant matric logging.
								// Generate the output file name.
								char *p;
								p = &szBuffer[strlen(szBuffer)];
								while (*p != '.') p--;
								p[1] = 0;
								strcat(p, "quants.txt");
								// Open the output file.
								Quants = fopen(szBuffer, "w");
								// Init the recorded quant matrices for change detection.
								memset(intra_quantizer_matrix_log, 0xffffffff, sizeof(intra_quantizer_matrix_log));
								memset(non_intra_quantizer_matrix_log, 0xffffffff, sizeof(non_intra_quantizer_matrix_log));
								memset(chroma_intra_quantizer_matrix_log, 0xffffffff, sizeof(chroma_intra_quantizer_matrix_log));
								memset(chroma_non_intra_quantizer_matrix_log, 0xffffffff, sizeof(chroma_non_intra_quantizer_matrix_log));
							}

							if (LogTimestamps_Flag)
							{
								// Initialize timestamp logging.
								// Generate the output file name.
								char *p;
								p = &szBuffer[strlen(szBuffer)];
								while (*p != '.') p--;
								p[1] = 0;
								strcat(p, "timestamps.txt");
								// Open the output file.
								Timestamps = fopen(szBuffer, "w");
								fprintf(Timestamps, "DGIndex Timestamps Dump\n\n");
								fprintf(Timestamps, "frame rate = %f\n", frame_rate);
							}

							D2V_Flag = true;
							Display_Flag = false;
							gop_positions_ndx = 0;
							RunningEnables();
							ShowInfo(true);
							// Get the audio type so that we parse correctly for transport streams.
							if (SystemStream_Flag == TRANSPORT_STREAM)
							{
								MPEG2_Transport_AudioType =
											pat_parser.GetAudioType(Infilename[0], MPEG2_Transport_AudioPID);
							}

							process.locate = LOCATE_RIP;
							if (CLIActive || WaitForSingleObject(hThread, INFINITE)==WAIT_OBJECT_0)
							{
								hThread = CreateThread(NULL, 0, MPEG2Dec, 0, 0, &threadId);
							}
						}
					}
					break;

				case IDM_LOAD_D2V:
					if (PopFileDlg(szInput, hWnd, OPEN_D2V))
					{
D2V_PROCESS:
						int m, n;
						unsigned int num, den;
						char line[2048];
						int D2Vformat;

						D2VFile = fopen(szInput, "r");

						// Validate the D2V file.
						fgets(line, 2048, D2VFile);
						if (strncmp(line, "DGIndexProjectFile", 18) != 0)
						{
							MessageBox(hWnd, "The file is not a DGIndex project file!", NULL, MB_OK | MB_ICONERROR);
							fclose(D2VFile);
							break;
						}
						sscanf(line, "DGIndexProjectFile%d", &D2Vformat);
						if (D2Vformat != D2V_FILE_VERSION)
						{
							MessageBox(hWnd, "Obsolete D2V file.\nRecreate it with this version of DGIndex.", NULL, MB_OK | MB_ICONERROR);
							fclose(D2VFile);
							break;
						}

						// Close any opened files.
						while (NumLoadedFiles)
						{
							NumLoadedFiles--;
							fclose(Infile[NumLoadedFiles]);
                            Infile[NumLoadedFiles] = NULL;
						}

						fscanf(D2VFile, "%d\n", &NumLoadedFiles);

						i = NumLoadedFiles;
						while (i)
						{
							fgets(Infilename[NumLoadedFiles-i], DG_MAX_PATH - 1, D2VFile);
							// Strip newline.
							Infilename[NumLoadedFiles-i][strlen(Infilename[NumLoadedFiles-i])-1] = 0;
							if ((Infile[NumLoadedFiles-i] = fopen(Infilename[NumLoadedFiles-i], "rb")) == NULL)
							{
								while (i<NumLoadedFiles)
								{
									fclose(Infile[NumLoadedFiles-i-1]);
                                    Infile[NumLoadedFiles-i-1] = NULL;
									i++;
								}

								NumLoadedFiles = 0;
								break;
							}

							i--;
						}

						Recovery();

						fscanf(D2VFile, "\nStream_Type=%d\n", &SystemStream_Flag);
						if (SystemStream_Flag == TRANSPORT_STREAM)
						{
							fscanf(D2VFile, "MPEG2_Transport_PID=%x,%x,%x\n",
								   &MPEG2_Transport_VideoPID, &MPEG2_Transport_AudioPID, &MPEG2_Transport_PCRPID);
							fscanf(D2VFile, "Transport_Packet_Size=%d\n", &TransportPacketSize);
						}
                        // Don't use the read value. It will be detected.
                        SystemStream_Flag = ELEMENTARY_STREAM;
						fscanf(D2VFile, "MPEG_Type=%d\n", &mpeg_type);
						fscanf(D2VFile, "iDCT_Algorithm=%d\n", &iDCT_Flag);
						fscanf(D2VFile, "YUVRGB_Scale=%d\n", &Scale_Flag);
						setRGBValues();
						fscanf(D2VFile, "Luminance_Filter=%d,%d\n", &LumGamma, &LumOffset);

						if (LumGamma || LumOffset)
						{
							CheckMenuItem(hMenu, IDM_LUMINANCE, MF_CHECKED);
							Luminance_Flag = true;				
						}
						else
						{
							CheckMenuItem(hMenu, IDM_LUMINANCE, MF_UNCHECKED);
							Luminance_Flag = false;
						}

						fscanf(D2VFile, "Clipping=%d,%d,%d,%d\n", 
								&Clip_Left, &Clip_Right, &Clip_Top, &Clip_Bottom);

						if (Clip_Top || Clip_Bottom || Clip_Left || Clip_Right)
						{
							CheckMenuItem(hMenu, IDM_CROPPING, MF_CHECKED);
							Cropping_Flag = true;				
						}
						else
						{
							CheckMenuItem(hMenu, IDM_CROPPING, MF_UNCHECKED);
							Cropping_Flag = false;
						}

						fscanf(D2VFile, "Aspect_Ratio=%d:%d\n", &m, &n);
						fscanf(D2VFile, "Picture_Size=%dx%d\n", &m, &n);
						fscanf(D2VFile, "Field_Operation=%d\n", &FO_Flag);
						fscanf(D2VFile, "Frame_Rate=%d (%u/%u)\n", &m, &num, &den);

						CheckFlag();

						if (NumLoadedFiles)
						{
							fscanf(D2VFile, "Location=%d,%I64x,%d,%I64x\n",
								&process.leftfile, &process.leftlba, &process.rightfile, &process.rightlba);

                            process.startfile = process.leftfile;
							process.startloc = process.leftlba * SECTOR_SIZE;
						    process.end = Infiletotal - SECTOR_SIZE;
						    process.endfile = process.rightfile;
						    process.endloc = process.rightlba* SECTOR_SIZE;
							process.run = 0;
							process.start = process.startloc;
							process.trackleft = (process.startloc*TRACK_PITCH/Infiletotal);
							process.trackright = (process.endloc*TRACK_PITCH/Infiletotal);
							process.locate = LOCATE_INIT;
						    InvalidateRect(hwndSelect, NULL, TRUE);

							if (!threadId || WaitForSingleObject(hThread, INFINITE)==WAIT_OBJECT_0)
								hThread = CreateThread(NULL, 0, MPEG2Dec, 0, 0, &threadId);
						}
					}
					break;

				case IDM_PARSE_D2V:
					if (PopFileDlg(szInput, hWnd, OPEN_D2V))
					{
						if (parse_d2v(hWnd, szInput))
						{
							ShellExecute(hDlg, "open", szInput, NULL, NULL, SW_SHOWNORMAL);
						}
					}
					break;

				case IDM_FIX_D2V:
					if (PopFileDlg(szInput, hWnd, OPEN_D2V))
					{
						fix_d2v(hWnd, szInput, 0);
					}
					break;

				case IDM_LOG_QUANTS:
					if (LogQuants_Flag == 0)
					{
						// Enable quant matrix logging.
						LogQuants_Flag = 1;
						CheckMenuItem(hMenu, IDM_LOG_QUANTS, MF_CHECKED);
					}
					else
					{
						// Disable quant matrix logging.
						LogQuants_Flag = 0;
						CheckMenuItem(hMenu, IDM_LOG_QUANTS, MF_UNCHECKED);
					}
					break;

				case IDM_LOG_TIMESTAMPS:
					if (LogTimestamps_Flag == 0)
					{
						// Enable quant matrix logging.
						LogTimestamps_Flag = 1;
						CheckMenuItem(hMenu, IDM_LOG_TIMESTAMPS, MF_CHECKED);
					}
					else
					{
						// Disable quant matrix logging.
						LogTimestamps_Flag = 0;
						CheckMenuItem(hMenu, IDM_LOG_TIMESTAMPS, MF_UNCHECKED);
					}
					break;

				case IDM_INFO_LOG:
					if (InfoLog_Flag == 0)
					{
						// Enable quant matrix logging.
						InfoLog_Flag = 1;
						CheckMenuItem(hMenu, IDM_INFO_LOG, MF_CHECKED);
					}
					else
					{
						// Disable quant matrix logging.
						InfoLog_Flag = 0;
						CheckMenuItem(hMenu, IDM_INFO_LOG, MF_UNCHECKED);
					}
					break;

				case IDM_STOP:
					Stop_Flag = true;
					ExitOnEnd = 0;
					CLIActive = 0;
					FileLoadedEnables();

					if (Pause_Flag)
						ResumeThread(hThread);
					break;

				case IDM_AUDIO_NONE:
					Method_Flag = AUDIO_NONE;
					CheckMenuItem(hMenu, IDM_AUDIO_NONE, MF_CHECKED);
					CheckMenuItem(hMenu, IDM_DEMUX, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_DEMUXALL, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_DECODE, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_NORM, MF_UNCHECKED);
					Norm_Flag = false;
					EnableMenuItem(GetSubMenu(hMenu, 3), 1, MF_BYPOSITION | MF_GRAYED);
					EnableMenuItem(GetSubMenu(hMenu, 3), 3, MF_BYPOSITION | MF_GRAYED);
					EnableMenuItem(GetSubMenu(hMenu, 3), 4, MF_BYPOSITION | MF_GRAYED);
					EnableMenuItem(GetSubMenu(hMenu, 3), 5, MF_BYPOSITION | MF_GRAYED);
					break;

				case IDM_DEMUX:
					Method_Flag = AUDIO_DEMUX;
					CheckMenuItem(hMenu, IDM_AUDIO_NONE, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_DEMUX, MF_CHECKED);
					CheckMenuItem(hMenu, IDM_DEMUXALL, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_DECODE, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_NORM, MF_UNCHECKED);
					Norm_Flag = false;
					EnableMenuItem(GetSubMenu(hMenu, 3), 1, MF_BYPOSITION | MF_ENABLED);
					EnableMenuItem(GetSubMenu(hMenu, 3), 3, MF_BYPOSITION | MF_GRAYED);
					EnableMenuItem(GetSubMenu(hMenu, 3), 4, MF_BYPOSITION | MF_GRAYED);
					EnableMenuItem(GetSubMenu(hMenu, 3), 5, MF_BYPOSITION | MF_GRAYED);
					break;

				case IDM_DEMUXALL:
					Method_Flag = AUDIO_DEMUXALL;
					CheckMenuItem(hMenu, IDM_AUDIO_NONE, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_DEMUX, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_DEMUXALL, MF_CHECKED);
					CheckMenuItem(hMenu, IDM_DECODE, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_NORM, MF_UNCHECKED);
					Norm_Flag = false;
					EnableMenuItem(GetSubMenu(hMenu, 3), 1, MF_BYPOSITION | MF_GRAYED);
					EnableMenuItem(GetSubMenu(hMenu, 3), 3, MF_BYPOSITION | MF_GRAYED);
					EnableMenuItem(GetSubMenu(hMenu, 3), 4, MF_BYPOSITION | MF_GRAYED);
					EnableMenuItem(GetSubMenu(hMenu, 3), 5, MF_BYPOSITION | MF_GRAYED);
					break;

				case IDM_DECODE:
					Method_Flag = AUDIO_DECODE;
					CheckMenuItem(hMenu, IDM_AUDIO_NONE, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_DEMUX, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_DEMUXALL, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_DECODE, MF_CHECKED);
					EnableMenuItem(GetSubMenu(hMenu, 3), 1, MF_BYPOSITION | MF_ENABLED);
					EnableMenuItem(GetSubMenu(hMenu, 3), 3, MF_BYPOSITION | MF_ENABLED);
					EnableMenuItem(GetSubMenu(hMenu, 3), 4, MF_BYPOSITION | MF_ENABLED);
					EnableMenuItem(GetSubMenu(hMenu, 3), 5, MF_BYPOSITION | MF_ENABLED);
					break;

				case IDM_DRC_NONE:
					DRC_Flag = DRC_NONE;
					CheckMenuItem(hMenu, IDM_DRC_NONE, MF_CHECKED);
					CheckMenuItem(hMenu, IDM_DRC_LIGHT, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_DRC_NORMAL, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_DRC_HEAVY, MF_UNCHECKED);
					break;

				case IDM_DRC_LIGHT:
					DRC_Flag = DRC_LIGHT;
					CheckMenuItem(hMenu, IDM_DRC_NONE, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_DRC_LIGHT, MF_CHECKED);
					CheckMenuItem(hMenu, IDM_DRC_NORMAL, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_DRC_HEAVY, MF_UNCHECKED);
					break;

				case IDM_DRC_NORMAL:
					DRC_Flag = DRC_NORMAL;
					CheckMenuItem(hMenu, IDM_DRC_NONE, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_DRC_LIGHT, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_DRC_NORMAL, MF_CHECKED);
					CheckMenuItem(hMenu, IDM_DRC_HEAVY, MF_UNCHECKED);
					break;

				case IDM_DRC_HEAVY:
					DRC_Flag = DRC_HEAVY;
					CheckMenuItem(hMenu, IDM_DRC_NONE, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_DRC_LIGHT, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_DRC_NORMAL, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_DRC_HEAVY, MF_CHECKED);
					break;

				case IDM_DSDOWN:
					if (DSDown_Flag)
						CheckMenuItem(hMenu, IDM_DSDOWN, MF_UNCHECKED);
					else
						CheckMenuItem(hMenu, IDM_DSDOWN, MF_CHECKED);

					DSDown_Flag = !DSDown_Flag;
					break;

				case IDM_PRESCALE:
					if (PreScale_Ratio==1.0 && Check_Flag && Method_Flag!=AUDIO_DEMUXALL && IsWindowEnabled(hTrack))
					{
						Decision_Flag = true;
						Display_Flag = false;
						RunningEnables();
						ShowInfo(true);

						process.locate = LOCATE_RIP;
						PreScale_Ratio = 1.0;

						if (WaitForSingleObject(hThread, INFINITE)==WAIT_OBJECT_0)
							hThread = CreateThread(NULL, 0, MPEG2Dec, 0, 0, &threadId);
					}
					else
					{
						CheckMenuItem(hMenu, IDM_PRESCALE, MF_UNCHECKED);
						PreScale_Ratio = 1.0;
					}
					break;

				case IDM_DETECT_PIDS_RAW:
					Pid_Detect_Method = PID_DETECT_RAW;
					DialogBox(hInst, (LPCTSTR)IDD_DETECT_PIDS, hWnd, (DLGPROC) DetectPids);
					break;

				case IDM_DETECT_PIDS:
					Pid_Detect_Method = PID_DETECT_PATPMT;
					DialogBox(hInst, (LPCTSTR)IDD_DETECT_PIDS, hWnd, (DLGPROC) DetectPids);
					break;

				case IDM_DETECT_PIDS_PSIP:
					Pid_Detect_Method = PID_DETECT_PSIP;
					DialogBox(hInst, (LPCTSTR)IDD_DETECT_PIDS, hWnd, (DLGPROC) DetectPids);
					break;

				case IDM_SET_PIDS:
					DialogBox(hInst, (LPCTSTR)IDD_SET_PIDS, hWnd, (DLGPROC) SetPids);
					break;

				case IDM_IDCT_MMX:
					iDCT_Flag = IDCT_MMX;
					CheckMenuItem(hMenu, IDM_IDCT_MMX, MF_CHECKED);
					CheckMenuItem(hMenu, IDM_IDCT_SSEMMX, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_IDCT_SSE2MMX, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_IDCT_FPU, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_IDCT_REF, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_IDCT_SKAL, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_IDCT_SIMPLE, MF_UNCHECKED);
					break;

				case IDM_IDCT_SSEMMX:
					iDCT_Flag = IDCT_SSEMMX;
					CheckMenuItem(hMenu, IDM_IDCT_MMX, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_IDCT_SSEMMX, MF_CHECKED);
					CheckMenuItem(hMenu, IDM_IDCT_SSE2MMX, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_IDCT_FPU, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_IDCT_REF, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_IDCT_SKAL, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_IDCT_SIMPLE, MF_UNCHECKED);
					break;

				case IDM_IDCT_SSE2MMX:
					iDCT_Flag = IDCT_SSE2MMX;
					CheckMenuItem(hMenu, IDM_IDCT_MMX, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_IDCT_SSEMMX, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_IDCT_SSE2MMX, MF_CHECKED);
					CheckMenuItem(hMenu, IDM_IDCT_FPU, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_IDCT_REF, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_IDCT_SKAL, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_IDCT_SIMPLE, MF_UNCHECKED);
					break;

				case IDM_IDCT_FPU:
					iDCT_Flag = IDCT_FPU;
					CheckMenuItem(hMenu, IDM_IDCT_MMX, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_IDCT_SSEMMX, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_IDCT_SSE2MMX, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_IDCT_FPU, MF_CHECKED);
					CheckMenuItem(hMenu, IDM_IDCT_REF, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_IDCT_SKAL, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_IDCT_SIMPLE, MF_UNCHECKED);
					break;

				case IDM_IDCT_REF:
					iDCT_Flag = IDCT_REF;
					CheckMenuItem(hMenu, IDM_IDCT_MMX, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_IDCT_SSEMMX, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_IDCT_SSE2MMX, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_IDCT_FPU, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_IDCT_REF, MF_CHECKED);
					CheckMenuItem(hMenu, IDM_IDCT_SKAL, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_IDCT_SIMPLE, MF_UNCHECKED);
					break;

				case IDM_IDCT_SKAL:
					iDCT_Flag = IDCT_SKAL;
					CheckMenuItem(hMenu, IDM_IDCT_MMX, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_IDCT_SSEMMX, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_IDCT_SSE2MMX, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_IDCT_FPU, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_IDCT_REF, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_IDCT_SKAL, MF_CHECKED);
					CheckMenuItem(hMenu, IDM_IDCT_SIMPLE, MF_UNCHECKED);
					break;

				case IDM_IDCT_SIMPLE:
					iDCT_Flag = IDCT_SIMPLE;
					CheckMenuItem(hMenu, IDM_IDCT_MMX, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_IDCT_SSEMMX, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_IDCT_SSE2MMX, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_IDCT_FPU, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_IDCT_REF, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_IDCT_SKAL, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_IDCT_SIMPLE, MF_CHECKED);
					break;

				case IDM_FO_NONE:
					FO_Flag = FO_NONE;
					CheckMenuItem(hMenu, IDM_FO_NONE, MF_CHECKED);
					CheckMenuItem(hMenu, IDM_FO_FILM, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_FO_RAW, MF_UNCHECKED);
					SetDlgItemText(hDlg, IDC_INFO, "");
					break;

				case IDM_FO_FILM:
					FO_Flag = FO_FILM;
					CheckMenuItem(hMenu, IDM_FO_NONE, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_FO_FILM, MF_CHECKED);
					CheckMenuItem(hMenu, IDM_FO_RAW, MF_UNCHECKED);
					SetDlgItemText(hDlg, IDC_INFO, "");
					break;

				case IDM_FO_RAW:
					FO_Flag = FO_RAW;
					CheckMenuItem(hMenu, IDM_FO_NONE, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_FO_FILM, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_FO_RAW, MF_CHECKED);
					SetDlgItemText(hDlg, IDC_INFO, "");
					break;

				case IDM_TVSCALE:
					Scale_Flag = false;

					setRGBValues();

					CheckMenuItem(hMenu, IDM_TVSCALE, MF_CHECKED);
					CheckMenuItem(hMenu, IDM_PCSCALE, MF_UNCHECKED);

					RefreshWindow(true);
					break;

				case IDM_PCSCALE:
					Scale_Flag = true;

					setRGBValues();

					CheckMenuItem(hMenu, IDM_TVSCALE, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_PCSCALE, MF_CHECKED);

					RefreshWindow(true);
					break;

				case IDM_CROPPING:
					DialogBox(hInst, (LPCTSTR)IDD_CROPPING, hWnd, (DLGPROC) Cropping);
					break;

				case IDM_LUMINANCE:
					DialogBox(hInst, (LPCTSTR)IDD_LUMINANCE, hWnd, (DLGPROC) Luminance);
					break;

				case IDM_NORM:
					DialogBox(hInst, (LPCTSTR)IDD_NORM, hWnd, (DLGPROC) Normalization);
					break;

				case IDM_TRACK_NUMBER:
					DialogBox(hInst, (LPCTSTR)IDD_SELECT_TRACKS, hWnd, (DLGPROC) SelectTracks);
					break;

				case IDM_ANALYZESYNC:
					DialogBox(hInst, (LPCTSTR)IDD_SELECT_DELAY_TRACK, hWnd, (DLGPROC) SelectDelayTrack);
					break;

				case IDM_SPEED_SINGLE_STEP:
					PlaybackSpeed = SPEED_SINGLE_STEP;
					SetDlgItemText(hDlg, IDC_FPS, "");
					if (process.locate == LOCATE_RIP) EnableWindow(hRightArrow, true);
					CheckMenuItem(hMenu, IDM_SPEED_SINGLE_STEP, MF_CHECKED);
					CheckMenuItem(hMenu, IDM_SPEED_SUPER_SLOW, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_SPEED_SLOW, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_SPEED_NORMAL, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_SPEED_FAST, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_SPEED_MAXIMUM, MF_UNCHECKED);
					break;

				case IDM_SPEED_SUPER_SLOW:
					PlaybackSpeed = SPEED_SUPER_SLOW;
					// Cancel wait for single-step.
					RightArrowHit = true;
					SetDlgItemText(hDlg, IDC_FPS, "");
					if (process.locate == LOCATE_RIP) EnableWindow(hRightArrow, false);
					CheckMenuItem(hMenu, IDM_SPEED_SINGLE_STEP, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_SPEED_SUPER_SLOW, MF_CHECKED);
					CheckMenuItem(hMenu, IDM_SPEED_SLOW, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_SPEED_NORMAL, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_SPEED_FAST, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_SPEED_MAXIMUM, MF_UNCHECKED);
					break;

				case IDM_SPEED_SLOW:
					PlaybackSpeed = SPEED_SLOW;
					// Cancel wait for single-step.
					RightArrowHit = true;
					SetDlgItemText(hDlg, IDC_FPS, "");
					if (process.locate == LOCATE_RIP) EnableWindow(hRightArrow, false);
					CheckMenuItem(hMenu, IDM_SPEED_SINGLE_STEP, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_SPEED_SUPER_SLOW, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_SPEED_SLOW, MF_CHECKED);
					CheckMenuItem(hMenu, IDM_SPEED_NORMAL, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_SPEED_FAST, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_SPEED_MAXIMUM, MF_UNCHECKED);
					break;

				case IDM_SPEED_NORMAL:
					PlaybackSpeed = SPEED_NORMAL;
					// Cancel wait for single-step.
					RightArrowHit = true;
					SetDlgItemText(hDlg, IDC_FPS, "");
					if (process.locate == LOCATE_RIP) EnableWindow(hRightArrow, false);
					CheckMenuItem(hMenu, IDM_SPEED_SINGLE_STEP, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_SPEED_SUPER_SLOW, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_SPEED_SLOW, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_SPEED_NORMAL, MF_CHECKED);
					CheckMenuItem(hMenu, IDM_SPEED_FAST, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_SPEED_MAXIMUM, MF_UNCHECKED);
					break;

				case IDM_SPEED_FAST:
					PlaybackSpeed = SPEED_FAST;
					// Cancel wait for single-step.
					RightArrowHit = true;
					SetDlgItemText(hDlg, IDC_FPS, "");
					if (process.locate == LOCATE_RIP) EnableWindow(hRightArrow, false);
					CheckMenuItem(hMenu, IDM_SPEED_SINGLE_STEP, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_SPEED_SUPER_SLOW, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_SPEED_SLOW, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_SPEED_NORMAL, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_SPEED_FAST, MF_CHECKED);
					CheckMenuItem(hMenu, IDM_SPEED_MAXIMUM, MF_UNCHECKED);
					break;

				case IDM_SPEED_MAXIMUM:
					PlaybackSpeed = SPEED_MAXIMUM;
					// Cancel wait for single-step.
					RightArrowHit = true;
					SetDlgItemText(hDlg, IDC_FPS, "");
					if (process.locate == LOCATE_RIP) EnableWindow(hRightArrow, false);
					CheckMenuItem(hMenu, IDM_SPEED_SINGLE_STEP, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_SPEED_SUPER_SLOW, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_SPEED_SLOW, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_SPEED_NORMAL, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_SPEED_FAST, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_SPEED_MAXIMUM, MF_CHECKED);
					break;

				case IDM_FULL_SIZED:
					HDDisplay = HD_DISPLAY_FULL_SIZED;
					CheckMenuItem(hMenu, IDM_FULL_SIZED, MF_CHECKED);
					CheckMenuItem(hMenu, IDM_SHRINK_BY_HALF, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_TOP_LEFT, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_TOP_RIGHT, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_BOTTOM_LEFT, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_BOTTOM_RIGHT, MF_UNCHECKED);
                    RefreshWindow(TRUE);
					break;

				case IDM_SHRINK_BY_HALF:
					HDDisplay = HD_DISPLAY_SHRINK_BY_HALF;
					CheckMenuItem(hMenu, IDM_FULL_SIZED, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_SHRINK_BY_HALF, MF_CHECKED);
					CheckMenuItem(hMenu, IDM_TOP_LEFT, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_TOP_RIGHT, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_BOTTOM_LEFT, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_BOTTOM_RIGHT, MF_UNCHECKED);
                    RefreshWindow(TRUE);
					break;

				case IDM_TOP_LEFT:
					HDDisplay = HD_DISPLAY_TOP_LEFT;
					CheckMenuItem(hMenu, IDM_FULL_SIZED, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_SHRINK_BY_HALF, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_TOP_LEFT, MF_CHECKED);
					CheckMenuItem(hMenu, IDM_TOP_RIGHT, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_BOTTOM_LEFT, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_BOTTOM_RIGHT, MF_UNCHECKED);
                    RefreshWindow(TRUE);
					break;

				case IDM_TOP_RIGHT:
					HDDisplay = HD_DISPLAY_TOP_RIGHT;
					CheckMenuItem(hMenu, IDM_FULL_SIZED, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_SHRINK_BY_HALF, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_TOP_LEFT, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_TOP_RIGHT, MF_CHECKED);
					CheckMenuItem(hMenu, IDM_BOTTOM_LEFT, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_BOTTOM_RIGHT, MF_UNCHECKED);
                    RefreshWindow(TRUE);
					break;

				case IDM_BOTTOM_LEFT:
					HDDisplay = HD_DISPLAY_BOTTOM_LEFT;
					CheckMenuItem(hMenu, IDM_FULL_SIZED, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_SHRINK_BY_HALF, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_TOP_LEFT, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_TOP_RIGHT, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_BOTTOM_LEFT, MF_CHECKED);
					CheckMenuItem(hMenu, IDM_BOTTOM_RIGHT, MF_UNCHECKED);
                    RefreshWindow(TRUE);
					break;

				case IDM_BOTTOM_RIGHT:
					HDDisplay = HD_DISPLAY_BOTTOM_RIGHT;
					CheckMenuItem(hMenu, IDM_FULL_SIZED, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_SHRINK_BY_HALF, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_TOP_LEFT, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_TOP_RIGHT, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_BOTTOM_LEFT, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_BOTTOM_RIGHT, MF_CHECKED);
                    RefreshWindow(TRUE);
					break;

				case IDM_PP_HIGH:
					Priority_Flag = PRIORITY_HIGH;
					SetPriorityClass(hProcess, HIGH_PRIORITY_CLASS);
					CheckMenuItem(hMenu, IDM_PP_HIGH, MF_CHECKED);
					CheckMenuItem(hMenu, IDM_PP_NORMAL, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_PP_LOW, MF_UNCHECKED);
					break;

				case IDM_PP_NORMAL:
					Priority_Flag = PRIORITY_NORMAL;
					SetPriorityClass(hProcess, NORMAL_PRIORITY_CLASS);
					CheckMenuItem(hMenu, IDM_PP_HIGH, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_PP_NORMAL, MF_CHECKED);
					CheckMenuItem(hMenu, IDM_PP_LOW, MF_UNCHECKED);
					break;

				case IDM_PP_LOW:
					Priority_Flag = PRIORITY_LOW;
					SetPriorityClass(hProcess, IDLE_PRIORITY_CLASS);
					CheckMenuItem(hMenu, IDM_PP_HIGH, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_PP_NORMAL, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_PP_LOW, MF_CHECKED);
					break;

				case IDM_AVS_TEMPLATE:
					DialogBox(hInst, (LPCTSTR)IDD_AVS_TEMPLATE, hWnd, (DLGPROC) AVSTemplate);
					break;

				case IDM_BMP_PATH:
					DialogBox(hInst, (LPCTSTR)IDD_BMP_PATH, hWnd, (DLGPROC) BMPPath);
					break;

				case IDM_FORCE_OPEN:
					ForceOpenGops ^= 1;
					if (ForceOpenGops)
						CheckMenuItem(hMenu, IDM_FORCE_OPEN, MF_CHECKED);
					else
						CheckMenuItem(hMenu, IDM_FORCE_OPEN, MF_UNCHECKED);
					break;

                case IDM_FULL_PATH:
                    FullPathInFiles ^= 1;
                    if (FullPathInFiles)
                        CheckMenuItem(hMenu, IDM_FULL_PATH, MF_CHECKED);
                    else
                        CheckMenuItem(hMenu, IDM_FULL_PATH, MF_UNCHECKED);
                    break;

                case IDM_LOOP_PLAYBACK:
                    LoopPlayback ^= 1;
                    if (LoopPlayback)
                        CheckMenuItem(hMenu, IDM_LOOP_PLAYBACK, MF_CHECKED);
                    else
                        CheckMenuItem(hMenu, IDM_LOOP_PLAYBACK, MF_UNCHECKED);
                    break;

                case IDM_FUSION_AUDIO:
                    FusionAudio ^= 1;
                    if (FusionAudio)
                        CheckMenuItem(hMenu, IDM_FUSION_AUDIO, MF_CHECKED);
                    else
                        CheckMenuItem(hMenu, IDM_FUSION_AUDIO, MF_UNCHECKED);
                    break;

				case IDM_PAUSE:
					if (Pause_Flag)
					{
						// Forces restart of speed control algorithm upon resumption.
						OldPlaybackSpeed = -1;
						ResumeThread(hThread);
					}
					else
						SuspendThread(hThread);

					Pause_Flag = !Pause_Flag;
					break;

				case IDM_BMP:
					SaveBMP();
					break;

				case IDM_SRC_NONE:
					SRC_Flag = SRC_NONE;
					CheckMenuItem(hMenu, IDM_SRC_NONE, MF_CHECKED);
					CheckMenuItem(hMenu, IDM_SRC_LOW, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_SRC_MID, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_SRC_HIGH, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_SRC_UHIGH, MF_UNCHECKED);
					break;

				case IDM_SRC_LOW:
					SRC_Flag = SRC_LOW;
					CheckMenuItem(hMenu, IDM_SRC_NONE, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_SRC_LOW, MF_CHECKED);
					CheckMenuItem(hMenu, IDM_SRC_MID, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_SRC_HIGH, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_SRC_UHIGH, MF_UNCHECKED);
					break;

				case IDM_SRC_MID:
					SRC_Flag = SRC_MID;
					CheckMenuItem(hMenu, IDM_SRC_NONE, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_SRC_LOW, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_SRC_MID, MF_CHECKED);
					CheckMenuItem(hMenu, IDM_SRC_HIGH, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_SRC_UHIGH, MF_UNCHECKED);
					break;

				case IDM_SRC_HIGH:
					SRC_Flag = SRC_HIGH;
					CheckMenuItem(hMenu, IDM_SRC_NONE, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_SRC_LOW, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_SRC_MID, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_SRC_HIGH, MF_CHECKED);
					CheckMenuItem(hMenu, IDM_SRC_UHIGH, MF_UNCHECKED);
					break;

				case IDM_SRC_UHIGH:
					SRC_Flag = SRC_UHIGH;
					CheckMenuItem(hMenu, IDM_SRC_NONE, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_SRC_LOW, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_SRC_MID, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_SRC_HIGH, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_SRC_UHIGH, MF_CHECKED);
					break;

				case IDM_COPYFRAMETOCLIPBOARD:
					CopyBMP();
					break;

				case IDM_ABOUT:
					DialogBox(hInst, (LPCTSTR)IDD_ABOUT, hWnd, (DLGPROC)About);
					break;

				case IDM_QUICK_START:
				case IDM_DGINDEX_MANUAL:
				case IDM_DGDECODE_MANUAL:
				{
					strcpy(prog, ExePath);
					if (wmId == IDM_QUICK_START)
						strcat(prog, "QuickStart.html");
					else if (wmId == IDM_DGINDEX_MANUAL)
						strcat(prog, "DGIndexManual.html");
					else
						strcat(prog, "DGDecodeManual.html");
					ShellExecute(NULL, "open", prog, NULL, NULL, SW_SHOWNORMAL);
					break;
				}

				case IDM_NEURON2:
					ShellExecute(NULL, "open", "http://rationalqm.us/dgmpgdec/dgmpgdec.html", NULL, NULL, SW_SHOWNORMAL);
					break;

				case IDM_EXIT:
					DestroyWindow(hWnd);
					break;

				case ID_LEFT_BUTTON:
					if (IsWindowEnabled(hTrack))
					{
						SetFocus(hWnd);
						if (Info_Flag)
						{
							DestroyWindow(hDlg);
							Info_Flag = false;
						}
//						if ((process.file < process.rightfile) || (process.file==process.rightfile && process.lba<process.rightlba))
						{
							process.leftfile = process.file;
							process.leftlba = process.lba;

							process.run = 0;
							for (i=0; i<process.leftfile; i++)
								process.run += Infilelength[i];
							process.trackleft = ((process.run + process.leftlba * SECTOR_SIZE) * TRACK_PITCH / Infiletotal);

							SendMessage(hTrack, TBM_SETPOS, (WPARAM) true, (LONG) process.trackleft);
	                        InvalidateRect(hwndSelect, NULL, TRUE);
//							SendMessage(hTrack, TBM_SETSEL, (WPARAM) true, (LPARAM) MAKELONG(process.trackleft, process.trackright));
						}
						InvalidateRect(hwndSelect, NULL, TRUE);
					}
					break;

				case ID_LEFT_ARROW:
left_arrow:
					SetFocus(hWnd);

					InvalidateRect(hwndSelect, NULL, TRUE);
					if (Info_Flag)
					{
						DestroyWindow(hDlg);
						Info_Flag = false;
					}
					if (WaitForSingleObject(hThread, 0)==WAIT_OBJECT_0)
					{
						Display_Flag = true;

						process.locate = LOCATE_BACKWARD;
						hThread = CreateThread(NULL, 0, MPEG2Dec, 0, 0, &threadId);
					}
					break;

				case ID_RIGHT_ARROW:
right_arrow:
					SetFocus(hWnd);
					if (process.locate == LOCATE_RIP)
					{
						// We are in play/preview mode. Signal the
						// display process to step forward one frame.
						if (PlaybackSpeed == SPEED_SINGLE_STEP)
							RightArrowHit = true;
						break;
					}

					InvalidateRect(hwndSelect, NULL, TRUE);
					if (Info_Flag)
					{
						DestroyWindow(hDlg);
						Info_Flag = false;
					}
					if (WaitForSingleObject(hThread, 0)==WAIT_OBJECT_0)
					{
						Display_Flag = true;

						process.locate = LOCATE_FORWARD;
						hThread = CreateThread(NULL, 0, MPEG2Dec, 0, 0, &threadId);
					}
					break;

				case ID_RIGHT_BUTTON:
					if (IsWindowEnabled(hTrack))
					{
						SetFocus(hWnd);
						if (Info_Flag)
						{
							DestroyWindow(hDlg);
							Info_Flag = false;
						}
//						if ((process.file>process.leftfile) || (process.file==process.leftfile && process.lba>process.leftlba))
						{
							process.rightfile = process.file;
							process.rightlba = process.lba;

							process.run = 0;
							for (i=0; i<process.rightfile; i++)
								process.run += Infilelength[i];
							process.trackright = ((process.run + (__int64)process.rightlba*SECTOR_SIZE)*TRACK_PITCH/Infiletotal);

							SendMessage(hTrack, TBM_SETPOS, (WPARAM) true, (LONG) process.trackright);
                        	InvalidateRect(hwndSelect, NULL, TRUE);
//							SendMessage(hTrack, TBM_SETSEL, (WPARAM) true, (LPARAM) MAKELONG(process.trackleft, process.trackright));
						}
						InvalidateRect(hwndSelect, NULL, TRUE);
					}
					break;

				default:
					return DefWindowProc(hWnd, message, wParam, lParam);
			}
			break;

		case WM_MOUSEWHEEL:
			wmEvent = HIWORD(wParam);
			if ((short) wmEvent < 0)
				goto right_arrow;
			else
				goto left_arrow;
			break;

        case WM_HSCROLL:
			SetFocus(hWnd);

			InvalidateRect(hwndSelect, NULL, TRUE);
			if (Info_Flag)
			{
				DestroyWindow(hDlg);
				Info_Flag = false;
			}
			if (WaitForSingleObject(hThread, 0)==WAIT_OBJECT_0)
			{
				int trackpos;

				Display_Flag = true;

				trackpos = SendMessage(hTrack, TBM_GETPOS, 0, 0);
				process.startloc = process.start = Infiletotal*trackpos/TRACK_PITCH;

				process.startfile = 0;
				process.run = 0;
				while (process.startloc > Infilelength[process.startfile])
				{
					process.startloc -= Infilelength[process.startfile];
					process.run += Infilelength[process.startfile];
					process.startfile++;
				}

				process.end = Infiletotal - SECTOR_SIZE;
				process.endfile = NumLoadedFiles - 1;
				process.endloc = (Infilelength[NumLoadedFiles-1]/SECTOR_SIZE-1)*SECTOR_SIZE;

				process.locate = LOCATE_SCROLL;

				hThread = CreateThread(NULL, 0, MPEG2Dec, 0, 0, &threadId);
			}
			break;
        }

		case WM_KEYDOWN:
			switch (wParam)
			{
				case VK_HOME:
					if (IsWindowEnabled(hTrack))
					{
						if (Info_Flag)
						{
							DestroyWindow(hDlg);
							Info_Flag = false;
						}
						SendMessage(hWnd, WM_COMMAND, MAKEWPARAM(ID_LEFT_BUTTON, 0), (LPARAM) 0);
					}
					break;

				case VK_END:
					if (IsWindowEnabled(hTrack))
					{
						if (Info_Flag)
						{
							DestroyWindow(hDlg);
							Info_Flag = false;
						}
						SendMessage(hWnd, WM_COMMAND, MAKEWPARAM(ID_RIGHT_BUTTON, 0), (LPARAM) 0);
					}
					break;

				case VK_LEFT:
					if (IsWindowEnabled(hTrack))
					{
						if (Info_Flag)
						{
							DestroyWindow(hDlg);
							Info_Flag = false;
						}
						if (WaitForSingleObject(hThread, 0)==WAIT_OBJECT_0)
						{
							Display_Flag = true;

							process.locate = LOCATE_BACKWARD;
							hThread = CreateThread(NULL, 0, MPEG2Dec, 0, 0, &threadId);
						}
					}
					break;

				case VK_RIGHT:
					if (process.locate == LOCATE_RIP)
					{
						// We are in play/preview mode. Signal the
						// display process to step forward one frame.
						if (PlaybackSpeed == SPEED_SINGLE_STEP)
							RightArrowHit = true;
						break;
					}
					if (IsWindowEnabled(hTrack))
					{
						if (Info_Flag)
						{
							DestroyWindow(hDlg);
							Info_Flag = false;
						}
						if (WaitForSingleObject(hThread, 0)==WAIT_OBJECT_0)
						{
							Display_Flag = true;

							process.locate = LOCATE_FORWARD;
							hThread = CreateThread(NULL, 0, MPEG2Dec, 0, 0, &threadId);
						}
					}
					break;
			}
			break;

		case WM_SIZE:
			if (!IsIconic(hWnd))
			{
				ShowInfo(false);
				RefreshWindow(true);
			}
			break;

		case WM_MOVE:
			if (!IsIconic(hWnd))
			{
	            GetWindowRect(hWnd, &wrect);
				RefreshWindow(false);
			}
			break;

		case WM_PAINT:
			{
			BITMAP bm;
			HDC hdcMem;
			HBITMAP hbmOld;

			hdc = BeginPaint(hWnd, &ps);
			// Paint the splash screen if no files are loaded.
			if (splash && NumLoadedFiles == 0)
			{
				hdcMem = CreateCompatibleDC(hdc);
				hbmOld = (HBITMAP) SelectObject(hdcMem, splash);
				GetObject(splash, sizeof(bm), &bm);
				BitBlt(hdc, 0, 0, bm.bmWidth, bm.bmHeight, hdcMem, 0, 0, SRCCOPY);
				SelectObject(hdcMem, hbmOld);
				DeleteDC(hdcMem);
			}
			EndPaint(hWnd, &ps);
			ReleaseDC(hWnd, hdc);
			RefreshWindow(false);
			break;
			}

		case WM_DROPFILES:
			char *ext, *tmp;
			int drop_count, drop_index;
			int n;

			if (Info_Flag)
			{
				DestroyWindow(hDlg);
				Info_Flag = false;
			}

			DragQueryFile((HDROP)wParam, 0, szInput, sizeof(szInput));
			SetForegroundWindow(hWnd);

			// Set the output directory for a Save D2V operation to the
			// same path as these input files.
			strcpy(path, szInput);
			tmp = path + strlen(path);
			while (*tmp != '\\' && tmp >= path) tmp--;
			tmp[1] = 0;
			strcpy(szSave, path);

			ext = strrchr(szInput, '.');
			if (ext!=NULL)
			{
				if (!_strnicmp(ext, ".d2v", 4))
				{
					DragFinish((HDROP)wParam);
					goto D2V_PROCESS;
				}
			}

			while (NumLoadedFiles)
			{
				NumLoadedFiles--;
				fclose(Infile[NumLoadedFiles]);
                Infile[NumLoadedFiles] = NULL;
			}

			drop_count = DragQueryFile((HDROP)wParam, 0xffffffff, szInput, sizeof(szInput));
			for (drop_index = 0; drop_index < drop_count; drop_index++)
			{
				DragQueryFile((HDROP)wParam, drop_index, szInput, sizeof(szInput));
				struct _finddata_t seqfile;
				if (_findfirst(szInput, &seqfile) != -1L)
				{
					strcpy(Infilename[NumLoadedFiles], szInput);
					NumLoadedFiles++;
					SystemStream_Flag = ELEMENTARY_STREAM;
				}
			}
			DragFinish((HDROP)wParam);
			// Sort the filenames.
			// This is a special sort designed to do things intelligently
			// for typical sequentially numbered filenames.
			// Sorry, just a bubble sort. No need for performance here. KISS.
			n = NumLoadedFiles;
			for (i = 0; i < n - 1; i++)
			{
				for (j = 0; j < n - 1 - i; j++)
				{
					if (strverscmp(Infilename[j+1], Infilename[j]) < 0)
					{
						tmp = Infilename[j];
						Infilename[j] = Infilename[j+1];
						Infilename[j+1] = tmp;
					}
				}
			}
			// Open the files.
			for (i = 0; i < NumLoadedFiles; i++)
			{
				Infile[i] = fopen(Infilename[i], "rb");
			}
			DialogBox(hInst, (LPCTSTR)IDD_FILELIST, hWnd, (DLGPROC)VideoList);
			break;

		case WM_DESTROY:
            Stop_Flag = 1;
            WaitForSingleObject(hThread, 2000);
            strcpy(prog, ExePath);
			strcat(prog, "DGIndex.ini");
			if ((INIFile = fopen(prog, "w")) != NULL)
			{
				fprintf(INIFile, "DGIndex_Version=%s\n", Version);
				fprintf(INIFile, "Window_Position=%d,%d\n", wrect.left, wrect.top);
				fprintf(INIFile, "Info_Window_Position=%d,%d\n", info_wrect.left, info_wrect.top);
				fprintf(INIFile, "iDCT_Algorithm=%d\n", iDCT_Flag);
				fprintf(INIFile, "YUVRGB_Scale=%d\n", Scale_Flag);
				fprintf(INIFile, "Field_Operation=%d\n", FO_Flag);
				fprintf(INIFile, "Output_Method=%d\n", Method_Flag);
				fprintf(INIFile, "Track_List=%s\n", Track_List);
				fprintf(INIFile, "DR_Control=%d\n", DRC_Flag);
				fprintf(INIFile, "DS_Downmix=%d\n", DSDown_Flag);
				fprintf(INIFile, "SRC_Precision=%d\n", SRC_Flag);
				fprintf(INIFile, "Norm_Ratio=%d\n", 100 * Norm_Flag + Norm_Ratio);
				fprintf(INIFile, "Process_Priority=%d\n", Priority_Flag);
				fprintf(INIFile, "Playback_Speed=%d\n", PlaybackSpeed);
				fprintf(INIFile, "Force_Open_Gops=%d\n", ForceOpenGops);
				fprintf(INIFile, "AVS_Template_Path=%s\n", AVSTemplatePath);
				fprintf(INIFile, "Full_Path_In_Files=%d\n", FullPathInFiles);
				fprintf(INIFile, "Fusion_Audio=%d\n", FusionAudio);
				fprintf(INIFile, "Loop_Playback=%d\n", LoopPlayback);
				fprintf(INIFile, "HD_Display=%d\n", HDDisplay);
                for (i = 0; i < 4; i++)
                {
    				fprintf(INIFile, "MRUList[%d]=%s\n", i, mMRUList[i]);
                }
				fprintf(INIFile, "Enable_Info_Log=%d\n", InfoLog_Flag);
				fprintf(INIFile, "BMP_Path=%s\n", BMPPathString);
				fprintf(INIFile, "Use_MPA_Extensions=%d\n", UseMPAExtensions);
				fprintf(INIFile, "Notify_When_Done=%d\n", NotifyWhenDone);
				fclose(INIFile);
			}

			while (NumLoadedFiles)
			{
				NumLoadedFiles--;
				fclose(Infile[NumLoadedFiles]);
               Infile[NumLoadedFiles] = NULL;
			}

			Recovery();

			for (i=0; i<8; i++)
				_aligned_free(block[i]);

			for (i=0; i<MAX_FILE_NUMBER; i++)
				free(Infilename[i]);

            free(Rdbfr);

	        DeleteObject(splash);
			ReleaseDC(hWnd, hDC);
			PostQuitMessage(0);
			break;

		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return false;
}

LRESULT CALLBACK SelectProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  HBRUSH hBrush, hLine;
  HPEN hPenBrush, hPenLine;
  PAINTSTRUCT ps;
  RECT rect;
  HDC hdc;

  switch(msg)  
  {
	int left, right, trackpos;

	case WM_PAINT:
		left = (int) process.trackleft;
		right = (int) process.trackright;
		hdc = BeginPaint(hwnd, &ps);
		GetClientRect(hwnd, &rect);
        hPenBrush = CreatePen(PS_NULL, 1, RGB(150, 200, 255));        
        hPenLine = CreatePen(PS_NULL, 1, RGB(0, 0, 0));        
		hBrush = CreateSolidBrush(RGB(150, 200, 255));
		hLine = CreateSolidBrush(RGB(0, 0, 0));
		SelectObject(hdc, hBrush);
		SelectObject(hdc, hPenBrush);
		left = (int) ((rect.right * left) / TRACK_PITCH);
		right = (int) ((rect.right * right) / TRACK_PITCH + 1);
		if (right - left < 2)
			right = left + 2;
		Rectangle(hdc, left, 0, right, TRACK_HEIGHT/3);            
		trackpos = SendMessage(hTrack, TBM_GETPOS, 0, 0);
		left = ((rect.right * trackpos) / TRACK_PITCH);
		if (left < 0)
		{
			left = 0;
		}
		if (right > rect.right)
		{
			right = rect.right;
		}
		right = left + 2;
		SelectObject(hdc, hLine);
		SelectObject(hdc, hPenLine);
		Rectangle(hdc, left, 0, (int) right, TRACK_HEIGHT/3);            
        DeleteObject(hPenBrush);          
        DeleteObject(hBrush);          
		DeleteObject(hPenLine);          
        DeleteObject(hLine);          
		EndPaint(hwnd, &ps);
		break;
  }
  return DefWindowProc(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK DetectPids(HWND hDialog, UINT message, WPARAM wParam, LPARAM lParam)
{
	char msg[255];

	switch (message)
	{
		case WM_INITDIALOG:
			if (SystemStream_Flag != TRANSPORT_STREAM)
			{
				sprintf(msg, "Not a transport stream!");
				SendDlgItemMessage(hDialog, IDC_PID_LISTBOX, LB_ADDSTRING, 0, (LPARAM)msg);
			}
			else if (Pid_Detect_Method == PID_DETECT_RAW)
			{
				pat_parser.DumpRaw(hDialog, Infilename[0]);
			}
			else if (Pid_Detect_Method == PID_DETECT_PSIP && pat_parser.DumpPSIP(hDialog, Infilename[0]) == 1)
			{
				sprintf(msg, "Could not find PSIP tables!");
				SendDlgItemMessage(hDialog, IDC_PID_LISTBOX, LB_ADDSTRING, 0, (LPARAM)msg);
			}
			else if (Pid_Detect_Method == PID_DETECT_PATPMT && pat_parser.DumpPAT(hDialog, Infilename[0]) == 1)
			{
				sprintf(msg, "Could not find PAT/PMT tables!");
				SendDlgItemMessage(hDialog, IDC_PID_LISTBOX, LB_ADDSTRING, 0, (LPARAM)msg);
			}
			return true;

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				int item;
				char text[80], *ptr;

				case IDC_SET_AUDIO:
				case IDC_SET_VIDEO:
				case IDC_SET_PCR:
					item = SendDlgItemMessage(hDialog, IDC_PID_LISTBOX, (UINT) LB_GETCURSEL, 0, 0);
					if (item != LB_ERR)
					{
						SendDlgItemMessage(hDialog, IDC_PID_LISTBOX, (UINT) LB_GETTEXT, item, (LPARAM) text);
						if ((ptr = strstr(text, "0x")) != NULL)
						{
							if (LOWORD(wParam) == IDC_SET_AUDIO)
								sscanf(ptr, "%x", &MPEG2_Transport_AudioPID);
							else if (LOWORD(wParam) == IDC_SET_VIDEO)
								sscanf(ptr, "%x", &MPEG2_Transport_VideoPID);
							else
								sscanf(ptr, "%x", &MPEG2_Transport_PCRPID);
						}
					}
					if (LOWORD(wParam) == IDC_SET_AUDIO || LOWORD(wParam) == IDC_SET_PCR) break;
					Recovery();
					if (NumLoadedFiles)
					{
						FileLoadedEnables();
						process.rightfile = NumLoadedFiles-1;
						process.rightlba = (int)(Infilelength[NumLoadedFiles-1]/SECTOR_SIZE);

						process.end = Infiletotal - SECTOR_SIZE;
						process.endfile = NumLoadedFiles - 1;
						process.endloc = (Infilelength[NumLoadedFiles-1]/SECTOR_SIZE - 1)*SECTOR_SIZE;

						process.locate = LOCATE_INIT;

						if (!threadId || WaitForSingleObject(hThread, INFINITE)==WAIT_OBJECT_0)
							hThread = CreateThread(NULL, 0, MPEG2Dec, 0, 0, &threadId);
					}
					break;

				case IDC_SET_DONE:
				case IDCANCEL:
					EndDialog(hDialog, 0);
			}
			return true;
	}
    return false;
}

LRESULT CALLBACK VideoList(HWND hVideoListDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	int i, j;
	char updown[DG_MAX_PATH];
	char *name;
	FILE *handle;

	switch (message)
	{
		case WM_INITDIALOG:
            HadAddDialog = 0;
			SendDlgItemMessage(hVideoListDlg, IDC_LIST, LB_SETHORIZONTALEXTENT, (WPARAM) 1024, 0);  
			if (NumLoadedFiles)
				for (i=0; i<NumLoadedFiles; i++)
					SendDlgItemMessage(hVideoListDlg, IDC_LIST, LB_ADDSTRING, 0, (LPARAM)Infilename[i]);
			else
				OpenVideoFile(hVideoListDlg);

			if (NumLoadedFiles)
				SendDlgItemMessage(hVideoListDlg, IDC_LIST, LB_SETCURSEL, NumLoadedFiles-1, 0);

			return true;

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case ID_ADD:
					OpenVideoFile(hVideoListDlg);

					if (NumLoadedFiles)
						SendDlgItemMessage(hVideoListDlg, IDC_LIST, LB_SETCURSEL, NumLoadedFiles-1, 0);
					break;

				case ID_UP:
					i = SendDlgItemMessage(hVideoListDlg, IDC_LIST, LB_GETCURSEL, 0, 0);
					if (i > 0)
					{
						name = Infilename[i];
						Infilename[i] = Infilename[i-1];
						Infilename[i-1] = name;
						handle = Infile[i];
						Infile[i] = Infile[i-1];
						Infile[i-1] = handle;
						SendDlgItemMessage(hVideoListDlg, IDC_LIST, LB_GETTEXT, i - 1, (LPARAM) updown);
						SendDlgItemMessage(hVideoListDlg, IDC_LIST, LB_DELETESTRING, i - 1, 0);
						SendDlgItemMessage(hVideoListDlg, IDC_LIST, LB_INSERTSTRING, i, (LPARAM) updown);
						SendDlgItemMessage(hVideoListDlg, IDC_LIST, LB_SETCURSEL, i - 1, 0);
					}
					break;

				case ID_DOWN:
					i = SendDlgItemMessage(hVideoListDlg, IDC_LIST, LB_GETCURSEL, 0, 0);
					j = SendDlgItemMessage(hVideoListDlg, IDC_LIST, LB_GETCOUNT, 0, 0);
					if (i < j - 1)
					{
						name = Infilename[i];
						Infilename[i] = Infilename[i+1];
						Infilename[i+1] = name;
						handle = Infile[i];
						Infile[i] = Infile[i+1];
						Infile[i+1] = handle;
						SendDlgItemMessage(hVideoListDlg, IDC_LIST, LB_GETTEXT, i, (LPARAM) updown);
						SendDlgItemMessage(hVideoListDlg, IDC_LIST, LB_DELETESTRING, i, 0);
						SendDlgItemMessage(hVideoListDlg, IDC_LIST, LB_INSERTSTRING, i + 1, (LPARAM) updown);
						SendDlgItemMessage(hVideoListDlg, IDC_LIST, LB_SETCURSEL, i + 1, 0);
					}
					break;

				case ID_DEL:
					if (NumLoadedFiles)
					{
						i= SendDlgItemMessage(hVideoListDlg, IDC_LIST, LB_GETCURSEL, 0, 0);
						SendDlgItemMessage(hVideoListDlg, IDC_LIST, LB_DELETESTRING, i, 0);
						NumLoadedFiles--;
						fclose(Infile[i]);
                        Infile[i] = NULL;
						for (j=i; j<NumLoadedFiles; j++)
						{
							Infile[j] = Infile[j+1];
							strcpy(Infilename[j], Infilename[j+1]);
						}
						SendDlgItemMessage(hVideoListDlg, IDC_LIST, LB_SETCURSEL, i>=NumLoadedFiles ? NumLoadedFiles-1 : i, 0);
					}
					if (!NumLoadedFiles)
					{
						Recovery();
						MPEG2_Transport_VideoPID = 2;
						MPEG2_Transport_AudioPID = 2;
						MPEG2_Transport_PCRPID = 2;
					}
					break;

				case ID_DELALL:
					while (NumLoadedFiles)
					{
						NumLoadedFiles--;
						i= SendDlgItemMessage(hVideoListDlg, IDC_LIST, LB_GETCURSEL, 0, 0);
						SendDlgItemMessage(hVideoListDlg, IDC_LIST, LB_DELETESTRING, i, 0);
						fclose(Infile[i]);
                        Infile[i] = NULL;
						SendDlgItemMessage(hVideoListDlg, IDC_LIST, LB_SETCURSEL, i>=NumLoadedFiles ? NumLoadedFiles-1 : i, 0);
					}
					Recovery();
					break;

				case IDOK:
				case IDCANCEL:
					EndDialog(hVideoListDlg, 0);
					Recovery();

					if (!HadAddDialog)
                    {
		                for (i = 0; i < NumLoadedFiles; i++)
		                {
			                if (Infile[i] != NULL)
                                fclose(Infile[i]);
                            Infile[i] = fopen(Infilename[i], "rb");
		                }
                    }
                    if (NumLoadedFiles)
					{
						FileLoadedEnables();
						process.rightfile = NumLoadedFiles-1;
						process.rightlba = (int)(Infilelength[NumLoadedFiles-1]/SECTOR_SIZE);

						process.end = Infiletotal - SECTOR_SIZE;
						process.endfile = NumLoadedFiles - 1;
						process.endloc = (Infilelength[NumLoadedFiles-1]/SECTOR_SIZE - 1)*SECTOR_SIZE;

						process.locate = LOCATE_INIT;

						if (!threadId || WaitForSingleObject(hThread, INFINITE)==WAIT_OBJECT_0)
							hThread = CreateThread(NULL, 0, MPEG2Dec, 0, 0, &threadId);
					}
					else
					{
						StartupEnables();
					}
					return true;
			}
			break;
	}
    return false;
}

static void OpenVideoFile(HWND hVideoListDlg)
{
	if (PopFileDlg(szInput, hVideoListDlg, OPEN_VOB))
	{
		char *p;
		char path[DG_MAX_PATH];
		char filename[DG_MAX_PATH];
		char curPath[DG_MAX_PATH];
		struct _finddata_t seqfile;
		int i, j, n;
		char *tmp;

		SystemStream_Flag = ELEMENTARY_STREAM;
		_getcwd(curPath, DG_MAX_PATH);
		if (strlen(curPath) != strlen(szInput))
		{
			// Only one file specified.
			if (_findfirst(szInput, &seqfile) == -1L) return;
			SendDlgItemMessage(hVideoListDlg, IDC_LIST, LB_ADDSTRING, 0, (LPARAM) szInput);
			strcpy(Infilename[NumLoadedFiles], szInput);
			Infile[NumLoadedFiles] = fopen(szInput, "rb");
			NumLoadedFiles++;
			// Set the output directory for a Save D2V operation to the
			// same path as this input files.
			strcpy(path, szInput);
			p = path + strlen(path);
			while (*p != '\\' && p >= path) p--;
			p[1] = 0;
			strcpy(szSave, path);
			return;
		}
		// Multi-select handling.
		// First clear existing file list box.
		n = NumLoadedFiles;
		while (n)
		{
			n--;
			i = SendDlgItemMessage(hVideoListDlg, IDC_LIST, LB_GETCURSEL, 0, 0);
			SendDlgItemMessage(hVideoListDlg, IDC_LIST, LB_DELETESTRING, i, 0);
			SendDlgItemMessage(hVideoListDlg, IDC_LIST, LB_SETCURSEL, i >= n ? n - 1 : i, 0);
		}
		// Save the path prefix (path without the filename).
		strcpy(path, szInput);
		// Also set that path as the default for a Save D2V operation.
		strcpy(szSave, szInput);
		// Add a trailing backslash if needed.
		p = szInput;
		while (*p != 0) p++;
		p--;
		if (*p != '\\')
			strcat(path, "\\");
		// Skip the path prefix.
		p = szInput;
		while (*p++ != 0);
		// Load the filenames.
		for (;;)
		{
			// Build full path plus filename.
			strcpy(filename, path);
			strcat(filename, p);
			if (_findfirst(filename, &seqfile) == -1L) break;
			strcpy(Infilename[NumLoadedFiles], filename);
			NumLoadedFiles++;
			// Skip to next filename.
			while (*p++ != 0);
			// A double zero is the end of the file list.
			if (*p == 0) break;
		}
		// Sort the filenames.
		// This is a special sort designed to do things intelligently
		// for typical sequentially numbered filenames.
		// Sorry, just a bubble sort. No need for performance here. KISS.
		n = NumLoadedFiles;
		for (i = 0; i < n - 1; i++)
		{
			for (j = 0; j < n - 1 - i; j++)
			{
				if (strverscmp(Infilename[j+1], Infilename[j]) < 0)
				{
					tmp = Infilename[j];
					Infilename[j] = Infilename[j+1];
					Infilename[j+1] = tmp;
				}
			}
		}
		// Load up the file open dialog list box and open the files.
		for (i = 0; i < NumLoadedFiles; i++)
		{
			SendDlgItemMessage(hVideoListDlg, IDC_LIST, LB_ADDSTRING, 0, (LPARAM) Infilename[i]);
			if (Infile[i] == NULL)
                Infile[i] = fopen(Infilename[i], "rb");
		}
        HadAddDialog = 1;
    }
}

void ThreadKill(int mode)
{
	int i;
	double film_percent;

	// Get rid of the % completion string in the window title.
	remain = 0;
	UpdateWindowText();

	// Close the quants log if necessary.
	if (Quants)
		fclose(Quants);

	for (i = 0; i < 0xc8; i++)
	{
		if ((D2V_Flag || AudioOnly_Flag) &&
            ((audio[i].rip && audio[i].type == FORMAT_AC3 && Method_Flag==AUDIO_DECODE)
            || (audio[i].rip && audio[i].type == FORMAT_LPCM)))
		{
			if (SRC_Flag)
			{
				EndSRC(audio[i].file);
				audio[i].size = ((int)(0.91875*audio[i].size)>>2)<<2;
			}

			Normalize(NULL, 44, audio[i].filename, audio[i].file, 44, audio[i].size);
			CloseWAV(audio[i].file, audio[i].size);
		}
	}

	if (D2V_Flag)
	{
		int i;

		if (Quants)
		{
			fclose(Quants);
			Quants = NULL;
		}
		if (Timestamps)
		{
			fclose(Timestamps);
			Timestamps = NULL;
		}
		if (mpafp)
		{
			fclose(mpafp);
			mpafp = NULL;
		}
		if (mpvfp)
		{
			fclose(mpvfp);
			mpvfp = NULL;
		}
		if (pcmfp)
		{
			fclose(pcmfp);
			pcmfp = NULL;
		}
		for (i = 0; i < 256; i++)
		{
			if (audio[i].file)
			{
				fclose(audio[i].file);
				audio[i].file = NULL;
			}
		}
	}

	if (AudioOnly_Flag)
	{
		FileLoadedEnables();
//		SendMessage(hTrack, TBM_SETSEL, (WPARAM) true, (LPARAM) MAKELONG(process.trackleft, process.trackright));
        if (NotifyWhenDone & 1)
            SetForegroundWindow(hWnd);
        if (NotifyWhenDone & 2)
		    MessageBeep(MB_OK);	
		SetDlgItemText(hDlg, IDC_REMAIN, "FINISH");
		AudioOnly_Flag = 0;
		ExitThread(0);
	}

	if (process.locate==LOCATE_INIT || process.locate==LOCATE_RIP)
	{
		if (D2V_Flag)
		{
			// Revised by Donald Graft to support IBBPIBBP...
			WriteD2VLine(1);
			fprintf(D2VFile, "\nFINISHED");
			// Prevent divide by 0.
			if (FILM_Purity+VIDEO_Purity == 0) VIDEO_Purity = 1;
			film_percent = (FILM_Purity*100.0)/(FILM_Purity+VIDEO_Purity);
			if (film_percent >= 50.0)
				fprintf(D2VFile, "  %.2f%% FILM\n", film_percent);
			else
				fprintf(D2VFile, "  %.2f%% VIDEO\n", 100.0 - film_percent);
			if (D2VFile)
			{
				fclose(D2VFile);
				D2VFile = NULL;
			}
		}

        if (MuxFile > 0 && MuxFile != (FILE *) 0xffffffff)
        {
            void StopVideoDemux(void);

            StopVideoDemux();
        }

		if (D2V_Flag)
        {
			if (fix_d2v(hWnd, D2VFilePath, 1))
            {
                // User wants to correct the field order transition.
                fix_d2v(hWnd, D2VFilePath, 0);
            }
        }

		if (Decision_Flag)
		{
			if (Sound_Max > 1)
			{
				PreScale_Ratio = 327.68 * Norm_Ratio / Sound_Max;

				if (PreScale_Ratio > 1.0 && PreScale_Ratio < 1.01)
					PreScale_Ratio = 1.0;

				sprintf(szBuffer, "%.2f", PreScale_Ratio);
				SetDlgItemText(hDlg, IDC_INFO, szBuffer);

				CheckMenuItem(hMenu, IDM_PRESCALE, MF_CHECKED);
				CheckMenuItem(hMenu, IDM_NORM, MF_UNCHECKED);
				Norm_Flag = false;
			}
			else
			{
				SetDlgItemText(hDlg, IDC_INFO, "n/a");
				CheckMenuItem(hMenu, IDM_PRESCALE, MF_UNCHECKED);
			}
		}

		FileLoadedEnables();
//		SendMessage(hTrack, TBM_SETSEL, (WPARAM) true, (LPARAM) MAKELONG(process.trackleft, process.trackright));
	}

	if (process.locate == LOCATE_RIP)
	{
        if (NotifyWhenDone & 1)
            SetForegroundWindow(hWnd);
        if (NotifyWhenDone & 2)
		    MessageBeep(MB_OK);	
		SetDlgItemText(hDlg, IDC_REMAIN, "FINISH");
		if (D2V_Flag)
			SendMessage(hWnd, D2V_DONE_MESSAGE, 0, 0);
	}
    if (LoopPlayback && mode == END_OF_DATA_KILL && process.locate == LOCATE_RIP && !D2V_Flag)
    {
	    PostMessage(hWnd, WM_COMMAND,  MAKEWPARAM(IDM_PREVIEW_NO_INFO, 0), (LPARAM) 0);
        ExitThread(0);
    }

	if (process.locate==LOCATE_INIT || process.locate==LOCATE_RIP)
	{
		D2V_Flag = false;
		Decision_Flag = false;
		Display_Flag = false;
	}
	// This restores the normal operation of the right arrow button
	// if we were in single-step playback.
   process.locate = LOCATE_INIT;

	if (CLIActive)
	{
		SendMessage(hWnd, CLI_RIP_MESSAGE, 0, 0);
	}

	ExitThread(0);
}

LRESULT CALLBACK Info(HWND hInfoDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
			return true;

        case WM_MOVE:
            GetWindowRect(hInfoDlg, &info_wrect);
            break;

        case WM_COMMAND:
			if (LOWORD(wParam)==IDCANCEL)
			{
				DestroyWindow(hInfoDlg);
				Info_Flag = false;
				return true;
			}
		case WM_DESTROY:
			{
				char logfile[DG_MAX_PATH], *p;
				FILE *lfp;
				int i, count;

                if (InfoLog_Flag)
                {
                    strcpy(logfile, Infilename[0]);
				    p = strrchr(logfile, '.');
				    strcpy(++p, "log");
				    if (lfp = fopen(logfile, "w"))
				    {
					    GetDlgItemText(hDlg, IDC_STREAM_TYPE, logfile, 255);
					    fprintf(lfp, "Stream Type: %s\n", logfile);
					    GetDlgItemText(hDlg, IDC_PROFILE, logfile, 255);
					    fprintf(lfp, "Profile: %s\n", logfile);
					    GetDlgItemText(hDlg, IDC_FRAME_SIZE, logfile, 255);
					    fprintf(lfp, "Frame Size: %s\n", logfile);
					    GetDlgItemText(hDlg, IDC_DISPLAY_SIZE, logfile, 255);
					    fprintf(lfp, "Display Size: %s\n", logfile);
					    GetDlgItemText(hDlg, IDC_ASPECT_RATIO, logfile, 255);
					    fprintf(lfp, "Aspect Ratio: %s\n", logfile);
					    GetDlgItemText(hDlg, IDC_FRAME_RATE, logfile, 255);
					    fprintf(lfp, "Frame Rate: %s\n", logfile);
					    GetDlgItemText(hDlg, IDC_VIDEO_TYPE, logfile, 255);
					    fprintf(lfp, "Video Type: %s\n", logfile);
					    GetDlgItemText(hDlg, IDC_FRAME_TYPE, logfile, 255);
					    fprintf(lfp, "Frame Type: %s\n", logfile);
					    GetDlgItemText(hDlg, IDC_CODING_TYPE, logfile, 255);
					    fprintf(lfp, "Coding Type: %s\n", logfile);
					    GetDlgItemText(hDlg, IDC_COLORIMETRY, logfile, 255);
					    fprintf(lfp, "Colorimetry: %s\n", logfile);
					    GetDlgItemText(hDlg, IDC_FRAME_STRUCTURE, logfile, 255);
					    fprintf(lfp, "Frame Structure: %s\n", logfile);
					    GetDlgItemText(hDlg, IDC_FIELD_ORDER, logfile, 255);
					    fprintf(lfp, "Field Order: %s\n", logfile);
					    GetDlgItemText(hDlg, IDC_CODED_NUMBER, logfile, 255);
					    fprintf(lfp, "Coded Number: %s\n", logfile);
					    GetDlgItemText(hDlg, IDC_PLAYBACK_NUMBER, logfile, 255);
					    fprintf(lfp, "Playback Number: %s\n", logfile);
					    GetDlgItemText(hDlg, IDC_FRAME_REPEATS, logfile, 255);
					    fprintf(lfp, "Frame Repeats: %s\n", logfile);
					    GetDlgItemText(hDlg, IDC_FIELD_REPEATS, logfile, 255);
					    fprintf(lfp, "Field Repeats: %s\n", logfile);
					    GetDlgItemText(hDlg, IDC_VOB_ID, logfile, 255);
					    fprintf(lfp, "VOB ID: %s\n", logfile);
					    GetDlgItemText(hDlg, IDC_CELL_ID, logfile, 255);
					    fprintf(lfp, "Cell ID: %s\n", logfile);
					    GetDlgItemText(hDlg, IDC_BITRATE, logfile, 255);
					    fprintf(lfp, "Bitrate: %s\n", logfile);
					    GetDlgItemText(hDlg, IDC_BITRATE_AVG, logfile, 255);
					    fprintf(lfp, "Bitrate (Avg): %s\n", logfile);
					    GetDlgItemText(hDlg, IDC_BITRATE_MAX, logfile, 255);
					    fprintf(lfp, "Bitrate (Max): %s\n", logfile);
					    if ((count = SendDlgItemMessage(hDlg, IDC_AUDIO_LIST, LB_GETCOUNT, 0, 0)) != LB_ERR)
					    {
						    for (i = 0; i < count; i++)
						    {
							    SendDlgItemMessage(hDlg, IDC_AUDIO_LIST, LB_GETTEXT, i, (LPARAM)logfile);
							    fprintf(lfp, "Audio Stream: %s\n", logfile);
						    }
					    }
					    GetDlgItemText(hDlg, IDC_TIMESTAMP, logfile, 255);
					    fprintf(lfp, "Timestamp: %s\n", logfile);
					    GetDlgItemText(hDlg, IDC_ELAPSED, logfile, 255);
					    fprintf(lfp, "Elapsed: %s\n", logfile);
					    GetDlgItemText(hDlg, IDC_REMAIN, logfile, 255);
					    fprintf(lfp, "Remain: %s\n", logfile);
					    GetDlgItemText(hDlg, IDC_FPS, logfile, 255);
					    fprintf(lfp, "FPS: %s\n", logfile);
					    GetDlgItemText(hDlg, IDC_INFO, logfile, 255);
					    fprintf(lfp, "Info: %s\n", logfile);
					    fclose(lfp);
				    }
                }
			}
			break;
	}
    return false;
}

LRESULT CALLBACK About(HWND hAboutDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
			sprintf(szBuffer, "%s", Version);
			SetDlgItemText(hAboutDlg, IDC_VERSION, szBuffer);
			return true;

		case WM_COMMAND:
			if (LOWORD(wParam)==IDOK || LOWORD(wParam)==IDCANCEL) 
			{
				EndDialog(hAboutDlg, 0);
				return true;
			}
	}
    return false;
}

LRESULT CALLBACK Cropping(HWND hDialog, UINT message, WPARAM wParam, LPARAM lParam)
{
	int i;

	switch (message)
	{
		case WM_INITDIALOG:
			SendDlgItemMessage(hDialog, IDC_LEFT_SLIDER, TBM_SETRANGE, 0, MAKELPARAM(0, 256));
			SendDlgItemMessage(hDialog, IDC_LEFT_SLIDER, TBM_SETPOS, 1, Clip_Left/4);
			SetDlgItemInt(hDialog, IDC_LEFT, Clip_Left, 0);

			SendDlgItemMessage(hDialog, IDC_RIGHT_SLIDER, TBM_SETRANGE, 0, MAKELPARAM(0, 256));
			SendDlgItemMessage(hDialog, IDC_RIGHT_SLIDER, TBM_SETPOS, 1, Clip_Right/4);
			SetDlgItemInt(hDialog, IDC_RIGHT, Clip_Right, 0);

			SendDlgItemMessage(hDialog, IDC_TOP_SLIDER, TBM_SETRANGE, 0, MAKELPARAM(0, 256));
			SendDlgItemMessage(hDialog, IDC_TOP_SLIDER, TBM_SETPOS, 1, Clip_Top/4);
			SetDlgItemInt(hDialog, IDC_TOP, Clip_Top, 0);

			SendDlgItemMessage(hDialog, IDC_BOTTOM_SLIDER, TBM_SETRANGE, 0, MAKELPARAM(0, 256));
			SendDlgItemMessage(hDialog, IDC_BOTTOM_SLIDER, TBM_SETPOS, 1, Clip_Bottom/4);
			SetDlgItemInt(hDialog, IDC_BOTTOM, Clip_Bottom, 0);

			SetDlgItemInt(hDialog, IDC_WIDTH, horizontal_size-Clip_Left-Clip_Right, 0);
			SetDlgItemInt(hDialog, IDC_HEIGHT, vertical_size-Clip_Top-Clip_Bottom, 0);

			ShowWindow(hDialog, SW_SHOW);

			if (Cropping_Flag)
				SendDlgItemMessage(hDialog, IDC_CROPPING_CHECK, BM_SETCHECK, BST_CHECKED, 0);
			return true;

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDC_CROPPING_CHECK:
					if (SendDlgItemMessage(hDialog, IDC_CROPPING_CHECK, BM_GETCHECK, 1, 0)==BST_CHECKED)
					{
						CheckMenuItem(hMenu, IDM_CROPPING, MF_CHECKED);
						Cropping_Flag = true;
					}
					else
					{
						CheckMenuItem(hMenu, IDM_CROPPING, MF_UNCHECKED);
						Cropping_Flag = false;
					}

					RefreshWindow(true);
					ShowInfo(false);
					break;

				case IDCANCEL:
					EndDialog(hDialog, 0);
					return true;
			}
			break;

		case WM_HSCROLL:
			switch (GetWindowLong((HWND)lParam, GWL_ID))
			{
				case IDC_LEFT_SLIDER:
					i = SendDlgItemMessage(hDialog, IDC_LEFT_SLIDER, TBM_GETPOS, 0, 0)*4;
					if (i+Clip_Right+MIN_WIDTH <= horizontal_size)
						Clip_Left = i;
					else
						Clip_Left = horizontal_size - (Clip_Right+MIN_WIDTH);
					SetDlgItemInt(hDialog, IDC_LEFT, Clip_Left, 0);
					SetDlgItemInt(hDialog, IDC_WIDTH, horizontal_size-Clip_Left-Clip_Right, 0);
					break;

				case IDC_RIGHT_SLIDER:
					i = SendDlgItemMessage(hDialog, IDC_RIGHT_SLIDER, TBM_GETPOS, 0, 0)*4;
					if (i+Clip_Left+MIN_WIDTH <= horizontal_size)
						Clip_Right = i;
					else
						Clip_Right = horizontal_size - (Clip_Left+MIN_WIDTH);
					SetDlgItemInt(hDialog, IDC_RIGHT, Clip_Right, 0);
					SetDlgItemInt(hDialog, IDC_WIDTH, horizontal_size-Clip_Left-Clip_Right, 0);
					break;

				case IDC_TOP_SLIDER:
					i = SendDlgItemMessage(hDialog, IDC_TOP_SLIDER, TBM_GETPOS, 0, 0)*4;
					if (i+Clip_Bottom+MIN_HEIGHT <= vertical_size)
						Clip_Top = i;
					else
						Clip_Top = vertical_size - (Clip_Bottom+MIN_HEIGHT);
					SetDlgItemInt(hDialog, IDC_TOP, Clip_Top, 0);
					SetDlgItemInt(hDialog, IDC_HEIGHT, vertical_size-Clip_Top-Clip_Bottom, 0);
					break;

				case IDC_BOTTOM_SLIDER:
					i = SendDlgItemMessage(hDialog, IDC_BOTTOM_SLIDER, TBM_GETPOS, 0, 0)*4;
					if (i+Clip_Top+MIN_HEIGHT <= vertical_size)
						Clip_Bottom = i;
					else
						Clip_Bottom = vertical_size - (Clip_Top+MIN_HEIGHT);
					SetDlgItemInt(hDialog, IDC_BOTTOM, Clip_Bottom, 0);
					SetDlgItemInt(hDialog, IDC_HEIGHT, vertical_size-Clip_Top-Clip_Bottom, 0);
					break;
			}

			RefreshWindow(true);
			ShowInfo(false);
			break;
	}
    return false;
}

LRESULT CALLBACK Luminance(HWND hDialog, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
			SendDlgItemMessage(hDialog, IDC_GAMMA_SPIN, UDM_SETRANGE, 0, MAKELPARAM(511, 1));
			SendDlgItemMessage(hDialog, IDC_GAMMA_SPIN, UDM_SETPOS, 1, LumGamma + 256);
			sprintf(szTemp, "%d", LumGamma);
			SetDlgItemText(hDialog, IDC_GAMMA_BOX, szTemp);	

			SendDlgItemMessage(hDialog, IDC_OFFSET_SPIN, UDM_SETRANGE, 0, MAKELPARAM(511, 1));
			SendDlgItemMessage(hDialog, IDC_OFFSET_SPIN, UDM_SETPOS, 1, LumOffset + 256);
			sprintf(szTemp, "%d", LumOffset);
			SetDlgItemText(hDialog, IDC_OFFSET_BOX, szTemp);	

			ShowWindow(hDialog, SW_SHOW);

			if (Luminance_Flag)
				SendDlgItemMessage(hDialog, IDC_LUM_CHECK, BM_SETCHECK, BST_CHECKED, 0);
			return true;

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDC_LUM_CHECK:
					if (SendDlgItemMessage(hDialog, IDC_LUM_CHECK, BM_GETCHECK, 1, 0)==BST_CHECKED)
					{
						CheckMenuItem(hMenu, IDM_LUMINANCE, MF_CHECKED);
						Luminance_Flag = true;
					}
					else
					{
						CheckMenuItem(hMenu, IDM_LUMINANCE, MF_UNCHECKED);
						Luminance_Flag = false;
					}
					
					RefreshWindow(true);
					break;

				case IDCANCEL:
					EndDialog(hDialog, 0);
					return true;
			}
			break;

		case WM_VSCROLL:
			switch (GetWindowLong((HWND)lParam, GWL_ID))
			{
				case IDC_GAMMA_SPIN:
					LumGamma = LOWORD(SendDlgItemMessage(hDialog, IDC_GAMMA_SPIN, UDM_GETPOS, 0,0)) - 256;
					sprintf(szTemp, "%d", LumGamma);
					SetDlgItemText(hDialog, IDC_GAMMA_BOX, szTemp);	
					break;
				case IDC_OFFSET_SPIN:
					LumOffset = LOWORD(SendDlgItemMessage(hDialog, IDC_OFFSET_SPIN, UDM_GETPOS, 0,0)) - 256;
					sprintf(szTemp, "%d", LumOffset);
					SetDlgItemText(hDialog, IDC_OFFSET_BOX, szTemp);	
					break;
			}

			RefreshWindow(true);
			break;
	}
    return false;
}

LRESULT CALLBACK AVSTemplate(HWND hDialog, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
			sprintf(szTemp, "%s", AVSTemplatePath);
			SetDlgItemText(hDialog, IDC_AVS_TEMPLATE, szTemp);
			ShowWindow(hDialog, SW_SHOW);
			return true;

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDC_NO_TEMPLATE:
					AVSTemplatePath[0] = 0;
					sprintf(szTemp, "%s", "");
					SetDlgItemText(hDialog, IDC_AVS_TEMPLATE, szTemp);
					ShowWindow(hDialog, SW_SHOW);
					EndDialog(hDialog, 0);
					return true;
				case IDC_CHANGE_TEMPLATE:
					if (PopFileDlg(AVSTemplatePath, hWnd, OPEN_AVS))
					{
						sprintf(szTemp, "%s", AVSTemplatePath);
						SetDlgItemText(hDialog, IDC_AVS_TEMPLATE, szTemp);
					}
					else
					{
						AVSTemplatePath[0] = 0;
						sprintf(szTemp, "%s", "");
						SetDlgItemText(hDialog, IDC_AVS_TEMPLATE, szTemp);
					}
					ShowWindow(hDialog, SW_SHOW);
					EndDialog(hDialog, 0);
					return true;
				case IDC_KEEP_TEMPLATE:
				case IDCANCEL:
					EndDialog(hDialog, 0);
					return true;
			}
			break;
	}
    return false;
}

LRESULT CALLBACK BMPPath(HWND hDialog, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
			sprintf(szTemp, "%s", BMPPathString);
			SetDlgItemText(hDialog, IDC_BMP_PATH, szTemp);
			ShowWindow(hDialog, SW_SHOW);
			return true;

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDC_BMP_PATH_OK:
					GetDlgItemText(hDialog, IDC_BMP_PATH, BMPPathString, DG_MAX_PATH - 1);
					ShowWindow(hDialog, SW_SHOW);
					EndDialog(hDialog, 0);
					return true;
				case IDC_BMP_PATH_CANCEL:
				case IDCANCEL:
					EndDialog(hDialog, 0);
					return true;
			}
			break;
	}
    return false;
}

LRESULT CALLBACK SetPids(HWND hDialog, UINT message, WPARAM wParam, LPARAM lParam)
{
	char buf[80];

	switch (message)
	{
		case WM_INITDIALOG:
			sprintf(szTemp, "%x", MPEG2_Transport_VideoPID);
			SetDlgItemText(hDialog, IDC_VIDEO_PID, szTemp);
			sprintf(szTemp, "%x", MPEG2_Transport_AudioPID);
			SetDlgItemText(hDialog, IDC_AUDIO_PID, szTemp);
			sprintf(szTemp, "%x", MPEG2_Transport_PCRPID);
			SetDlgItemText(hDialog, IDC_PCR_PID, szTemp);
			ShowWindow(hDialog, SW_SHOW);
			return true;

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDC_PIDS_OK:
					GetDlgItemText(hDialog, IDC_VIDEO_PID, buf, 10);
					sscanf(buf, "%x", &MPEG2_Transport_VideoPID);
					GetDlgItemText(hDialog, IDC_AUDIO_PID, buf, 10);
					sscanf(buf, "%x", &MPEG2_Transport_AudioPID);
					GetDlgItemText(hDialog, IDC_PCR_PID, buf, 10);
					sscanf(buf, "%x", &MPEG2_Transport_PCRPID);
					EndDialog(hDialog, 0);
					Recovery();
					if (NumLoadedFiles)
					{
						FileLoadedEnables();
						process.rightfile = NumLoadedFiles-1;
						process.rightlba = (int)(Infilelength[NumLoadedFiles-1]/SECTOR_SIZE);

						process.end = Infiletotal - SECTOR_SIZE;
						process.endfile = NumLoadedFiles - 1;
						process.endloc = (Infilelength[NumLoadedFiles-1]/SECTOR_SIZE - 1)*SECTOR_SIZE;

						process.locate = LOCATE_INIT;

						if (!threadId || WaitForSingleObject(hThread, INFINITE)==WAIT_OBJECT_0)
							hThread = CreateThread(NULL, 0, MPEG2Dec, 0, 0, &threadId);
					}
					return true;

				case IDCANCEL:
				case IDC_PIDS_CANCEL:
					EndDialog(hDialog, 0);
					return true;
			}
			break;
	}
    return false;
}

LRESULT CALLBACK Normalization(HWND hDialog, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
			SendDlgItemMessage(hDialog, IDC_NORM_SLIDER, TBM_SETRANGE, 0, MAKELPARAM(0, 100));
			SendDlgItemMessage(hDialog, IDC_NORM_SLIDER, TBM_SETTICFREQ, 50, 0);
			SendDlgItemMessage(hDialog, IDC_NORM_SLIDER, TBM_SETPOS, 1, Norm_Ratio);
			sprintf(szTemp, "%d", Norm_Ratio);
			SetDlgItemText(hDialog, IDC_NORM, szTemp);

			ShowWindow(hDialog, SW_SHOW);

			if (Norm_Flag)
				SendDlgItemMessage(hDialog, IDC_NORM_CHECK, BM_SETCHECK, BST_CHECKED, 0);
			return true;

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDC_NORM_CHECK:
					if (SendDlgItemMessage(hDialog, IDC_NORM_CHECK, BM_GETCHECK, 1, 0)==BST_CHECKED)
					{
						CheckMenuItem(hMenu, IDM_NORM, MF_CHECKED);
						Norm_Flag = true;
					}
					else
					{
						CheckMenuItem(hMenu, IDM_NORM, MF_UNCHECKED);
						Norm_Flag = false;
					}
					break;

				case IDCANCEL:
					EndDialog(hDialog, 0);
					return true;
			}
			break;

		case WM_HSCROLL:
			if (GetWindowLong((HWND)lParam, GWL_ID)==IDC_NORM_SLIDER)
			{
				Norm_Ratio = SendDlgItemMessage(hDialog, IDC_NORM_SLIDER, TBM_GETPOS, 0, 0);
				sprintf(szTemp, "%d", Norm_Ratio);
				SetDlgItemText(hDialog, IDC_NORM, szTemp);
			}
			break;
	}
    return false;
}

LRESULT CALLBACK SelectTracks(HWND hDialog, UINT message, WPARAM wParam, LPARAM lParam)
{
	unsigned int i;
    static char track_list[255];
    char *p;
    unsigned int audio_id;

	switch (message)
	{
		case WM_INITDIALOG:
            SetDlgItemText(hDialog, IDC_TRACK_LIST, Track_List);
            strcpy(track_list, Track_List);
            for (i = 0; i < 0xc8; i++)
                audio[i].selected_for_demux = false;
            p = Track_List;
            while ((*p >= '0' && *p <= '9') || (*p >= 'a' && *p <= 'f') || (*p >= 'A' && *p <= 'F'))
            {
                sscanf(p, "%x", &audio_id);
                if (audio_id > 0xc7)
                    break;
                audio[audio_id].selected_for_demux = true;
                while (*p != ',' && *p != 0) p++;
                if (*p == 0)
                    break;
                p++;
            }
			ShowWindow(hDialog, SW_SHOW);

			return true;

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDC_TRACK_OK:
                    GetDlgItemText(hDialog, IDC_TRACK_LIST, track_list, 255);
                    if (strcmp(Track_List, track_list))
                    {
                        strcpy(Track_List, track_list);
                        for (i = 0; i < 0xc8; i++)
                            audio[i].selected_for_demux = false;
                        p = Track_List;
                        while ((*p >= '0' && *p <= '9') || (*p >= 'a' && *p <= 'f') || (*p >= 'A' && *p <= 'F'))
                        {
                            sscanf(p, "%x", &audio_id);
                            if (audio_id > 0xc7)
                                break;
                            audio[audio_id].selected_for_demux = true;
                            while (*p != ',' && *p != 0) p++;
                            if (*p == 0)
                                break;
                            p++;
                        }
						CheckMenuItem(hMenu, IDM_PRESCALE, MF_UNCHECKED);
						PreScale_Ratio = 1.0;
                    }
					EndDialog(hDialog, 0);
					return false;
				case IDCANCEL:
				case IDC_TRACK_CANCEL:
					EndDialog(hDialog, 0);
					return true;
			}
			break;
	}
    return false;
}

LRESULT CALLBACK SelectDelayTrack(HWND hDialog, UINT message, WPARAM wParam, LPARAM lParam)
{
    static char delay_track[255];
    char *p;
    unsigned char audio_id;

	switch (message)
	{
		case WM_INITDIALOG:
            SetDlgItemText(hDialog, IDC_DELAY_LIST, Delay_Track);
            strcpy(delay_track, Delay_Track);
			ShowWindow(hDialog, SW_SHOW);
			return true;

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDC_DELAY_OK:
                    GetDlgItemText(hDialog, IDC_DELAY_LIST, delay_track, 255);
                    strcpy(Delay_Track, delay_track);
                    p = delay_track;
					unsigned int dumdum;
                    (void)sscanf(p, "%x", &dumdum);
					audio_id = (unsigned char) dumdum;
					if (PopFileDlg(szInput, hWnd, OPEN_TXT))
					{
						if (analyze_sync(hWnd, szInput, audio_id))
						{
							ShellExecute(hDlg, "open", szInput, NULL, NULL, SW_SHOWNORMAL);
						}
					}
					EndDialog(hDialog, 0);
					return false;
				case IDCANCEL:
				case IDC_TRACK_CANCEL:
					EndDialog(hDialog, 0);
					return true;
			}
			break;
	}
    return false;
}

/* register the window class */
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize			= sizeof(WNDCLASSEX);
	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= (WNDPROC)WndProc;
	wcex.cbClsExtra		= false;
	wcex.cbWndExtra		= false;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, (LPCTSTR)IDI_MOVIE);
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= CreateSolidBrush(MASKCOLOR);

	wcex.lpszMenuName = (LPCSTR)IDC_GUI;
	wcex.lpszMenuName = (LPCSTR)IDC_GUI;
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, (LPCTSTR)IDI_SMALL);

	return RegisterClassEx(&wcex);
}

bool PopFileDlg(PTSTR pstrFileName, HWND hOwner, int Status)
{
	static OPENFILENAME ofn;
	static char *szFilter, *ext;

	switch (Status)
	{
		case OPEN_VOB:
			ofn.nFilterIndex = 4;
			szFilter = \
				TEXT ("vob\0*.vob\0") \
				TEXT ("mpg, mpeg, m1v, m2v, mpv\0*.mpg;*.mpeg;*.m1v;*.m2v;*.mpv\0") \
				TEXT ("tp, ts, trp, m2t, m2ts, pva, vro\0*.tp;*.ts;*.trp;*.m2t;*.m2ts;*.pva;*.vro\0") \
				TEXT ("All MPEG Files\0*.vob;*.mpg;*.mpeg;*.m1v;*.m2v;*.mpv;*.tp;*.ts;*.trp;*.m2t;*.m2ts;*.pva;*.vro\0") \
				TEXT ("All Files (*.*)\0*.*\0");
			break;

		case OPEN_D2V:
			szFilter = TEXT ("DGIndex Project File (*.d2v)\0*.d2v\0")  \
				TEXT ("All Files (*.*)\0*.*\0");
			break;

		case OPEN_TXT:
			szFilter = TEXT ("DGIndex Timestamps File (*.txt)\0*.txt\0")  \
				TEXT ("All Files (*.*)\0*.*\0");
			break;

		case OPEN_AVS:
			szFilter = TEXT ("Template File (*.avs, *.vpy)\0*.avs;*.vpy\0")  \
				TEXT ("All Files (*.*)\0*.*\0");
			break;

		case SAVE_D2V:
			szFilter = TEXT ("DGIndex Project File (*.d2v)\0*.d2v\0")  \
				TEXT ("All Files (*.*)\0*.*\0");
			break;

		case SAVE_BMP:
			szFilter = TEXT ("BMP File (*.bmp)\0*.bmp\0")  \
				TEXT ("All Files (*.*)\0*.*\0");
			break;

		case OPEN_WAV:
		case SAVE_WAV:
			szFilter = TEXT ("WAV File (*.wav)\0*.wav\0")  \
				TEXT ("All Files (*.*)\0*.*\0");
			break;
	}

	ofn.lStructSize       = sizeof (OPENFILENAME) ;
	ofn.hwndOwner         = hOwner ;
	ofn.hInstance         = hInst ;
	ofn.lpstrFilter       = szFilter ;
	ofn.nMaxFile          = MAX_FILE_NUMBER * DG_MAX_PATH - 1 ;
	ofn.lpstrFileTitle    = 0 ;
	ofn.lpstrFile         = pstrFileName ;
	ofn.lpstrInitialDir   = szSave;

	switch (Status)
	{
		case OPEN_VOB:
		case OPEN_D2V:
		case OPEN_WAV:
			crop1088_warned = false;
			*ofn.lpstrFile = 0;
			ofn.Flags = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT | OFN_EXPLORER;
			return GetOpenFileName(&ofn);

		case OPEN_AVS:
		case OPEN_TXT:
			*ofn.lpstrFile = 0;
			ofn.Flags = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT | OFN_EXPLORER;
			return GetOpenFileName(&ofn);

		case SAVE_BMP:
			*ofn.lpstrFile = 0;
            ofn.lpstrInitialDir = BMPPathString;
			ofn.Flags = OFN_HIDEREADONLY | OFN_EXPLORER;
			if (GetSaveFileName(&ofn))
			{
				ext = strrchr(pstrFileName, '.');
				if (ext!=NULL && !_strnicmp(ext, ".bmp", 4))
					*ext = 0;
				return true;
			}
			break;

		case SAVE_WAV:
			*ofn.lpstrFile = 0;
			ofn.Flags = OFN_HIDEREADONLY | OFN_EXPLORER;
			if (GetSaveFileName(&ofn))
			{
				ext = strrchr(pstrFileName, '.');
				if (ext!=NULL && !_strnicmp(ext, ".wav", 4))
					*ext = 0;
				return true;
			}
			break;

		case SAVE_D2V:
			{
			char *p;

			ofn.Flags = OFN_HIDEREADONLY | OFN_EXPLORER;
			// Load a default filename based on the name of the first input file.
			if (szOutput[0] == NULL)
			{
				strcpy(ofn.lpstrFile, Infilename[0]);
				p = &ofn.lpstrFile[strlen(ofn.lpstrFile)];
				while (*p != '.' && p >= ofn.lpstrFile) p--;
				if (p != ofn.lpstrFile)
				{
					*p = 0;
				}
			}
			if (GetSaveFileName(&ofn))
			{
				ext = strrchr(pstrFileName, '.');
				if (ext!=NULL && !_strnicmp(ext, ".d2v", 4))
					*ext = 0;
				return true;
			}
			break;
			}
	}
	return false;
}

static void ShowInfo(bool update)
{
    RECT rect;

	if (update)
	{
		if (Info_Flag)
        {
			DestroyWindow(hDlg);
        }

		Info_Flag = true;
		hDlg = CreateDialog(hInst, (LPCTSTR)IDD_INFO, hWnd, (DLGPROC)Info);
	}

	if (Info_Flag)
	{
		GetWindowRect(hDlg, &rect);
		MoveWindow(hDlg, info_wrect.left, info_wrect.top, rect.right-rect.left, rect.bottom-rect.top, true);
		ShowWindow(hDlg, WindowMode);
	}
    SetFocus(hWnd);
}

void CheckFlag()
{
	CheckMenuItem(hMenu, IDM_IDCT_MMX, MF_UNCHECKED);
	CheckMenuItem(hMenu, IDM_IDCT_SSEMMX, MF_UNCHECKED);
	CheckMenuItem(hMenu, IDM_IDCT_SSE2MMX, MF_UNCHECKED);
	CheckMenuItem(hMenu, IDM_IDCT_FPU, MF_UNCHECKED);
	CheckMenuItem(hMenu, IDM_IDCT_REF, MF_UNCHECKED);
	CheckMenuItem(hMenu, IDM_IDCT_SKAL, MF_UNCHECKED);
	CheckMenuItem(hMenu, IDM_IDCT_SIMPLE, MF_UNCHECKED);
	CheckMenuItem(hMenu, IDM_PCSCALE, MF_UNCHECKED);
	CheckMenuItem(hMenu, IDM_TVSCALE, MF_UNCHECKED);
	CheckMenuItem(hMenu, IDM_FO_FILM, MF_UNCHECKED);

	//Downgrade the iDCT if the processor does not support it.
	if (iDCT_Flag == IDCT_SSE2MMX && !cpu.sse2)
		iDCT_Flag = IDCT_SKAL;
	if (iDCT_Flag == IDCT_SKAL && !cpu.ssemmx)
		iDCT_Flag = IDCT_SSEMMX;
	if (iDCT_Flag == IDCT_SSEMMX && !cpu.ssemmx)
		iDCT_Flag = IDCT_MMX;

	switch (iDCT_Flag)
	{
		case IDCT_MMX:
			CheckMenuItem(hMenu, IDM_IDCT_MMX, MF_CHECKED);
			break;

		case IDCT_SSEMMX:
			CheckMenuItem(hMenu, IDM_IDCT_SSEMMX, MF_CHECKED);
			break;

		case IDCT_SSE2MMX:
			CheckMenuItem(hMenu, IDM_IDCT_SSE2MMX, MF_CHECKED);
			break;

		case IDCT_FPU:
			CheckMenuItem(hMenu, IDM_IDCT_FPU, MF_CHECKED);
			break;

		case IDCT_REF:
			CheckMenuItem(hMenu, IDM_IDCT_REF, MF_CHECKED);
			break;

		case IDCT_SKAL:
			CheckMenuItem(hMenu, IDM_IDCT_SKAL, MF_CHECKED);
			break;

		case IDCT_SIMPLE:
			CheckMenuItem(hMenu, IDM_IDCT_SIMPLE, MF_CHECKED);
			break;
	}

	setRGBValues();
	if (Scale_Flag) CheckMenuItem(hMenu, IDM_PCSCALE, MF_CHECKED);
	else CheckMenuItem(hMenu, IDM_TVSCALE, MF_CHECKED);

	switch (FO_Flag)
	{
		case FO_NONE:
			CheckMenuItem(hMenu, IDM_FO_NONE, MF_CHECKED);
			CheckMenuItem(hMenu, IDM_FO_FILM, MF_UNCHECKED);
			CheckMenuItem(hMenu, IDM_FO_RAW, MF_UNCHECKED);
			break;

		case FO_FILM:
			CheckMenuItem(hMenu, IDM_FO_NONE, MF_UNCHECKED);
			CheckMenuItem(hMenu, IDM_FO_FILM, MF_CHECKED);
			CheckMenuItem(hMenu, IDM_FO_RAW, MF_UNCHECKED);
			break;

		case FO_RAW:
			CheckMenuItem(hMenu, IDM_FO_NONE, MF_UNCHECKED);
			CheckMenuItem(hMenu, IDM_FO_FILM, MF_UNCHECKED);
			CheckMenuItem(hMenu, IDM_FO_RAW, MF_CHECKED);
			break;
	}

	switch (Method_Flag)
	{
		case AUDIO_NONE:
			CheckMenuItem(hMenu, IDM_AUDIO_NONE, MF_CHECKED);
			break;

		case AUDIO_DEMUX:
			CheckMenuItem(hMenu, IDM_DEMUX, MF_CHECKED);
			break;

		case AUDIO_DEMUXALL:
			CheckMenuItem(hMenu, IDM_DEMUXALL, MF_CHECKED);
			break;

		case AUDIO_DECODE:
			CheckMenuItem(hMenu, IDM_DECODE, MF_CHECKED);
			break;
	}

	if (Method_Flag == AUDIO_DECODE) switch (DRC_Flag)
	{
		case DRC_NONE:
			CheckMenuItem(hMenu, IDM_DRC_NONE, MF_CHECKED);
			break;

		case DRC_LIGHT:
			CheckMenuItem(hMenu, IDM_DRC_LIGHT, MF_CHECKED);
			break;

		case DRC_NORMAL:
			CheckMenuItem(hMenu, IDM_DRC_NORMAL, MF_CHECKED);
			break;

		case DRC_HEAVY:
			CheckMenuItem(hMenu, IDM_DRC_HEAVY, MF_CHECKED);
			break;
	}

	if (Method_Flag == AUDIO_DECODE && DSDown_Flag)
		CheckMenuItem(hMenu, IDM_DSDOWN, MF_CHECKED);

	if (Method_Flag == AUDIO_DECODE) switch (SRC_Flag)
	{
		case SRC_NONE:
			CheckMenuItem(hMenu, IDM_SRC_NONE, MF_CHECKED);
			break;

		case SRC_LOW:
			CheckMenuItem(hMenu, IDM_SRC_LOW, MF_CHECKED);
			break;

		case SRC_MID:
			CheckMenuItem(hMenu, IDM_SRC_MID, MF_CHECKED);
			break;

		case SRC_HIGH:
			CheckMenuItem(hMenu, IDM_SRC_HIGH, MF_CHECKED);
			break;

		case SRC_UHIGH:
			CheckMenuItem(hMenu, IDM_SRC_UHIGH, MF_CHECKED);
			break;
	}

	if (Method_Flag == AUDIO_DECODE && Norm_Ratio > 100)
	{
		CheckMenuItem(hMenu, IDM_NORM, MF_CHECKED);
		Norm_Flag = true;
		Norm_Ratio -= 100;
	}

	switch (Priority_Flag)
	{
		case PRIORITY_HIGH:
			CheckMenuItem(hMenu, IDM_PP_HIGH, MF_CHECKED);
			SetPriorityClass(hProcess, HIGH_PRIORITY_CLASS);
			break;

		case PRIORITY_NORMAL:
			CheckMenuItem(hMenu, IDM_PP_NORMAL, MF_CHECKED);
			SetPriorityClass(hProcess, NORMAL_PRIORITY_CLASS);
			break;

		case PRIORITY_LOW:
			CheckMenuItem(hMenu, IDM_PP_LOW, MF_CHECKED);
			SetPriorityClass(hProcess, IDLE_PRIORITY_CLASS);
			break;
	}

	switch (PlaybackSpeed)
	{
		case SPEED_SINGLE_STEP:
			CheckMenuItem(hMenu, IDM_SPEED_SINGLE_STEP, MF_CHECKED);
			break;

		case SPEED_SUPER_SLOW:
			CheckMenuItem(hMenu, IDM_SPEED_SUPER_SLOW, MF_CHECKED);
			break;

		case SPEED_SLOW:
			CheckMenuItem(hMenu, IDM_SPEED_SLOW, MF_CHECKED);
			break;

		case SPEED_NORMAL:
			CheckMenuItem(hMenu, IDM_SPEED_NORMAL, MF_CHECKED);
			break;

		case SPEED_FAST:
			CheckMenuItem(hMenu, IDM_SPEED_FAST, MF_CHECKED);
			break;

		case SPEED_MAXIMUM:
			CheckMenuItem(hMenu, IDM_SPEED_MAXIMUM, MF_CHECKED);
			break;
	}

	switch (HDDisplay)
	{
		case HD_DISPLAY_FULL_SIZED:
			CheckMenuItem(hMenu, IDM_FULL_SIZED, MF_CHECKED);
			break;

		case HD_DISPLAY_SHRINK_BY_HALF:
			CheckMenuItem(hMenu, IDM_SHRINK_BY_HALF, MF_CHECKED);
			break;

		case HD_DISPLAY_TOP_LEFT:
			CheckMenuItem(hMenu, IDM_TOP_LEFT, MF_CHECKED);
			break;

		case HD_DISPLAY_TOP_RIGHT:
			CheckMenuItem(hMenu, IDM_TOP_RIGHT, MF_CHECKED);
			break;

		case HD_DISPLAY_BOTTOM_LEFT:
			CheckMenuItem(hMenu, IDM_BOTTOM_LEFT, MF_CHECKED);
			break;

		case HD_DISPLAY_BOTTOM_RIGHT:
			CheckMenuItem(hMenu, IDM_BOTTOM_RIGHT, MF_CHECKED);
			break;
	}

	if (ForceOpenGops)
		CheckMenuItem(hMenu, IDM_FORCE_OPEN, MF_CHECKED);

    if (FullPathInFiles)
        CheckMenuItem(hMenu, IDM_FULL_PATH, MF_CHECKED);

    if (LoopPlayback)
        CheckMenuItem(hMenu, IDM_LOOP_PLAYBACK, MF_CHECKED);

    if (FusionAudio)
        CheckMenuItem(hMenu, IDM_FUSION_AUDIO, MF_CHECKED);

    if (InfoLog_Flag)
        CheckMenuItem(hMenu, IDM_INFO_LOG, MF_CHECKED);
}

void Recovery()
{
	int i;

	if (Check_Flag)
	{
		for (i=0; i<3; i++)
		{
			_aligned_free(backward_reference_frame[i]);
			_aligned_free(forward_reference_frame[i]);
			_aligned_free(auxframe[i]);
		}

		_aligned_free(u422);
		_aligned_free(v422);
		_aligned_free(u444);
		_aligned_free(v444);
		_aligned_free(rgb24);
		_aligned_free(rgb24small);
		_aligned_free(yuy2);
		_aligned_free(lum);
	}

	Check_Flag = false;
	MuxFile = (FILE *) 0xffffffff;

	SendMessage(hTrack, TBM_SETPOS, (WPARAM) true, 0);
	InvalidateRect(hwndSelect, NULL, TRUE);
//	SendMessage(hTrack, TBM_SETSEL, (WPARAM) true, (LPARAM) MAKELONG(0, 0));

	LumGamma = LumOffset = 0;
	Luminance_Flag = false;
	CheckMenuItem(hMenu, IDM_LUMINANCE, MF_UNCHECKED);

	Clip_Left = Clip_Right = Clip_Top = Clip_Bottom = 0;
	Cropping_Flag = false;
	CheckMenuItem(hMenu, IDM_CROPPING, MF_UNCHECKED);

	PreScale_Ratio = 1.0;
	CheckMenuItem(hMenu, IDM_PRESCALE, MF_UNCHECKED);

	SetWindowText(hWnd, "DGIndex");

	if (NumLoadedFiles)
	{
		ZeroMemory(&process, sizeof(process));
		process.trackright = TRACK_PITCH;

		Display_Flag = true;

		for (i=0, Infiletotal = 0; i<NumLoadedFiles; i++)
		{
#if 0
			long long save = _fseeki64(Infile[i], 0, SEEK_CUR);
			_fseeki64(Infile[i], 0, SEEK_END);
			Infilelength[i] = _ftelli64(Infile[i]);
			_fseeki64(Infile[i], save, SEEK_SET);
#else
			DWORD high;
			Infilelength[i] = GetFileSize((HANDLE)_get_osfhandle(_fileno(Infile[i])), &high);
			Infilelength[i] |= ((long long)high << 32);
#endif
			Infiletotal += Infilelength[i];
		}
	}

	InvalidateRect(hwndSelect, NULL, TRUE);
	ResizeWindow(INIT_WIDTH, INIT_HEIGHT);
	ResizeWindow(INIT_WIDTH, INIT_HEIGHT);	// 2-line menu antidote

	if (!CLIActive)
		szOutput[0] = 0;
	VOB_ID = CELL_ID = 0;

	SystemStream_Flag = ELEMENTARY_STREAM;

    if (NumLoadedFiles)
    {
        AddMRUList(Infilename[0]);
        UpdateMRUList();
    }
}

void ResizeWindow(int width, int height)
{
	MoveWindow(hTrack, 0, height+TRACK_HEIGHT/3, width-4*TRACK_HEIGHT, TRACK_HEIGHT, true);
	MoveWindow(hwndSelect, 0, height, width, TRACK_HEIGHT/3, true);
	MoveWindow(hLeftButton, width-4*TRACK_HEIGHT, height+TRACK_HEIGHT/3, TRACK_HEIGHT, TRACK_HEIGHT, true);
	MoveWindow(hLeftArrow, width-3*TRACK_HEIGHT, height+TRACK_HEIGHT/3, TRACK_HEIGHT, TRACK_HEIGHT, true);
	MoveWindow(hRightArrow, width-2*TRACK_HEIGHT, height+TRACK_HEIGHT/3, TRACK_HEIGHT, TRACK_HEIGHT, true);
	MoveWindow(hRightButton, width-TRACK_HEIGHT, height+TRACK_HEIGHT/3, TRACK_HEIGHT, TRACK_HEIGHT, true);

	GetWindowRect(hWnd, &wrect);
	GetClientRect(hWnd, &crect);
	Edge_Width = wrect.right - wrect.left - crect.right + crect.left;
	Edge_Height = wrect.bottom - wrect.top - crect.bottom + crect.top;

	MoveWindow(hWnd, wrect.left, wrect.top, width+Edge_Width, height+Edge_Height+TRACK_HEIGHT+TRACK_HEIGHT/3, true);
}

void RefreshWindow(bool update)
{
	if (update)
	{
		if (Check_Flag && WaitForSingleObject(hThread, 0)==WAIT_OBJECT_0)
		{
			Fault_Flag = 0;
			Display_Flag = true;
			Write_Frame(backward_reference_frame, d2v_backward, 0);
		}
	}
	else
		ShowFrame(true);
}

static void SaveBMP()
{
	FILE *BMPFile;

	if (!PopFileDlg(szTemp, hWnd, SAVE_BMP)) return;
	strcat(szTemp, ".bmp");
	if (fopen(szTemp, "r"))
	{
		char line[255];
		sprintf(line, "%s already exists.\nDo you want to replace it?", szTemp);
		if (MessageBox(hWnd, line, "Save BMP",
			MB_YESNO | MB_ICONWARNING) != IDYES)
		return;
	}
	if ((BMPFile = fopen(szTemp, "wb")) == NULL)
		return;

	int width = horizontal_size;
	int height = vertical_size;

	if (Cropping_Flag)
	{
		width -= Clip_Left+Clip_Right;
		height -= Clip_Top+Clip_Bottom;
	}

	BITMAPFILEHEADER bmfh;
	BITMAPINFOHEADER bmih;

	ZeroMemory(&bmfh, sizeof(BITMAPFILEHEADER));
	bmfh.bfType = 'M'*256 + 'B';
	bmfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
	bmfh.bfSize = bmfh.bfOffBits + ((width*24+31)&~31)/8*height;

	ZeroMemory(&bmih, sizeof(BITMAPINFOHEADER));
	bmih.biSize = sizeof(BITMAPINFOHEADER);
	bmih.biWidth = width;
	bmih.biHeight = height;
	bmih.biPlanes = 1;
	bmih.biBitCount = 24;
	bmih.biCompression = BI_RGB;
	bmih.biSizeImage = width * height * 3;

	fwrite(&bmfh, sizeof(BITMAPFILEHEADER), 1, BMPFile);
	fwrite(&bmih, sizeof(BITMAPINFOHEADER), 1, BMPFile);
	fwrite(rgb24, bmfh.bfSize - bmfh.bfOffBits, 1, BMPFile);

	fclose(BMPFile);
}

static void CopyBMP(void)
{
	BITMAPINFOHEADER bmih;
	HANDLE hMem;
	void *mem;

	ZeroMemory(&bmih, sizeof(BITMAPINFOHEADER));
	bmih.biSize = sizeof(BITMAPINFOHEADER);
	bmih.biWidth = Clip_Width;
	bmih.biHeight = Clip_Height;
	bmih.biPlanes = 1;
	bmih.biXPelsPerMeter = 0;
	bmih.biYPelsPerMeter = 0;
	bmih.biClrUsed = 0;
	bmih.biClrImportant = 0;
	bmih.biBitCount = 24;
	bmih.biCompression = BI_RGB;
	bmih.biSizeImage = ((Clip_Width*24+31)&~31)/8*Clip_Height;

	if (OpenClipboard(hWnd))
	{
		if (EmptyClipboard())
		{
			if (hMem = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, bmih.biSize + bmih.biSizeImage))
			{
				if (mem = GlobalLock(hMem))
				{
					memcpy(mem, (void *) &bmih, bmih.biSize);
					memcpy((unsigned char *) mem + bmih.biSize,	rgb24, ((Clip_Width*24+31)&~31)/8*Clip_Height);
					GlobalUnlock(mem);
					SetClipboardData(CF_DIB, hMem);
					CloseClipboard();
					return;
				}
				GlobalFree(hMem);
			}
		}
		CloseClipboard();
	}
}

static void StartupEnables(void)
{
	// This sets enabled and disabled controls for the startup condition.

	// Main menu.
	EnableMenuItem(hMenu, 0, MF_BYPOSITION | MF_ENABLED);
	EnableMenuItem(hMenu, 1, MF_BYPOSITION | MF_ENABLED);
	EnableMenuItem(hMenu, 2, MF_BYPOSITION | MF_ENABLED);
	EnableMenuItem(hMenu, 3, MF_BYPOSITION | MF_ENABLED);
	EnableMenuItem(hMenu, 4, MF_BYPOSITION | MF_ENABLED);
	EnableMenuItem(hMenu, 5, MF_BYPOSITION | MF_ENABLED);
	EnableMenuItem(hMenu, 6, MF_BYPOSITION | MF_ENABLED);

	// File menu.
	EnableMenuItem(hMenu, IDM_OPEN, MF_ENABLED);
	EnableMenuItem(hMenu, IDM_LOAD_D2V, MF_ENABLED);
	EnableMenuItem(hMenu, IDM_CLOSE, MF_GRAYED);
	EnableMenuItem(hMenu, IDM_SAVE_D2V, MF_GRAYED);
	EnableMenuItem(hMenu, IDM_SAVE_D2V_AND_DEMUX, MF_GRAYED);
	EnableMenuItem(hMenu, IDM_DEMUX_AUDIO, MF_GRAYED);
	EnableMenuItem(hMenu, IDM_BMP, MF_GRAYED);
	EnableMenuItem(hMenu, IDM_PLAY, MF_GRAYED);
	EnableMenuItem(hMenu, IDM_PREVIEW, MF_GRAYED);
	EnableMenuItem(hMenu, IDM_STOP, MF_GRAYED);
	EnableMenuItem(hMenu, IDM_PAUSE, MF_GRAYED);
	EnableMenuItem(hMenu, ID_MRU_FILE0, MF_ENABLED);
	EnableMenuItem(hMenu, ID_MRU_FILE1, MF_ENABLED);
	EnableMenuItem(hMenu, ID_MRU_FILE2, MF_ENABLED);
	EnableMenuItem(hMenu, ID_MRU_FILE3, MF_ENABLED);
	EnableMenuItem(hMenu, IDM_EXIT, MF_ENABLED);

	// Video menu.
	EnableMenuItem(hMenu, IDM_LUMINANCE, MF_GRAYED);
	EnableMenuItem(hMenu, IDM_CROPPING, MF_GRAYED);
	EnableMenuItem(hMenu, IDM_COPYFRAMETOCLIPBOARD, MF_GRAYED);

	// Audio menu.
	if (Method_Flag == AUDIO_NONE)
	{
		EnableMenuItem(GetSubMenu(hMenu, 3), 1, MF_BYPOSITION | MF_GRAYED);
		EnableMenuItem(GetSubMenu(hMenu, 3), 3, MF_BYPOSITION | MF_GRAYED);
		EnableMenuItem(GetSubMenu(hMenu, 3), 4, MF_BYPOSITION | MF_GRAYED);
		EnableMenuItem(GetSubMenu(hMenu, 3), 5, MF_BYPOSITION | MF_GRAYED);
	}
	else if (Method_Flag == AUDIO_DEMUX)
	{
		EnableMenuItem(GetSubMenu(hMenu, 3), 1, MF_BYPOSITION | MF_ENABLED);
		EnableMenuItem(GetSubMenu(hMenu, 3), 3, MF_BYPOSITION | MF_GRAYED);
		EnableMenuItem(GetSubMenu(hMenu, 3), 4, MF_BYPOSITION | MF_GRAYED);
		EnableMenuItem(GetSubMenu(hMenu, 3), 5, MF_BYPOSITION | MF_GRAYED);
	}
	else if (Method_Flag == AUDIO_DEMUXALL)
	{
		EnableMenuItem(GetSubMenu(hMenu, 3), 1, MF_BYPOSITION | MF_GRAYED);
		EnableMenuItem(GetSubMenu(hMenu, 3), 3, MF_BYPOSITION | MF_GRAYED);
		EnableMenuItem(GetSubMenu(hMenu, 3), 4, MF_BYPOSITION | MF_GRAYED);
		EnableMenuItem(GetSubMenu(hMenu, 3), 5, MF_BYPOSITION | MF_GRAYED);
	}
	else if (Method_Flag == AUDIO_DECODE)
	{
		EnableMenuItem(GetSubMenu(hMenu, 3), 1, MF_BYPOSITION | MF_ENABLED);
		EnableMenuItem(GetSubMenu(hMenu, 3), 3, MF_BYPOSITION | MF_ENABLED);
		EnableMenuItem(GetSubMenu(hMenu, 3), 4, MF_BYPOSITION | MF_ENABLED);
		EnableMenuItem(GetSubMenu(hMenu, 3), 5, MF_BYPOSITION | MF_ENABLED);
	}

	// Other menus are all enabled in the resource file.

	// Drag and drop.
	DragAcceptFiles(hWnd, true);

	// Trackbar and buttons.
	EnableWindow(hTrack, false);
	EnableWindow(hLeftButton, false);
	EnableWindow(hLeftArrow, false);
	EnableWindow(hRightArrow, false);
	EnableWindow(hRightButton, false);

	// Refresh the menu bar.
	DrawMenuBar(hWnd);
}

static void FileLoadedEnables(void)
{
	// Main menu.
	EnableMenuItem(hMenu, 0, MF_BYPOSITION | MF_ENABLED);
	if (SystemStream_Flag == TRANSPORT_STREAM)
		EnableMenuItem(hMenu, 1, MF_BYPOSITION | MF_ENABLED);
	else
		EnableMenuItem(hMenu, 1, MF_BYPOSITION | MF_GRAYED);
	EnableMenuItem(hMenu, 2, MF_BYPOSITION | MF_ENABLED);
	EnableMenuItem(hMenu, 3, MF_BYPOSITION | MF_ENABLED);
	EnableMenuItem(hMenu, 4, MF_BYPOSITION | MF_ENABLED);
	EnableMenuItem(hMenu, 5, MF_BYPOSITION | MF_ENABLED);
	EnableMenuItem(hMenu, 6, MF_BYPOSITION | MF_ENABLED);

	// File menu.
	EnableMenuItem(hMenu, IDM_OPEN, MF_ENABLED);
	EnableMenuItem(hMenu, IDM_LOAD_D2V, MF_ENABLED);
	EnableMenuItem(hMenu, IDM_CLOSE, MF_ENABLED);
	EnableMenuItem(hMenu, IDM_SAVE_D2V, MF_ENABLED);
	EnableMenuItem(hMenu, IDM_SAVE_D2V_AND_DEMUX, MF_ENABLED);
	EnableMenuItem(hMenu, IDM_DEMUX_AUDIO, MF_ENABLED);
	EnableMenuItem(hMenu, IDM_BMP, MF_ENABLED);
	EnableMenuItem(hMenu, IDM_PLAY, MF_ENABLED);
	EnableMenuItem(hMenu, IDM_PREVIEW, MF_ENABLED);
	EnableMenuItem(hMenu, IDM_STOP, MF_GRAYED);
	EnableMenuItem(hMenu, IDM_PAUSE, MF_GRAYED);
	EnableMenuItem(hMenu, ID_MRU_FILE0, MF_ENABLED);
	EnableMenuItem(hMenu, ID_MRU_FILE1, MF_ENABLED);
	EnableMenuItem(hMenu, ID_MRU_FILE2, MF_ENABLED);
	EnableMenuItem(hMenu, ID_MRU_FILE3, MF_ENABLED);
	EnableMenuItem(hMenu, IDM_EXIT, MF_ENABLED);

	// Video menu.
	EnableMenuItem(hMenu, IDM_LUMINANCE, MF_ENABLED);
	EnableMenuItem(hMenu, IDM_CROPPING, MF_ENABLED);
	EnableMenuItem(hMenu, IDM_COPYFRAMETOCLIPBOARD, MF_ENABLED);

	// Drag and drop.
	DragAcceptFiles(hWnd, true);

	// Trackbar and buttons.
	EnableWindow(hTrack, true);
	EnableWindow(hLeftButton, true);
	EnableWindow(hLeftArrow, true);
	EnableWindow(hRightArrow, true);
	EnableWindow(hRightButton, true);

	// Refresh the menu bar.
	DrawMenuBar(hWnd);
}

static void RunningEnables(void)
{
	// Main menu.
	EnableMenuItem(hMenu, 0, MF_BYPOSITION | MF_ENABLED);
	EnableMenuItem(hMenu, 1, MF_BYPOSITION | MF_GRAYED);
	EnableMenuItem(hMenu, 2, MF_BYPOSITION | MF_GRAYED);
	EnableMenuItem(hMenu, 3, MF_BYPOSITION | MF_GRAYED);
	EnableMenuItem(hMenu, 4, MF_BYPOSITION | MF_ENABLED);
	EnableMenuItem(hMenu, 5, MF_BYPOSITION | MF_GRAYED);
	EnableMenuItem(hMenu, 6, MF_BYPOSITION | MF_ENABLED);

	// File menu.
	EnableMenuItem(hMenu, IDM_OPEN, MF_GRAYED);
	EnableMenuItem(hMenu, IDM_LOAD_D2V, MF_GRAYED);
	EnableMenuItem(hMenu, IDM_CLOSE, MF_GRAYED);
	EnableMenuItem(hMenu, IDM_SAVE_D2V, MF_GRAYED);
	EnableMenuItem(hMenu, IDM_SAVE_D2V_AND_DEMUX, MF_GRAYED);
	EnableMenuItem(hMenu, IDM_DEMUX_AUDIO, MF_GRAYED);
	EnableMenuItem(hMenu, IDM_BMP, MF_ENABLED);
	EnableMenuItem(hMenu, IDM_PLAY, MF_GRAYED);
	EnableMenuItem(hMenu, IDM_PREVIEW, MF_GRAYED);
	EnableMenuItem(hMenu, IDM_STOP, MF_ENABLED);
	EnableMenuItem(hMenu, IDM_PAUSE, MF_ENABLED);
	EnableMenuItem(hMenu, ID_MRU_FILE0, MF_GRAYED);
	EnableMenuItem(hMenu, ID_MRU_FILE1, MF_GRAYED);
	EnableMenuItem(hMenu, ID_MRU_FILE2, MF_GRAYED);
	EnableMenuItem(hMenu, ID_MRU_FILE3, MF_GRAYED);
	EnableMenuItem(hMenu, IDM_EXIT, MF_ENABLED);

    // Video menu.
	EnableMenuItem(hMenu, IDM_COPYFRAMETOCLIPBOARD, MF_ENABLED);

	// Drag and drop.
	DragAcceptFiles(hWnd, false);

	// Trackbar and buttons.
	EnableWindow(hTrack, false);
	EnableWindow(hLeftButton, false);
	EnableWindow(hLeftArrow, false);
	if (PlaybackSpeed == SPEED_SINGLE_STEP)
		EnableWindow(hRightArrow, true);
	else
		EnableWindow(hRightArrow, false);
	EnableWindow(hRightButton, false);

	// Refresh the menu bar.
	DrawMenuBar(hWnd);
}

void UpdateWindowText(void)
{
	char *ext;
	char szTemp[DG_MAX_PATH];

	if (timing.op)
	{
		float percent;
		timing.ed = timeGetTime();
		elapsed = (timing.ed-timing.op)/1000;
		percent = (float)(100.0*(process.run-process.start+_ftelli64(Infile[CurrentFile]))/(process.end-process.start));
		remain = (int)((timing.ed-timing.op)*(100.0-percent)/percent)/1000;

		sprintf(szBuffer, "%d:%02d:%02d", elapsed/3600, (elapsed%3600)/60, elapsed%60);
		SetDlgItemText(hDlg, IDC_ELAPSED, szBuffer);

		sprintf(szBuffer, "%d:%02d:%02d", remain/3600, (remain%3600)/60, remain%60);
		SetDlgItemText(hDlg, IDC_REMAIN, szBuffer);
	}
	else
		remain = 0;

	if (remain && process.locate == LOCATE_RIP || process.locate == LOCATE_PLAY || process.locate == LOCATE_DEMUX_AUDIO)
	{
		if (elapsed + remain)
		{
			sprintf(szBuffer, "DGIndex[%d%%] - ", (elapsed * 100) / (elapsed + remain));
			if(bIsWindowsXPorLater)
				PostMessage(hWnd, PROGRESS_MESSAGE, (elapsed * 100) / (elapsed + remain), 0);
		}
		else
		{
			sprintf(szBuffer, "DGIndex[0%%] - ");
			if(bIsWindowsXPorLater)
				PostMessage(hWnd, PROGRESS_MESSAGE, 0, 0);
		}
	}
	else
		sprintf(szBuffer, "DGIndex - ");
	ext = strrchr(Infilename[CurrentFile], '\\');
	if (ext)
		strncat(szBuffer, ext+1, strlen(Infilename[CurrentFile])-(int)(ext-Infilename[CurrentFile]));
	else
		strcat(szBuffer, Infilename[CurrentFile]);
	sprintf(szTemp, " [%dx%d] [File %d/%d]", Clip_Width, Clip_Height, CurrentFile+1, NumLoadedFiles);
	strcat(szBuffer, szTemp);
	if (VOB_ID && CELL_ID)
	{
		sprintf(szTemp, " [Vob %d] [Cell %d]", VOB_ID, CELL_ID);
		strcat(szBuffer, szTemp);
	}
	SetWindowText(hWnd, szBuffer);
}

void OutputProgress(int progr)
{
	static int lastprogress = -1;

	if (progr != lastprogress)
	{
		char percent[20];
		DWORD written;

		sprintf(percent, "%d\n", progr);
		WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), percent, strlen(percent), &written, NULL);
		lastprogress = progr;
	}
}

void DeleteMRUList(int index)
{
    for (; index < 3; index++)
    {
        strcpy(mMRUList[index], mMRUList[index+1]);
    }
    mMRUList[3][0] = 0;
    UpdateMRUList();
}

void AddMRUList(char *name)
{
    int i;

    // Is the name in the list?
    for (i = 0; i < 4; i++)
    {
        if (!strcmp(mMRUList[i], name))
            break;
    }
    if (i == 4)
    {
        // New name, add it to the list.
        for (i = 3; i > 0; i--)
            strcpy(mMRUList[i], mMRUList[i-1]);
        strcpy(mMRUList[0], name);
    }
    else
    {
        // Name exists, move it to the top.
        strcpy(name, mMRUList[i]);
        for (; i > 0; i--)
            strcpy(mMRUList[i], mMRUList[i-1]);
        strcpy(mMRUList[0], name);
    }
}

void UpdateMRUList(void)
{
	HMENU hmenuFile = GetSubMenu(GetMenu((HWND)hWnd), 0);
	MENUITEMINFO m;
	char name[DG_MAX_PATH];
	int index = 0;
#define MRU_LIST_POSITION 14

	memset(&m, 0, sizeof m);
	m.cbSize	= sizeof(MENUITEMINFO);
	for(;;) {
		m.fMask			= MIIM_TYPE;
		m.dwTypeData	= name;
		m.cch			= sizeof name;

		if (!GetMenuItemInfo(hmenuFile, MRU_LIST_POSITION, TRUE, &m)) break;

		if (m.fType & MFT_SEPARATOR) break;

		RemoveMenu(hmenuFile, MRU_LIST_POSITION, MF_BYPOSITION);
	}

	for (;;)
    {
        char path[DG_MAX_PATH];

        if (!mMRUList[index][0])
            break;

        strcpy(path, mMRUList[index]);
        PathCompactPath(GetDC(hWnd), (LPSTR) path, 320);

		m.fMask		= MIIM_TYPE | MIIM_STATE | MIIM_ID;
		m.fType		= MFT_STRING;
		m.fState	= MFS_ENABLED;
		m.dwTypeData	= path;
		m.cch		= DG_MAX_PATH;
		m.wID		= ID_MRU_FILE0 + index;

		if (!InsertMenuItem(hmenuFile, MRU_LIST_POSITION+index, TRUE, &m))
            break;
		if (++index >= 4)
            break;
	}

	if (!index) {
		m.fMask			= MIIM_TYPE | MIIM_STATE | MIIM_ID;
		m.fType			= MFT_STRING;
		m.fState		= MFS_GRAYED;
		m.wID			= ID_MRU_FILE0;
		m.dwTypeData	= "Recent file list";
		m.cch			= DG_MAX_PATH;

		InsertMenuItem(hmenuFile, MRU_LIST_POSITION+index, TRUE, &m);
	}

	DrawMenuBar((HWND)hWnd);
}
