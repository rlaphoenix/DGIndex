/* 
 *	Copyright (C) 2004-2006, Donald A Graft, All Rights Reserved
 *
 *  PAT/PMT table parser for PID detection.
 *	
 *  This is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  This is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with this code; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *
 */

#include <stdio.h>

#define TS_SYNC_BYTE 0x47
#define PAT_PID 0
#define PSIP_PID 0x1ffb
#define LIMIT 10000000
#define MAX_PROGRAMS 500
#define MAX_SECTION 4096
#define MAX_PIDS 500
#define MAX_PACKETS 100000
#define PCR_STREAM 1

class PATParser
{
private:
	enum operation {Dump=1, AudioType, InitialPids} op;
	HWND hDialog;
	char *filename;
	unsigned int audio_pid;
	unsigned int audio_type;
	FILE *fin;
	unsigned int num_pmt_pids, num_programs;
	bool first_pat, first_pmt;
	unsigned int pmt_pids[MAX_PIDS];
	unsigned int programs[MAX_PROGRAMS];
	unsigned char section[MAX_SECTION];
	unsigned char *section_ptr;
	unsigned char buffer[204];
private:
	int SyncTransport(void);
	void PATParser::GetTable(unsigned int table_pid);
	int AnalyzePAT(void);
	int AnalyzePSIP(void);
	int AnalyzeRaw(void);
	int ProcessPATSection(void);
	int ProcessPMTSection(void);
	int ProcessPSIPSection(void);
public:
	PATParser(void);
	int DumpPAT(HWND hDialog, char *filename);
	int DumpPSIP(HWND hDialog, char *filename);
	int DumpRaw(HWND hDialog, char *filename);
	int GetAudioType(char *filename, unsigned int audio_pid);
	int DoInitialPids(char *filename);
};

