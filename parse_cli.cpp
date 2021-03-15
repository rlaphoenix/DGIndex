#include <windows.h>
#include "resource.h"
#include "Shlwapi.h"
#include "global.h"

int parse_cli(LPSTR lpCmdLine, LPSTR ucCmdLine)
{
	char cwd[DG_MAX_PATH];
	int i;

//	if (!strstr(lpCmdLine, "="))
	{
		// UNIX-style CLI.
		char *p = lpCmdLine, *q;
		char *f, name[DG_MAX_PATH], *ptr;
		int in_quote = 0;
		FILE *tmp;
		int enable_demux = 0;
		char opt[32], *o;
		char suffix[DG_MAX_PATH];
		int val;

		// CLI invocation.
		NumLoadedFiles = 0;
		CLIPreview = 0;
		ExitOnEnd = 0;
		hadRGoption = 0;

		while (1)
		{
			while (*p == ' ' || *p == '\t') p++;
			if (*p == '-')
			{
				p++;
				o = opt;
				while (*p != ' ' && *p != '\t' && *p != 0)
					*o++ = *p++;
				*o = 0;
				if (!strncmp(opt, "i", 3))
				{
another:
					while (*p == ' ' || *p == '\t') p++;
					f = name;
					while (1)
					{
						if ((in_quote == 0) && (*p == ' ' || *p == '\t' || *p == 0))
							break;
						if ((in_quote == 1) && (*p == 0))
							break;
						if (*p == '"')
						{
							if (in_quote == 0)
							{
								in_quote = 1;
								p++;
							}
							else
							{
								in_quote = 0;
								p++;
								break;
							}
						}
						*f++ = *p++;
					}
					*f = 0;
					/* If the specified file does not include a path, use the
					   current directory. */
					if (name[0] != '\\' && name[1] != ':')
					{
						GetCurrentDirectory(sizeof(cwd) - 1, cwd);
						strcat(cwd, "\\");
						strcat(cwd, name);
					}
					else
					{
						strcpy(cwd, name);
					}
					if ((tmp = fopen(cwd, "rb")) != NULL)
					{
						strcpy(Infilename[NumLoadedFiles], cwd);
						Infile[NumLoadedFiles] = tmp;
						NumLoadedFiles++;
					}
					if (*p != 0 && *(p+1) != '-')
						goto another;
					Recovery();
					RefreshWindow(true);
				}
				if (!strncmp(opt, "ai", 3))
				{
					while (*p == ' ' || *p == '\t') p++;
					f = name;
					while (1)
					{
						if ((in_quote == 0) && (*p == ' ' || *p == '\t' || *p == 0))
							break;
						if ((in_quote == 1) && (*p == 0))
							break;
						if (*p == '"')
						{
							if (in_quote == 0)
							{
								in_quote = 1;
								p++;
							}
							else
							{
								in_quote = 0;
								p++;
								break;
							}
						}
						*f++ = *p++;
					}
					*f = 0;
					for (;;)
					{
						/* If the specified file does not include a path, use the
						   current directory. */
						if (name[0] != '\\' && name[1] != ':')
						{
							GetCurrentDirectory(sizeof(cwd) - 1, cwd);
							strcat(cwd, "\\");
							strcat(cwd, name);
						}
						else
						{
							strcpy(cwd, name);
						}
						if ((tmp = fopen(cwd, "rb")) == NULL)
							break;
						strcpy(Infilename[NumLoadedFiles], cwd);
						Infile[NumLoadedFiles] = tmp;
						NumLoadedFiles++;

						// First scan back from the end of the name for an _ character.
						ptr = name + strlen(name);
						while (*ptr != '_' && ptr >= name) ptr--;
						if (*ptr != '_') break;
						// Now pick up the number value and increment it.
						ptr++;
						if (*ptr < '0' || *ptr > '9') break;
						sscanf(ptr, "%d", &val);
						val++;
						// Save the suffix after the number.
						q = ptr;
						while (*ptr >= '0' && *ptr <= '9') ptr++;
						strcpy(suffix, ptr);
						// Write the new incremented number.
						sprintf(q, "%d", val);
						// Append the saved suffix.
						strcat(name, suffix);
					}
					Recovery();
					RefreshWindow(true);
				}
				else if (!strncmp(opt, "rg", 3))
				{
					while (*p == ' ' || *p == '\t') p++;
					sscanf(p, "%d %I64x %d %I64x\n",
						&process.leftfile, &process.leftlba, &process.rightfile, &process.rightlba);
					while (*p != '-' && *p != 0) p++;
					p--;

					process.startfile = process.leftfile;
					process.startloc = process.leftlba * SECTOR_SIZE;
					process.endfile = process.rightfile;
					process.endloc = (process.rightlba - 1) * SECTOR_SIZE;

					process.run = 0;
					for (i=0; i<process.leftfile; i++)
						process.run += Infilelength[i];
					process.trackleft = ((process.run + process.leftlba * SECTOR_SIZE) * TRACK_PITCH / Infiletotal);

					process.run = 0;
					for (i=0; i<process.rightfile; i++)
						process.run += Infilelength[i];
					process.trackright = ((process.run + (__int64)process.rightlba*SECTOR_SIZE)*TRACK_PITCH/Infiletotal);

					process.end = 0;
					for (i=0; i<process.endfile; i++)
						process.end += Infilelength[i];
					process.end += process.endloc;

					hadRGoption = 1;
				}
				else if (!strncmp(opt, "vp", 3))
				{
					while (*p == ' ' || *p == '\t') p++;
					sscanf(p, "%x", &MPEG2_Transport_VideoPID);
					while (*p != '-' && *p != 0) p++;
					p--;
				}
				else if (!strncmp(opt, "ap", 3))
				{
					while (*p == ' ' || *p == '\t') p++;
					sscanf(p, "%x", &MPEG2_Transport_AudioPID);
					while (*p != '-' && *p != 0) p++;
					p--;
				}
				else if (!strncmp(opt, "pp", 3))
				{
					while (*p == ' ' || *p == '\t') p++;
					sscanf(p, "%x", &MPEG2_Transport_PCRPID);
					while (*p != '-' && *p != 0) p++;
					p--;
				}
				else if (!strncmp(opt, "ia", 3))
				{
					CheckMenuItem(hMenu, IDM_IDCT_MMX, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_IDCT_SSEMMX, MF_UNCHECKED);
 					CheckMenuItem(hMenu, IDM_IDCT_SSE2MMX, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_IDCT_FPU, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_IDCT_REF, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_IDCT_SKAL, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_IDCT_SIMPLE, MF_UNCHECKED);
					while (*p == ' ' || *p == '\t') p++;
		   
					switch (*p++)
					{
					case '1':
					  iDCT_Flag = IDCT_MMX;
					  break;
					case '2':
					  iDCT_Flag = IDCT_SSEMMX;
					  break;
					default:
					case '3':
					  iDCT_Flag = IDCT_SSE2MMX;
					  break;
					case '4':
					  iDCT_Flag = IDCT_FPU;
					  break;
					case '5':
					  iDCT_Flag = IDCT_REF;
					  break;
					case '6':
					  iDCT_Flag = IDCT_SKAL;
					  break;
					case '7':
					  iDCT_Flag = IDCT_SIMPLE;
					  break;
					}
				}
				else if (!strncmp(opt, "fo", 3))
				{
					CheckMenuItem(hMenu, IDM_FO_NONE, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_FO_FILM, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_FO_RAW, MF_UNCHECKED);
					SetDlgItemText(hDlg, IDC_INFO, "");
					while (*p == ' ' || *p == '\t') p++;

					switch (*p++)
					{
					default:
					case '0':
					  FO_Flag = FO_NONE;
					  CheckMenuItem(hMenu, IDM_FO_NONE, MF_CHECKED);
					  break;
					case '1':
					  FO_Flag = FO_FILM;
					  CheckMenuItem(hMenu, IDM_FO_FILM, MF_CHECKED);
					  break;
					case '2':
					  FO_Flag = FO_RAW;
					  CheckMenuItem(hMenu, IDM_FO_RAW, MF_CHECKED);
					  break;
					}
				}
				else if (!strncmp(opt, "yr", 3))
				{
					CheckMenuItem(hMenu, IDM_TVSCALE, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_PCSCALE, MF_UNCHECKED);
					while (*p == ' ' || *p == '\t') p++;
		    
					switch (*p++)
					{
					default:
					case '1':
					  Scale_Flag = true;
					  setRGBValues();    
					  CheckMenuItem(hMenu, IDM_PCSCALE, MF_CHECKED);
					  break;
		      
					case '2':
					  Scale_Flag = false;
					  setRGBValues();
					  CheckMenuItem(hMenu, IDM_TVSCALE, MF_CHECKED);
					  break;
					}
				}
				else if (!strncmp(opt, "tn", 3))
				{
					char track_list[1024];
					unsigned int i, audio_id;
					// First get the track list into Track_List.
					while (*p == ' ' || *p == '\t') p++;
					strcpy(track_list, p);
					while (*p != '-' && *p != 0) p++;
					ptr = track_list;
					while (*ptr != ' ' && *ptr != 0)
						ptr++;
					*ptr = 0;
					strcpy(Track_List, track_list);
					// Now parse it and enable the specified audio ids for demuxing.
					for (i = 0; i < 0xc8; i++)
						audio[i].selected_for_demux = false;
					ptr = Track_List;
					while ((*ptr >= '0' && *ptr <= '9') || (*ptr >= 'a' && *ptr <= 'f') || (*ptr >= 'A' && *ptr <= 'F'))
					{
						sscanf(ptr, "%x", &audio_id);
						if (audio_id > 0xc7)
							break;
						audio[audio_id].selected_for_demux = true;
						while (*ptr != ',' && *ptr != 0) ptr++;
						if (*ptr == 0)
							break;
						ptr++;
					}

					Method_Flag = AUDIO_DEMUX;
					CheckMenuItem(hMenu, IDM_AUDIO_NONE, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_DEMUX, MF_CHECKED);
					CheckMenuItem(hMenu, IDM_DEMUXALL, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_DECODE, MF_UNCHECKED);
					EnableMenuItem(GetSubMenu(hMenu, 3), 1, MF_BYPOSITION | MF_ENABLED);
					EnableMenuItem(GetSubMenu(hMenu, 3), 3, MF_BYPOSITION | MF_GRAYED);
					EnableMenuItem(GetSubMenu(hMenu, 3), 4, MF_BYPOSITION | MF_GRAYED);
					EnableMenuItem(GetSubMenu(hMenu, 3), 5, MF_BYPOSITION | MF_GRAYED);
				}
				else if (!strncmp(opt, "om", 3))
				{
					CheckMenuItem(hMenu, IDM_AUDIO_NONE, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_DEMUX, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_DEMUXALL, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_DECODE, MF_UNCHECKED);
					while (*p == ' ' || *p == '\t') p++;

					switch (*p++)
					{
					default:
					case '0':
						Method_Flag = AUDIO_NONE;
						CheckMenuItem(hMenu, IDM_AUDIO_NONE, MF_CHECKED);
						EnableMenuItem(GetSubMenu(hMenu, 3), 1, MF_BYPOSITION | MF_GRAYED);
						EnableMenuItem(GetSubMenu(hMenu, 3), 3, MF_BYPOSITION | MF_GRAYED);
						EnableMenuItem(GetSubMenu(hMenu, 3), 4, MF_BYPOSITION | MF_GRAYED);
						EnableMenuItem(GetSubMenu(hMenu, 3), 5, MF_BYPOSITION | MF_GRAYED);
						break;
		      
					case '1':
						Method_Flag = AUDIO_DEMUX;
						CheckMenuItem(hMenu, IDM_DEMUX, MF_CHECKED);
						EnableMenuItem(GetSubMenu(hMenu, 3), 1, MF_BYPOSITION | MF_ENABLED);
						EnableMenuItem(GetSubMenu(hMenu, 3), 3, MF_BYPOSITION | MF_GRAYED);
						EnableMenuItem(GetSubMenu(hMenu, 3), 4, MF_BYPOSITION | MF_GRAYED);
						EnableMenuItem(GetSubMenu(hMenu, 3), 5, MF_BYPOSITION | MF_GRAYED);
						break;
		     
					case '2':
						Method_Flag = AUDIO_DEMUXALL;
						CheckMenuItem(hMenu, IDM_DEMUXALL, MF_CHECKED);
						EnableMenuItem(GetSubMenu(hMenu, 3), 1, MF_BYPOSITION | MF_GRAYED);
						EnableMenuItem(GetSubMenu(hMenu, 3), 3, MF_BYPOSITION | MF_GRAYED);
						EnableMenuItem(GetSubMenu(hMenu, 3), 4, MF_BYPOSITION | MF_GRAYED);
						EnableMenuItem(GetSubMenu(hMenu, 3), 5, MF_BYPOSITION | MF_GRAYED);
						break;
		     
					case '3':
						Method_Flag = AUDIO_DECODE;
						CheckMenuItem(hMenu, IDM_DECODE, MF_CHECKED);
						EnableMenuItem(GetSubMenu(hMenu, 3), 1, MF_BYPOSITION | MF_ENABLED);
						EnableMenuItem(GetSubMenu(hMenu, 3), 3, MF_BYPOSITION | MF_ENABLED);
						EnableMenuItem(GetSubMenu(hMenu, 3), 4, MF_BYPOSITION | MF_ENABLED);
						EnableMenuItem(GetSubMenu(hMenu, 3), 5, MF_BYPOSITION | MF_ENABLED);
						break;
					}
				}
				else if (!strncmp(opt, "drc", 3))
				{
					CheckMenuItem(hMenu, IDM_DRC_NONE, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_DRC_LIGHT, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_DRC_NORMAL, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_DRC_HEAVY, MF_UNCHECKED);
					while (*p == ' ' || *p == '\t') p++;

					switch (*p++)
					{
					default:
					case '0':
					  CheckMenuItem(hMenu, IDM_DRC_NONE, MF_CHECKED);
					  DRC_Flag = DRC_NONE;
					  break;
					case '1':
					  CheckMenuItem(hMenu, IDM_DRC_LIGHT, MF_CHECKED);
					  DRC_Flag = DRC_LIGHT;
					  break;
					case '2':
					  CheckMenuItem(hMenu, IDM_DRC_NORMAL, MF_CHECKED);
					  DRC_Flag = DRC_NORMAL;
					  break;
					case '3':
					  CheckMenuItem(hMenu, IDM_DRC_HEAVY, MF_CHECKED);
					  DRC_Flag = DRC_HEAVY;
					  break;
					}
				}
				else if (!strncmp(opt, "dsd", 3))
				{
					CheckMenuItem(hMenu, IDM_DSDOWN, MF_UNCHECKED);
					while (*p == ' ' || *p == '\t') p++;

					DSDown_Flag = *p++ - '0';
					if (DSDown_Flag)
					  CheckMenuItem(hMenu, IDM_DSDOWN, MF_CHECKED);
				}
				else if (!strncmp(opt, "dsa", 3))
				{
					CheckMenuItem(hMenu, IDM_SRC_NONE, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_SRC_LOW, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_SRC_MID, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_SRC_HIGH, MF_UNCHECKED);
					CheckMenuItem(hMenu, IDM_SRC_UHIGH, MF_UNCHECKED);
					while (*p == ' ' || *p == '\t') p++;

					switch (*p++)
					{
					default:
					case '0':
						SRC_Flag = SRC_NONE;
						CheckMenuItem(hMenu, IDM_SRC_NONE, MF_CHECKED);
						break;
					case '1':
						SRC_Flag = SRC_LOW;
						CheckMenuItem(hMenu, IDM_SRC_LOW, MF_CHECKED);
						break;
					case '2':
						SRC_Flag = SRC_MID;
						CheckMenuItem(hMenu, IDM_SRC_MID, MF_CHECKED);
						break;
					case '3':
						SRC_Flag = SRC_HIGH;
						CheckMenuItem(hMenu, IDM_SRC_HIGH, MF_CHECKED);
						break;
					case '4':
						SRC_Flag = SRC_UHIGH;
						CheckMenuItem(hMenu, IDM_SRC_UHIGH, MF_CHECKED);
						break;
					}
				}
				else if (!strncmp(opt, "pre", 3))
				{
					CLIPreview = 1;
				}
				else if (!strncmp(opt, "at", 3))
				{
					FILE *bf;

					while (*p == ' ' || *p == '\t') p++;
					if (*p == '-' || *p == 0)
					{
						// A null file name specifies no template.
						AVSTemplatePath[0] = 0;
						p--;
					}
					else
					{
						f = name;
						while (1)
						{
							if ((in_quote == 0) && (*p == ' ' || *p == '\t' || *p == 0))
								break;
							if ((in_quote == 1) && (*p == 0))
								break;
							if (*p == '"')
							{
								if (in_quote == 0)
								{
									in_quote = 1;
									p++;
								}
								else
								{
									in_quote = 0;
									p++;
									break;
								}
							}
							*f++ = *p++;
						}
						*f = 0;
						/* If the specified file does not include a path, use the
						   current directory. */
						if (name[0] != '\\' && name[1] != ':')
						{
							GetCurrentDirectory(sizeof(cwd) - 1, cwd);
							strcat(cwd, "\\");
							strcat(cwd, name);
						}
						else
						{
							strcpy(cwd, name);
						}
						// Check that the specified template file exists and is readable.
						bf = fopen(cwd, "r");
						if (bf != 0)
						{
							// Looks good; save the path.
							fclose(bf);
							strcpy(AVSTemplatePath, cwd);
						}
						else
						{
							// Something is wrong, so don't use a template.
							AVSTemplatePath[0] = 0;
						}
					}
				}
				else if (!strncmp(opt, "exit", 4))
				{
					ExitOnEnd = 1;
				}
				else if (!strncmp(opt, "o", 3) || !strncmp(opt, "od", 3))
				{
					// Set up demuxing if requested.
					if (p[-1] == 'd')
					{
						MuxFile = (FILE *) 0;
					}
					while (*p == ' ' || *p == '\t') p++;

					// Don't pop up warning boxes for automatic invocation.
					crop1088_warned = true;
					CLIActive = 1;
					f = name;
					while (1)
					{
						if ((in_quote == 0) && (*p == ' ' || *p == '\t' || *p == 0))
							break;
						if ((in_quote == 1) && (*p == 0))
							break;
						if (*p == '"')
						{
							if (in_quote == 0)
							{
								in_quote = 1;
								p++;
							}
							else
							{
								in_quote = 0;
								p++;
								break;
							}
						}
						*f++ = *p++;
					}
					*f = 0;
					/* If the specified file does not include a path, use the
					   current directory. */
					if (name[0] != '\\' && name[1] != ':')
					{
						GetCurrentDirectory(sizeof(szOutput) - 1, szOutput);
						strcat(szOutput, "\\");
						strcat(szOutput, name);
					}
					else
					{
						strcpy(szOutput, name);
					}
				}
			}
			else if (*p == 0)
				break;
			else
				break;
		}
		if (NumLoadedFiles == 0 && WindowMode == SW_HIDE)
		{
			MessageBoxTimeout(hWnd, "Couldn't open input file in HIDE mode! Exiting...", NULL, MB_OK | MB_ICONERROR, 0, CLIActive ? CLITimeout : 0xffffffff);
			return -1;
		}
		if (!CLIActive && WindowMode == SW_HIDE)
		{
			MessageBoxTimeout(hWnd, "No output file in HIDE mode! Exiting...", NULL, MB_OK | MB_ICONERROR, 0, CLIActive ? CLITimeout : 0xffffffff);
			return -1;
		}
		CheckFlag();
	}
#if 0
	else if(*lpCmdLine != 0)
	{
		// Old-style CLI.
		FILE *tmp;
        int hadRGoption = 0;
		char delimiter1[2], delimiter2[2];
		char *ende, save;
		char *ptr, *fptr, *p, *q;
		char aFName[DG_MAX_PATH];
		char suffix[DG_MAX_PATH];
		int val;

		NumLoadedFiles = 0;
		delimiter1[0] = '[';
		delimiter2[0] = ']';
		delimiter1[1] = delimiter2[1] = 0;
		if ((ptr = strstr(ucCmdLine,"-SET-DELIMITER=")) || (ptr = strstr(ucCmdLine,"-SD=")))
		{
			ptr = lpCmdLine + (ptr - ucCmdLine);
            while (*ptr++ != '=');
			delimiter1[0] = delimiter2[0] = *ptr;
		}
		if ((ptr = strstr(ucCmdLine,"-AUTO-INPUT-FILES=")) || (ptr = strstr(ucCmdLine,"-AIF=")))
		{
			ptr = lpCmdLine + (ptr - ucCmdLine);
			ptr  = strstr(ptr, delimiter1) + 1;
			ende = strstr(ptr + 1, delimiter2);
			save = *ende;
			*ende = 0;
			strcpy(aFName, ptr);
			*ende = save;
			for (;;)
			{
				/* If the specified file does not include a path, use the
				   current directory. */
				if (!strstr(aFName, "\\"))
				{
					GetCurrentDirectory(sizeof(cwd) - 1, cwd);
					strcat(cwd, "\\");
					strcat(cwd, aFName);
				}
				else
				{
					strcpy(cwd, aFName);
				}
				if ((tmp = fopen(cwd, "rb")) == NULL) break;
				strcpy(Infilename[NumLoadedFiles], cwd);
				Infile[NumLoadedFiles] = tmp;
				NumLoadedFiles++;

				// First scan back from the end of the name for an _ character.
				p = aFName+strlen(aFName);
				while (*p != '_' && p >= aFName) p--;
				if (*p != '_') break;
				// Now pick up the number value and increment it.
				p++;
				if (*p < '0' || *p > '9') break;
				sscanf(p, "%d", &val);
				val++;
				// Save the suffix after the number.
				q = p;
				while (*p >= '0' && *p <= '9') p++;
				strcpy(suffix, p);
				// Write the new incremented number.
				sprintf(q, "%d", val);
				// Append the saved suffix.
				strcat(aFName, suffix);
			}
		}
		else if ((ptr = strstr(ucCmdLine,"-INPUT-FILES=")) || (ptr = strstr(ucCmdLine,"-IF=")))
		{
		  ptr = lpCmdLine + (ptr - ucCmdLine);
		  ptr  = strstr(ptr, delimiter1) + 1;
		  ende = strstr(ptr + 1, delimiter2);
  
		  do
		  {
			i = 0;
			if ((fptr = strstr(ptr, ",")) || (fptr = strstr(ptr + 1, delimiter2)))
			{
			  while (ptr < fptr)
			  {
				aFName[i] = *ptr;
				ptr++;
				i++;
			  }
			  aFName[i] = 0x00;
			  ptr++;
			}
			else
			{
			  strcpy(aFName, ptr);
			  ptr = ende;
			}

			/* If the specified file does not include a path, use the
			   current directory. */
			if (!strstr(aFName, "\\"))
			{
				GetCurrentDirectory(sizeof(cwd) - 1, cwd);
				strcat(cwd, "\\");
				strcat(cwd, aFName);
			}
			else
			{
				strcpy(cwd, aFName);
			}
			if ((tmp = fopen(cwd, "rb")) != NULL)
			{
				strcpy(Infilename[NumLoadedFiles], cwd);
				Infile[NumLoadedFiles] = tmp;
				NumLoadedFiles++;
			}
		  }
		  while (ptr < ende);
		}
		else if ((ptr = strstr(ucCmdLine,"-BATCH-FILES=")) || (ptr = strstr(ucCmdLine,"-BF=")))
		{
			FILE *bf;
			char line[1024];

			ptr = lpCmdLine + (ptr - ucCmdLine);
			ptr  = strstr(ptr, delimiter1) + 1;
			ende = strstr(ptr + 1, delimiter2);
			save = *ende;
			*ende = 0;
			strcpy(aFName, ptr);
			*ende = save;
			/* If the specified batch file does not include a path, use the
			   current directory. */
			if (!strstr(aFName, "\\"))
			{
				GetCurrentDirectory(sizeof(cwd) - 1, cwd);
				strcat(cwd, "\\");
				strcat(cwd, aFName);
			}
			else
			{
				strcpy(cwd, aFName);
			}
			bf = fopen(cwd, "r");
			if (bf != 0)
			{
				while (fgets(line, 1023, bf) != 0)
				{
					// Zap the newline.
					line[strlen(line)-1] = 0;
					/* If the specified batch file does not include a path, use the
					   current directory. */
					if (!strstr(line, "\\"))
					{
						GetCurrentDirectory(sizeof(cwd) - 1, cwd);
						strcat(cwd, "\\");
						strcat(cwd, line);
					}
					else
					{
						strcpy(cwd, line);
					}
					if ((tmp = fopen(cwd, "rb")) != NULL)
					{
						strcpy(Infilename[NumLoadedFiles], cwd);
						Infile[NumLoadedFiles] = tmp;
						NumLoadedFiles++;
					}
				}
			}
		}
		Recovery();
		if ((ptr = strstr(ucCmdLine,"-RANGE=")) || (ptr = strstr(ucCmdLine,"-RG=")))
		{
            ptr = lpCmdLine + (ptr - ucCmdLine);
            while (*ptr++ != '=');
  
            sscanf(ptr, "%d/%I64x/%d/%I64x\n",
	            &process.leftfile, &process.leftlba, &process.rightfile, &process.rightlba);

		    process.startfile = process.leftfile;
		    process.startloc = process.leftlba * SECTOR_SIZE;
		    process.endfile = process.rightfile;
		    process.endloc = (process.rightlba - 1) * SECTOR_SIZE;

			process.run = 0;
			for (i=0; i<process.leftfile; i++)
				process.run += Infilelength[i];
			process.trackleft = ((process.run + process.leftlba * SECTOR_SIZE) * TRACK_PITCH / Infiletotal);

            process.run = 0;
			for (i=0; i<process.rightfile; i++)
				process.run += Infilelength[i];
			process.trackright = ((process.run + (__int64)process.rightlba*SECTOR_SIZE)*TRACK_PITCH/Infiletotal);

		    process.end = 0;
		    for (i=0; i<process.endfile; i++)
			    process.end += Infilelength[i];
		    process.end += process.endloc;

//			SendMessage(hTrack, TBM_SETSEL, (WPARAM) true, (LPARAM) MAKELONG(process.trackleft, process.trackright));
            hadRGoption = 1;
        }

		if (NumLoadedFiles == 0 && WindowMode == SW_HIDE)
		{
			MessageBoxTimeout(hWnd, "Couldn't open input file in HIDE mode! Exiting...", NULL, MB_OK | MB_ICONERROR, 0, CLIActive ? CLITimeout : 0xffffffff);
			return -1;
		}

		// Transport PIDs
		if ((ptr = strstr(ucCmdLine,"-VIDEO-PID=")) || (ptr = strstr(ucCmdLine,"-VP=")))
		{
			ptr = lpCmdLine + (ptr - ucCmdLine);
			sscanf(strstr(ptr,"=")+1, "%x", &MPEG2_Transport_VideoPID);
		}
		if ((ptr = strstr(ucCmdLine,"-AUDIO-PID=")) || (ptr = strstr(ucCmdLine,"-AP=")))
		{
			ptr = lpCmdLine + (ptr - ucCmdLine);
			sscanf(strstr(ptr,"=")+1, "%x", &MPEG2_Transport_AudioPID);
		}
		if ((ptr = strstr(ucCmdLine,"-PCR-PID=")) || (ptr = strstr(ucCmdLine,"-PP=")))
		{
			ptr = lpCmdLine + (ptr - ucCmdLine);
			sscanf(strstr(ptr,"=")+1, "%x", &MPEG2_Transport_PCRPID);
		}

		//iDCT Algorithm
		if ((ptr = strstr(ucCmdLine,"-IDCT-ALGORITHM=")) || (ptr = strstr(ucCmdLine,"-IA=")))
		{
			ptr = lpCmdLine + (ptr - ucCmdLine);
			CheckMenuItem(hMenu, IDM_IDCT_MMX, MF_UNCHECKED);
			CheckMenuItem(hMenu, IDM_IDCT_SSEMMX, MF_UNCHECKED);
 			CheckMenuItem(hMenu, IDM_IDCT_SSE2MMX, MF_UNCHECKED);
			CheckMenuItem(hMenu, IDM_IDCT_FPU, MF_UNCHECKED);
			CheckMenuItem(hMenu, IDM_IDCT_REF, MF_UNCHECKED);
			CheckMenuItem(hMenu, IDM_IDCT_SKAL, MF_UNCHECKED);
			CheckMenuItem(hMenu, IDM_IDCT_SIMPLE, MF_UNCHECKED);
   
			switch (*(strstr(ptr,"=")+1))
			{
			case '1':
			  iDCT_Flag = IDCT_MMX;
			  break;
			case '2':
			  iDCT_Flag = IDCT_SSEMMX;
			  break;
			default:
			case '3':
			  iDCT_Flag = IDCT_SSE2MMX;
			  break;
			case '4':
			  iDCT_Flag = IDCT_FPU;
			  break;
			case '5':
			  iDCT_Flag = IDCT_REF;
			  break;
			case '6':
			  iDCT_Flag = IDCT_SKAL;
			  break;
			case '7':
			  iDCT_Flag = IDCT_SIMPLE;
			  break;
			}
			CheckFlag();
		}

		//Field-Operation
		if ((ptr = strstr(ucCmdLine,"-FIELD-OPERATION=")) || (ptr = strstr(ucCmdLine,"-FO=")))
		{
			ptr = lpCmdLine + (ptr - ucCmdLine);
			CheckMenuItem(hMenu, IDM_FO_NONE, MF_UNCHECKED);
			CheckMenuItem(hMenu, IDM_FO_FILM, MF_UNCHECKED);
			CheckMenuItem(hMenu, IDM_FO_RAW, MF_UNCHECKED);
			SetDlgItemText(hDlg, IDC_INFO, "");

			switch (*(strstr(ptr,"=")+1))
			{
			default:
			case '0':
			  FO_Flag = FO_NONE;
			  CheckMenuItem(hMenu, IDM_FO_NONE, MF_CHECKED);
			  break;
			case '1':
			  FO_Flag = FO_FILM;
			  CheckMenuItem(hMenu, IDM_FO_FILM, MF_CHECKED);
			  break;
			case '2':
			  FO_Flag = FO_RAW;
			  CheckMenuItem(hMenu, IDM_FO_RAW, MF_CHECKED);
			  break;
			}
		}

		//YUV->RGB
		if ((ptr = strstr(ucCmdLine,"-YUV-RGB=")) || (ptr = strstr(ucCmdLine,"-YR=")))
		{
			ptr = lpCmdLine + (ptr - ucCmdLine);    
			CheckMenuItem(hMenu, IDM_TVSCALE, MF_UNCHECKED);
			CheckMenuItem(hMenu, IDM_PCSCALE, MF_UNCHECKED);
    
			switch (*(strstr(ptr,"=")+1))
			{
			default:
			case '1':
			  Scale_Flag = true;
			  setRGBValues();    
			  CheckMenuItem(hMenu, IDM_PCSCALE, MF_CHECKED);
			  break;
      
			case '2':
			  Scale_Flag = false;
			  setRGBValues();
			  CheckMenuItem(hMenu, IDM_TVSCALE, MF_CHECKED);
			  break;
			}
		}

		// Luminance filter and cropping not implemented

		//Track number
		if ((ptr = strstr(ucCmdLine,"-TRACK-NUMBER=")) || (ptr = strstr(ucCmdLine,"-TN=")))
		{
            char track_list[1024], *p;
            unsigned int i, audio_id;
            // First get the track list into Track_List.
			ptr = lpCmdLine + (ptr - ucCmdLine);
			ptr = strstr(ptr,"=") + 1;
            strcpy(track_list, ptr);
            p = track_list;
			while (*p != ' ' && *p != 0)
				p++;
            *p = 0;
            strcpy(Track_List, track_list);
            // Now parse it and enable the specified audio ids for demuxing.
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

			Method_Flag = AUDIO_DEMUX;
			CheckMenuItem(hMenu, IDM_AUDIO_NONE, MF_UNCHECKED);
			CheckMenuItem(hMenu, IDM_DEMUX, MF_CHECKED);
			CheckMenuItem(hMenu, IDM_DEMUXALL, MF_UNCHECKED);
			CheckMenuItem(hMenu, IDM_DECODE, MF_UNCHECKED);
			EnableMenuItem(GetSubMenu(hMenu, 3), 1, MF_BYPOSITION | MF_ENABLED);
			EnableMenuItem(GetSubMenu(hMenu, 3), 3, MF_BYPOSITION | MF_GRAYED);
			EnableMenuItem(GetSubMenu(hMenu, 3), 4, MF_BYPOSITION | MF_GRAYED);
			EnableMenuItem(GetSubMenu(hMenu, 3), 5, MF_BYPOSITION | MF_GRAYED);
		}

		// Output Method
		if ((ptr = strstr(ucCmdLine,"-OUTPUT-METHOD=")) || (ptr = strstr(ucCmdLine,"-OM=")))
		{
			ptr = lpCmdLine + (ptr - ucCmdLine);
			CheckMenuItem(hMenu, IDM_AUDIO_NONE, MF_UNCHECKED);
			CheckMenuItem(hMenu, IDM_DEMUX, MF_UNCHECKED);
			CheckMenuItem(hMenu, IDM_DEMUXALL, MF_UNCHECKED);
			CheckMenuItem(hMenu, IDM_DECODE, MF_UNCHECKED);
			switch (*(strstr(ptr,"=")+1))
			{
			default:
			case '0':
				Method_Flag = AUDIO_NONE;
				CheckMenuItem(hMenu, IDM_AUDIO_NONE, MF_CHECKED);
				EnableMenuItem(GetSubMenu(hMenu, 3), 1, MF_BYPOSITION | MF_GRAYED);
				EnableMenuItem(GetSubMenu(hMenu, 3), 3, MF_BYPOSITION | MF_GRAYED);
				EnableMenuItem(GetSubMenu(hMenu, 3), 4, MF_BYPOSITION | MF_GRAYED);
				EnableMenuItem(GetSubMenu(hMenu, 3), 5, MF_BYPOSITION | MF_GRAYED);
				break;
      
			case '1':
				Method_Flag = AUDIO_DEMUX;
				CheckMenuItem(hMenu, IDM_DEMUX, MF_CHECKED);
			    EnableMenuItem(GetSubMenu(hMenu, 3), 1, MF_BYPOSITION | MF_ENABLED);
			    EnableMenuItem(GetSubMenu(hMenu, 3), 3, MF_BYPOSITION | MF_GRAYED);
			    EnableMenuItem(GetSubMenu(hMenu, 3), 4, MF_BYPOSITION | MF_GRAYED);
			    EnableMenuItem(GetSubMenu(hMenu, 3), 5, MF_BYPOSITION | MF_GRAYED);
				break;
     
			case '2':
				Method_Flag = AUDIO_DEMUXALL;
				CheckMenuItem(hMenu, IDM_DEMUXALL, MF_CHECKED);
				EnableMenuItem(GetSubMenu(hMenu, 3), 1, MF_BYPOSITION | MF_GRAYED);
				EnableMenuItem(GetSubMenu(hMenu, 3), 3, MF_BYPOSITION | MF_GRAYED);
				EnableMenuItem(GetSubMenu(hMenu, 3), 4, MF_BYPOSITION | MF_GRAYED);
				EnableMenuItem(GetSubMenu(hMenu, 3), 5, MF_BYPOSITION | MF_GRAYED);
				break;
     
			case '3':
				Method_Flag = AUDIO_DECODE;
				CheckMenuItem(hMenu, IDM_DECODE, MF_CHECKED);
				EnableMenuItem(GetSubMenu(hMenu, 3), 1, MF_BYPOSITION | MF_ENABLED);
				EnableMenuItem(GetSubMenu(hMenu, 3), 3, MF_BYPOSITION | MF_ENABLED);
				EnableMenuItem(GetSubMenu(hMenu, 3), 4, MF_BYPOSITION | MF_ENABLED);
				EnableMenuItem(GetSubMenu(hMenu, 3), 5, MF_BYPOSITION | MF_ENABLED);
				break;
			}
		}

		// Dynamic-Range-Control
		if ((ptr = strstr(ucCmdLine,"-DYNAMIC-RANGE-CONTROL=")) || (ptr = strstr(ucCmdLine,"-DRC=")))
		{
			ptr = lpCmdLine + (ptr - ucCmdLine);
			CheckMenuItem(hMenu, IDM_DRC_NONE, MF_UNCHECKED);
			CheckMenuItem(hMenu, IDM_DRC_LIGHT, MF_UNCHECKED);
			CheckMenuItem(hMenu, IDM_DRC_NORMAL, MF_UNCHECKED);
			CheckMenuItem(hMenu, IDM_DRC_HEAVY, MF_UNCHECKED);
			switch (*(strstr(ptr,"=")+1))
			{
			default:
			case '0':
			  CheckMenuItem(hMenu, IDM_DRC_NONE, MF_CHECKED);
			  DRC_Flag = DRC_NONE;
			  break;
			case '1':
			  CheckMenuItem(hMenu, IDM_DRC_LIGHT, MF_CHECKED);
			  DRC_Flag = DRC_LIGHT;
			  break;
			case '2':
			  CheckMenuItem(hMenu, IDM_DRC_NORMAL, MF_CHECKED);
			  DRC_Flag = DRC_NORMAL;
			  break;
			case '3':
			  CheckMenuItem(hMenu, IDM_DRC_HEAVY, MF_CHECKED);
			  DRC_Flag = DRC_HEAVY;
			  break;
			}
		}

		// Dolby Surround Downmix
		if ((ptr = strstr(ucCmdLine,"-DOLBY-SURROUND-DOWNMIX=")) || (ptr = strstr(ucCmdLine,"-DSD=")))
		{
			ptr = lpCmdLine + (ptr - ucCmdLine);
			CheckMenuItem(hMenu, IDM_DSDOWN, MF_UNCHECKED);
			DSDown_Flag = *(strstr(ptr,"=")+1) - '0';
			if (DSDown_Flag)
			  CheckMenuItem(hMenu, IDM_DSDOWN, MF_CHECKED);
		}

		// 48 -> 44 kHz
		if ((ptr = strstr(ucCmdLine,"-DOWNSAMPLE-AUDIO=")) || (ptr = strstr(ucCmdLine,"-DSA=")))
		{
			ptr = lpCmdLine + (ptr - ucCmdLine);
			CheckMenuItem(hMenu, IDM_SRC_NONE, MF_UNCHECKED);
			CheckMenuItem(hMenu, IDM_SRC_LOW, MF_UNCHECKED);
			CheckMenuItem(hMenu, IDM_SRC_MID, MF_UNCHECKED);
			CheckMenuItem(hMenu, IDM_SRC_HIGH, MF_UNCHECKED);
			CheckMenuItem(hMenu, IDM_SRC_UHIGH, MF_UNCHECKED);
			switch (*(strstr(ptr,"=")+1))
			{
			default:
			case '0':
				SRC_Flag = SRC_NONE;
				CheckMenuItem(hMenu, IDM_SRC_NONE, MF_CHECKED);
				break;
			case '1':
				SRC_Flag = SRC_LOW;
				CheckMenuItem(hMenu, IDM_SRC_LOW, MF_CHECKED);
				break;
			case '2':
				SRC_Flag = SRC_MID;
				CheckMenuItem(hMenu, IDM_SRC_MID, MF_CHECKED);
				break;
			case '3':
				SRC_Flag = SRC_HIGH;
				CheckMenuItem(hMenu, IDM_SRC_HIGH, MF_CHECKED);
				break;
			case '4':
				SRC_Flag = SRC_UHIGH;
				CheckMenuItem(hMenu, IDM_SRC_UHIGH, MF_CHECKED);
				break;
			}
		}

		// Normalization not implemented

		RefreshWindow(true);

		// AVS Template
		if ((ptr = strstr(ucCmdLine,"-AVS-TEMPLATE=")) || (ptr = strstr(ucCmdLine,"-AT=")))
		{
			FILE *bf;

			ptr = lpCmdLine + (ptr - ucCmdLine);
			ptr  = strstr(ptr, delimiter1) + 1;
			if (ptr == (char *) 1 || *ptr == delimiter2[0])
			{
				// A null file name specifies no template.
				AVSTemplatePath[0] = 0;
			}
			else
			{
				ende = strstr(ptr + 1, delimiter2);
				save = *ende;
				*ende = 0;
				strcpy(aFName, ptr);
				*ende = save;
				/* If the specified template file does not include a path, use the
				   current directory. */
				if (!strstr(aFName, "\\"))
				{
					GetCurrentDirectory(sizeof(cwd) - 1, cwd);
					strcat(cwd, "\\");
					strcat(cwd, aFName);
				}
				else
				{
					strcpy(cwd, aFName);
				}
				// Check that the specified template file exists and is readable.
				bf = fopen(cwd, "r");
				if (bf != 0)
				{
					// Looks good; save the path.
					fclose(bf);
					strcpy(AVSTemplatePath, cwd);
				}
				else
				{
					// Something is wrong, so don't use a template.
					AVSTemplatePath[0] = 0;
				}
			}
		}

		// Output D2V file
		if ((ptr = strstr(ucCmdLine,"-OUTPUT-FILE=")) || (ptr = strstr(ucCmdLine,"-OF=")) ||
			(ptr = strstr(ucCmdLine,"-OUTPUT-FILE-DEMUX=")) || (ptr = strstr(ucCmdLine,"-OFD=")))
		{
			// Set up demuxing if requested.
			if (strstr(ucCmdLine,"-OUTPUT-FILE-DEMUX=") || strstr(ucCmdLine,"-OFD="))
			{
				MuxFile = (FILE *) 0;
			}

			// Don't pop up warning boxes for automatic invocation.
			crop1088_warned = true;
			CLIActive = 1;
			ExitOnEnd = strstr(ucCmdLine,"-EXIT") ? 1 : 0;
			ptr = lpCmdLine + (ptr - ucCmdLine);
			ptr  = strstr(ptr, delimiter1) + 1;
			ende = strstr(ptr + 1, delimiter2);
			save = *ende;
			*ende = 0;
			strcpy(aFName, ptr);
			*ende = save;
            // We need to store the full path, so that all our path handling options work
            // the same way as for GUI mode.
			if (aFName[0] != '\\' && aFName[1] != ':')
			{
				GetCurrentDirectory(sizeof(szOutput) - 1, szOutput);
				strcat(szOutput, "\\");
				strcat(szOutput, aFName);
			}
			else
			{
				strcpy(szOutput, aFName);
			}
		}

		// Preview mode for generating the Info log file
		CLIPreview = strstr(ucCmdLine,"-PREVIEW") ? 1 : 0;

		if (!CLIActive && WindowMode == SW_HIDE)
		{
			MessageBoxTimeout(hWnd, "No output file in HIDE mode! Exiting...", NULL, MB_OK | MB_ICONERROR, 0, CLIActive ? CLITimeout : 0xffffffff);
			return -1;
		}
	}
#endif
	return 0;
}