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

#include "windows.h"
#include "resource.h"
#include "global.h"

PATParser::PATParser(void)
{
}

// Need to add transport re-syncing in case of errors.
int PATParser::SyncTransport(void)
{
	int i;
	unsigned char byte;

	// Find a sync byte.
	for (i = 0; i < LIMIT; i++)
	{
		if (fread(&byte, 1, 1, fin) == 0)
		{
			return 1;
		}
		if (byte == TS_SYNC_BYTE)
		{
			fseek(fin, TransportPacketSize - 1, SEEK_CUR);
			if (fread(&byte, 1, 1, fin) == 0)
			{
				return 1;
			}
			if (byte == TS_SYNC_BYTE)
			{
				fseek(fin, -(TransportPacketSize + 1), SEEK_CUR);
				break;
			}
			fseek(fin, -TransportPacketSize, SEEK_CUR);
		}
	}
	if (i == LIMIT)
	{
		return 1;
	}
	return 0;
}

int PATParser::DumpRaw(HWND _hDialog, char *_filename)
{
	hDialog = _hDialog;
	filename = _filename;
	return AnalyzeRaw();
}

int PATParser::AnalyzeRaw(void)
{
	unsigned int i, pid = 0;
	unsigned char stream_id;
	int afc, pkt_count;
	unsigned char buffer[204];
	int read, pes_offset, pes_header_data_length, data_offset;
	char listbox_line[255], description[80];
	struct
	{
		unsigned int pid;
		unsigned char stream_id;
	} Pids[MAX_PIDS];

	// Open the input file for reading.
	if ((fin = fopen(filename, "rb")) == NULL)
	{
		if (hDialog != NULL)
		{
			sprintf(listbox_line, "Cannot open the input file!");
			SendDlgItemMessage(hDialog, IDC_PID_LISTBOX, LB_ADDSTRING, 0, (LPARAM)listbox_line);
		}
		return 1;
	}

	if (SyncTransport() == 1)
	{
		fclose(fin);
		return 1;
	}

	// Initialize the PID data table.
	for (i = 0; i < MAX_PIDS; i++)
	{
		Pids[i].pid = 0xffffffff;
		Pids[i].stream_id = 0;
	}

	// Process the transport packets looking for PIDs.
	pkt_count = 0;
	while ((pkt_count++ < MAX_PACKETS) && (read = fread(buffer, 1, TransportPacketSize, fin)) == TransportPacketSize)
	{
		// Pick up the PID.
		pid = ((buffer[1] & 0x1f) << 8) | buffer[2];
		stream_id = 0;
		// Check payload_unit_start_indicator.
		if (buffer[1] & 0x40)
		{
			// Look for a payload.
			afc = (buffer[3] & 0x30) >> 4;
			if (afc == 1 || afc == 3)
			{
				// Skip the adaptation field (if present).
				if (afc == 1)
				{
					pes_offset = 4;
				}
				else
				{
					pes_offset = 5 + buffer[4];
				}
				// Get the stream ID if there is a packet start code.
				if (buffer[pes_offset] == 0 && buffer[pes_offset+1] == 0 && buffer[pes_offset+2] == 1)
					stream_id = buffer[pes_offset+3];
				if (stream_id == 0xbd)
				{
					// This is private stream 1, which may carry AC3 or DTS audio.
					// Since all DTS frames fit in one PES packet, we can just
					// look for the DTS audio frame header to distinguish the two.
					pes_header_data_length = buffer[pes_offset+8];
					data_offset = pes_offset + 9 + pes_header_data_length;
					if (buffer[data_offset] == 0x7f &&
						buffer[data_offset+1] == 0xfe &&
						buffer[data_offset+2] == 0x80 &&
						buffer[data_offset+3] == 0x01)
					{
						// Borrow this reserved value to label DTS.
						stream_id = 0xfe;
					}
				}
			}
			// Find an empty table entry and enter this PID if we haven't already.
			for (i = 0; Pids[i].pid != 0xffffffff && i < MAX_PIDS; i++)
			{
				if (Pids[i].pid == pid) break;
			}
			if (Pids[i].pid == 0xffffffff)
			{
				Pids[i].pid = pid;
				Pids[i].stream_id = stream_id;
			}
		}
	}

	// Display the detection results in the dialog box.
	for (i = 0; Pids[i].pid != 0xffffffff && i < MAX_PIDS; i++)
	{
		if (Pids[i].pid == 0)
			strcpy(description, "PAT");
		else if (Pids[i].pid == 1)
			strcpy(description, "CAT");
		else if (Pids[i].pid >= 2 && Pids[i].pid <= 0xf)
			strcpy(description, "Reserved");
		else if (Pids[i].pid == 0x1fff)
			strcpy(description, "Null");
		else if (Pids[i].stream_id == 0xbc)
			strcpy(description, "Program Stream Map");
		else if (Pids[i].stream_id == 0xbd || Pids[i].stream_id == 0xfd)
		{
			if (!hDialog && Pids[i].pid == audio_pid)
			{
				audio_type = 0x81;
				fclose(fin);
				return 0;
			}
			if (op == InitialPids && MPEG2_Transport_AudioPID == 0x2)
				MPEG2_Transport_AudioPID = Pids[i].pid;
			strcpy(description, "Private Stream 1 (AC3/DTS Audio)");
		}
		else if (Pids[i].stream_id == 0xbe)
			strcpy(description, "Padding Stream");
		else if (Pids[i].stream_id == 0xbf)
			strcpy(description, "Private Stream 2");
		else if (((Pids[i].stream_id & 0xe0) == 0xc0) ||
				 (Pids[i].stream_id == 0xfa))
		{
			if (!hDialog && Pids[i].pid == audio_pid)
			{
				// The demuxing code is the same for MPEG and AAC.
				// The only difference will be the filename.
				// The demuxing code will look at the audio sync word to
				// decide between the two.
				audio_type = 0x4;
				fclose(fin);
				return 0;
			}
			if (op == InitialPids && MPEG2_Transport_AudioPID == 0x2)
				MPEG2_Transport_AudioPID = Pids[i].pid;
			strcpy(description, "MPEG1/MPEG2/AAC Audio");
		}
		else if ((Pids[i].stream_id & 0xf0) == 0xe0)
		{
			if (op == InitialPids && MPEG2_Transport_VideoPID == 0x2)
				MPEG2_Transport_VideoPID = Pids[i].pid;
			strcpy(description, "MPEG Video");
		}
		else if (Pids[i].stream_id == 0xf0)
			strcpy(description, "ECM Stream");
		else if (Pids[i].stream_id == 0xf1)
			strcpy(description, "EMM Stream");
		else if (Pids[i].stream_id == 0xf9)
			strcpy(description, "Ancillary Stream");
		else if (Pids[i].stream_id == 0xfe)
		{
			if (!hDialog && Pids[i].pid == audio_pid)
			{
				audio_type = 0xfe;
				fclose(fin);
				return 0;
			}
			if (op == InitialPids && MPEG2_Transport_AudioPID == 0x2)
				MPEG2_Transport_AudioPID = Pids[i].pid;
			strcpy(description, "Private Stream 1 (DTS Audio)");
		}
		else if (Pids[i].stream_id == 0xff)
			strcpy(description, "Program Stream Directory");
		else if (Pids[i].stream_id == PCR_STREAM)
			strcpy(description, "PCR");
		else
			strcpy(description, "Other");
		if (hDialog != NULL)
		{
			sprintf(listbox_line, "0x%x (%d): %s", Pids[i].pid, Pids[i].pid, description);
			SendDlgItemMessage(hDialog, IDC_PID_LISTBOX, LB_ADDSTRING, 0, (LPARAM)listbox_line);
		}
	}

	fclose(fin);
	return 0;
}

int PATParser::DumpPAT(HWND _hDialog, char *_filename)
{
	op = Dump;
	hDialog = _hDialog;
	filename = _filename;
	return AnalyzePAT();
}

int PATParser::DumpPSIP(HWND _hDialog, char *_filename)
{
	op = Dump;
	hDialog = _hDialog;
	filename = _filename;
	return AnalyzePSIP();
}

int PATParser::GetAudioType(char *_filename, unsigned int _audio_pid)
{
	op = AudioType;
	hDialog = NULL;
	filename = _filename;
	audio_pid = _audio_pid;
	audio_type = 0xffffffff;

	if ((AnalyzePAT() == 0) && (audio_type != 0xffffffff))
		return audio_type;
	else if ((AnalyzeRaw() == 0) && (audio_type != 0xffffffff))
		return audio_type;
	else
		return -1;
}

int PATParser::DoInitialPids(char *_filename)
{
	op = InitialPids;
	hDialog = NULL;
	filename = _filename;
	AnalyzePAT();
	if (MPEG2_Transport_VideoPID == 0x2)
	{
		AnalyzeRaw();
	}
	return 0;
}

int PATParser::AnalyzePAT(void)
{
	unsigned int entry;
	char listbox_line[255];

	// First, we need to get from the PAT the list of PMT PIDs that we will need
	// to examine.
	num_pmt_pids = 0;
	first_pat = first_pmt = true;

	// Open the input file for reading.
	if ((fin = fopen(filename, "rb")) == NULL)
	{
		if (op == Dump)
		{
			sprintf(listbox_line, "Cannot open the input file!");
			SendDlgItemMessage(hDialog, IDC_PID_LISTBOX, LB_ADDSTRING, 0, (LPARAM)listbox_line);
		}
		return 1;
	}

	if (SyncTransport() == 1)
	{
		fclose(fin);
		return 1;
	}

	// Acquire and parse the PAT.
	GetTable(PAT_PID);

	// Exit if we didn't find the PAT.
	if (first_pat == true)
	{
		fclose(fin);
		return 1;
	}

	// Now we have to get the PMT tables to retrieve the actual
	// program information. Scan on each PMT PID in turn.
	for (entry = 0, num_programs = 0; entry < num_pmt_pids; entry++)
	{
		// Start again at the beginning of the file.
		fseek(fin, 0, SEEK_SET);
		if (SyncTransport() == 1)
		{
			fclose(fin);
			return 1;
		}

		// Acquire and parse the PMT.
		GetTable(pmt_pids[entry]);
	}
	fclose(fin);
	return 0;
}

int PATParser::AnalyzePSIP(void)
{
	char listbox_line[255];

	first_pat = true;

	// Open the input file for reading.
	if ((fin = fopen(filename, "rb")) == NULL)
	{
		if (op == Dump)
		{
			sprintf(listbox_line, "Cannot open the input file!");
			SendDlgItemMessage(hDialog, IDC_PID_LISTBOX, LB_ADDSTRING, 0, (LPARAM)listbox_line);
		}
		return 1;
	}

	if (SyncTransport() == 1)
	{
		fclose(fin);
		return 1;
	}

	// Acquire and parse the PAT.
	GetTable(PSIP_PID);

	// Exit if we didn't find the PAT.
	if (first_pat == true)
	{
		fclose(fin);
		return 1;
	}

	fclose(fin);
	return 0;
}

int PATParser::ProcessPSIPSection(void)
{
	unsigned int i, j, ndx, ndx2, program, num_channels_in_section, elements;
	unsigned int descriptors_length, length, tag, pcrpid, type, pid;
	char *stream_type;
	char listbox_line[255];

	// We want only current tables.
	if (!(section[5] & 0x01))
		return 0;

	num_channels_in_section = section[9];
	first_pat = false;

	for (i = 0, ndx = 10; i < num_channels_in_section; i++)
	{
		program = (section[ndx+24] << 8) + section[ndx+25];
		if (op == Dump)
		{
			sprintf(listbox_line, "Program %d", program);
			SendDlgItemMessage(hDialog, IDC_PID_LISTBOX, LB_ADDSTRING, 0, (LPARAM)listbox_line);
		}
		descriptors_length = (section[ndx+30] << 8) + section[ndx+31];
		descriptors_length &= 0x3ff;
		ndx += 32;
		ndx2 = ndx;
		while (ndx2 < ndx + descriptors_length)
		{
			tag = section[ndx2];
			length = section[ndx2+1];
			pcrpid = (section[ndx2+2] << 8) + section[ndx2+3];
			pcrpid &= 0x1fff;
			if (op == Dump)
			{
				sprintf(listbox_line, "    PCR on PID 0x%x (%d)", pcrpid, pcrpid); 
				SendDlgItemMessage(hDialog, IDC_PID_LISTBOX, LB_ADDSTRING, 0, (LPARAM)listbox_line);
			}
			elements = section[ndx2+4];
			for (j = 0; j < elements; j++)
			{
				type = section[ndx2+5+j*6];
				switch (type)
				{
				case 0x01:
					stream_type = "MPEG1 Video";
					break;
				case 0x02:
					stream_type = "MPEG2 Video";
					break;
				case 0x03:
					stream_type = "MPEG1 Audio";
					break;
				case 0x04:
					stream_type = "MPEG2 Audio";
					break;
				case 0x05:
					stream_type = "Private Sections";
					break;
				case 0x07:
					stream_type = "Teletext/Subtitling";
					break;
				case 0x0f:
				case 0x11:
					stream_type = "AAC Audio";
					break;
				case 0x80:
					// This could be private stream video or LPCM audio.
					stream_type = "Private Stream";
					break;
				case 0x81:
					// This could be AC3 or DTS audio.
					stream_type = "AC3/DTS Audio";
					break;
				// These are found on bluray disks.
				case 0x85:
				case 0x86:
					stream_type = "DTS Audio";
					break;
				default:
					stream_type = "Other";
					break;
				}
				pid = (section[ndx2+5+j*6+1] << 8) + section[ndx2+5+j*6+2];
				pid &= 0x1fff;
				if (op == Dump)
				{
					sprintf(listbox_line, "    %s on PID 0x%x (%d)", stream_type, pid, pid);
					SendDlgItemMessage(hDialog, IDC_PID_LISTBOX, LB_ADDSTRING, 0, (LPARAM)listbox_line);
				}
			}
			ndx2 += length + 2;
		}
		ndx += descriptors_length;
	}

	return 1;
}

int PATParser::ProcessPATSection(void)
{
	unsigned int i, j, ndx, section_length, number, last, program, pmtpid;

	// We want only current tables.
	if (!(section[5] & 0x01))
		return 0;

	section_length = ((section[1] & 0x03) << 8);
	section_length |= section[2];

	// Get the number of this section.
	number = section[6];

	// Get the number of the last section.
	last = section[7];

	// Now we parse the program/pid tuples.
	ndx = 8;
	for (i = 0; i < section_length - 9;)
	{
		program = section[ndx+i++] << 8;
		program |= section[ndx+i++];
		pmtpid = (section[ndx+i++] & 0x1f) << 8;
		pmtpid |= section[ndx+i++];
		// Skip the Network Information Table (NIT).
		if (program != 0)
		{
			// Store the PTM PID if we haven't already.
			for (j = 0; j < num_pmt_pids; j++)
				if (pmt_pids[j] == pmtpid)
					break;
			if (j == num_pmt_pids)
				pmt_pids[num_pmt_pids++] = pmtpid;
		}
	}
	first_pat = false;

	// If this is the last section number, we're done.
	return (number == last);
}

int PATParser::ProcessPMTSection(void)
{
	unsigned int j, ndx, section_length, program, pid, pcrpid;
	unsigned int descriptors_length, es_descriptors_length, type, encrypted;
	char listbox_line[255];
	char *stream_type;

	// We want only current tables.
	if (!(section[5] & 0x01))
		return 0;

	section_length = ((section[1] & 0x03) << 8);
	section_length |= section[2];

	// Check the program number.
	program = section[3] << 8;
	program |= section[4];

	// If we're setting the initial PIDs automatically, then
	// we're intersted in only the first program.
	if (op == InitialPids && num_programs > 0)
		return 0;
	// We stop when we see a progam that we've already seen.
	for (j = 0; j < num_programs; j++)
		if (programs[j] == program)
			break;
	if (j != num_programs)
		return 1;
	programs[num_programs++] = program;
	if (op == Dump)
	{
		sprintf(listbox_line, "Program %d", program);
		SendDlgItemMessage(hDialog, IDC_PID_LISTBOX, LB_ADDSTRING, 0, (LPARAM)listbox_line);
	}

	// Get the PCRPID.
	pcrpid = (section[8] & 0x1f) << 8;
	pcrpid |= section[9];
	if (op == InitialPids)
		MPEG2_Transport_PCRPID = pcrpid;
	else if (op == Dump)
	{
		sprintf(listbox_line, "    PCR on PID 0x%x (%d)", pcrpid, pcrpid); 
		SendDlgItemMessage(hDialog, IDC_PID_LISTBOX, LB_ADDSTRING, 0, (LPARAM)listbox_line);
	}

	// Skip the descriptors (if any).
	ndx = 10;
	descriptors_length = (section[ndx++] & 0x0f) << 8;
	descriptors_length |= section[ndx++];
	ndx += descriptors_length;

	// Now we have the actual program data.
	while (ndx < section_length - 4)
	{
		switch (type = section[ndx++])
		{
		case 0x01:
			stream_type = "MPEG1 Video";
			pid = (section[ndx++] & 0x1f) << 8;
			pid |= section[ndx++];
			if (op == InitialPids && MPEG2_Transport_VideoPID == 0x02)
				MPEG2_Transport_VideoPID = pid;
			break;
		case 0x02:
			stream_type = "MPEG2 Video";
			pid = (section[ndx++] & 0x1f) << 8;
			pid |= section[ndx++];
			if (op == InitialPids && MPEG2_Transport_VideoPID == 0x02)
				MPEG2_Transport_VideoPID = pid;
			break;
		case 0x03:
			stream_type = "MPEG1 Audio";
			pid = (section[ndx++] & 0x1f) << 8;
			pid |= section[ndx++];
			if (op == InitialPids && MPEG2_Transport_AudioPID == 0x02)
				MPEG2_Transport_AudioPID = pid;
			break;
		case 0x04:
			stream_type = "MPEG2 Audio";
			pid = (section[ndx++] & 0x1f) << 8;
			pid |= section[ndx++];
			if (op == InitialPids && MPEG2_Transport_AudioPID == 0x02)
				MPEG2_Transport_AudioPID = pid;
			break;
		case 0x05:
			stream_type = "Private Sections";
			pid = (section[ndx++] & 0x1f) << 8;
			pid |= section[ndx++];
			break;
		case 0x06:
			// Have to parse the ES descriptors to figure this out.
			pid = (section[ndx++] & 0x1f) << 8;
			pid |= section[ndx++];
			break;
		case 0x07:
			stream_type = "Teletext/Subtitling";
			pid = (section[ndx++] & 0x1f) << 8;
			pid |= section[ndx++];
			break;
		case 0x0f:
		case 0x11:
			// The demuxing code is the same for MPEG and AAC.
			// The only difference will be the filename.
			// The demuxing code will look at the audio sync word to
			// decide between the two.
			type = 0x04;
			stream_type = "AAC Audio";
			pid = (section[ndx++] & 0x1f) << 8;
			pid |= section[ndx++];
			if (op == InitialPids && MPEG2_Transport_AudioPID == 0x02)
				MPEG2_Transport_AudioPID = pid;
			break;
		case 0x80:
			// This could be private stream video or LPCM audio.
			stream_type = "Private Stream";
			pid = (section[ndx++] & 0x1f) << 8;
			pid |= section[ndx++];
			break;
		case 0x81:
			// This could be AC3 or DTS audio.
			stream_type = "AC3/DTS Audio";
			pid = (section[ndx++] & 0x1f) << 8;
			pid |= section[ndx++];
			if (op == InitialPids && MPEG2_Transport_AudioPID == 0x02)
				MPEG2_Transport_AudioPID = pid;
			break;
		// These are found on bluray disks.
		case 0x85:
		case 0x86:
			stream_type = "DTS Audio";
			type = 0xfe;
			pid = (section[ndx++] & 0x1f) << 8;
			pid |= section[ndx++];
			if (op == InitialPids && MPEG2_Transport_AudioPID == 0x02)
				MPEG2_Transport_AudioPID = pid;
			break;
		default:
			stream_type = "Other";
			pid = (section[ndx++] & 0x1f) << 8;
			pid |= section[ndx++];
			break;
		}

		// Parse the ES descriptors if necessary.
		es_descriptors_length = (section[ndx++] & 0x0f) << 8;
		es_descriptors_length |= section[ndx++];
		encrypted = 0;
		if (es_descriptors_length)
		{
			unsigned int start = ndx, end = ndx + es_descriptors_length;
			int tag, length;

			// See if the ES is scrambled.
			do
			{
				tag = section[ndx++];
				if (tag == 0x09)
					encrypted = 1;
				length = section[ndx++];
				ndx += length;
			} while (ndx < end);
			ndx = start;

			if (type == 0x80)
			{
				// This might be private stream video.
				// Parse the descriptors for a video descriptor.
				int hadVideo = 0, hadAudio = 0;
				do
				{
					tag = section[ndx++];
					if (tag == 0x02)
						hadVideo = 1;
					else if (tag == 0x05)
						hadAudio = 1;
					length = section[ndx++];
					ndx += length;
				} while (ndx < end);
				ndx = end;
				if (hadVideo == 1)
				{
					stream_type = "Private Stream Video";
					type = 0x02;
					if (op == InitialPids && MPEG2_Transport_VideoPID == 0x02)
						MPEG2_Transport_VideoPID = pid;
				}
				else if (hadAudio == 1)
				{
					stream_type = "LPCM Audio";
					if (op == InitialPids && MPEG2_Transport_AudioPID == 0x02)
						MPEG2_Transport_AudioPID = pid;
				}
			}
			else if (type == 0x06)
			{
				// Parse the descriptors for a DTS audio descriptor.
				unsigned int end = ndx + es_descriptors_length;
				int hadDTS = 0, hadTeletext = 0;
				do
				{
					tag = section[ndx++];
					if (tag == 0x73)
						hadDTS = 1;
					else if (tag == 0x05)
					{
						if (section[ndx+1] == 0x44 && section[ndx+2] == 0x54 && section[ndx+3] == 0x53)
							hadDTS = 1;
					}
					else if (tag == 0x56)
						hadTeletext = 1;
					length = section[ndx++];
					ndx += length;
				} while (ndx < end);
				ndx = end;
				if (hadTeletext == 1)
				{
					stream_type = "Teletext";
				}
				else if (hadDTS == 1)
				{
					stream_type = "DTS Audio";
					type = 0xfe;
					if (op == InitialPids && MPEG2_Transport_AudioPID == 0x02)
						MPEG2_Transport_AudioPID = pid;
				}
				else
				{
					stream_type = "AC3 Audio";
					type = 0x81;
					if (op == InitialPids && MPEG2_Transport_AudioPID == 0x02)
						MPEG2_Transport_AudioPID = pid;
				}
			}
			else
			{
				ndx += es_descriptors_length;				
			}

		}
		else if (type == 0x06)
		{
			// The following assignments will be used if there are no descriptors.
			stream_type = "AC3/DTS Audio";
			type = 0x81;
		}
		if (op == Dump)
		{
			sprintf(listbox_line, "    %s on PID 0x%x (%d)", stream_type, pid, pid);
			SendDlgItemMessage(hDialog, IDC_PID_LISTBOX, LB_ADDSTRING, 0, (LPARAM)listbox_line);
			if (encrypted == 1)
			{
				sprintf(listbox_line, "    [Scrambled]"); 
				SendDlgItemMessage(hDialog, IDC_PID_LISTBOX, LB_ADDSTRING, 0, (LPARAM)listbox_line);
			}
		}
		if (op == AudioType && pid == audio_pid)
		{
			if (type != 0x81)
			{
				audio_type = type;
				return 0;
			}
            // If the packet size is 192, it's probably M2TS (bluray)
            // and so we accept it as AC3.
            else if (TransportPacketSize == 192)
			{
                audio_type = type;
				return 0;
			}
			// If it's private stream 1, it could be AC3 or DTS.
			// Force raw detection to find out which.
			return 1;
		}
	}

	return 0;
}

void PATParser::GetTable(unsigned int table_pid)
{
	unsigned char byte;
	unsigned int pid, ndx, section_length;
	int read;
	int pkt_count;
	int ret;

	// Process the transport packets.
	pkt_count = 0;
	section_ptr = section;
	while ((pkt_count++ < MAX_PACKETS) && (read = fread(buffer, 1, TransportPacketSize, fin)) == TransportPacketSize)
	{
		// Check that this is the desired PID.
		pid = ((buffer[1] & 0x1f) << 8) | buffer[2];
		if (pid != table_pid)
			continue;

		// We have a table packet.
		// Check error indicator.
		if (buffer[1] & 0x80)
			continue;

		// Check for presence of a section start.
		if ((section_ptr == section) && !(buffer[1] & 0x40))
			continue;

		// Skip if there is no payload.
		byte = (unsigned char) ((buffer[3] & 0x30) >> 4);
		if (byte == 0 || byte == 2)
			continue;

		// Skip the adaptation field (if any).
		ndx = 4;
		if (byte == 3)
		{
			ndx += buffer[ndx] + 1;
		}

		// This equality will fail when we have started gathering a section,
		// but it overflowed into the next transport packet. So here we collect
		// the rest of the section.
		if (section_ptr != section)
		{
			// Collect the section data.
            if (section_length <= TransportPacketSize - ndx - (TransportPacketSize == 192 ? 4 : 0))
			{
				// The remainder of the section is contained entirely
				// in this packet. Skip the pointer field if one exists.
				unsigned char *start;
				start = &buffer[ndx];
				if (buffer[1] & 0x40)
					start++;
				memcpy(section_ptr, start, section_length);
				section_ptr += section_length;
				// Parse the table.
				switch (table_pid)
				{
				case PAT_PID:
					ret = ProcessPATSection();
					break;
				case PSIP_PID:
					ret = ProcessPSIPSection();
					break;
				default:
					ret = ProcessPMTSection();
					break;
				}
				if (ret)
					break;
				// Get ready for collection of the next section.
				section_ptr = section;
				// Check the section syntax indicator to see if another
				// section follows in this packet.
				if (!(buffer[1] & 0x40))
				{
					// No more sections in this packet.
					continue;
				}
				// If we reach here, there is another section following
				// this one, so we fall through to the code below.
			}
			else
			{
				// The section is spilling over to the next packet.
				// There can't be a pointer field, so there's no need to
				// check for it.
				memcpy(section_ptr, &buffer[ndx], TransportPacketSize - ndx - (TransportPacketSize == 192 ? 4 : 0));
				section_ptr += (TransportPacketSize - ndx - (TransportPacketSize == 192 ? 4 : 0));
				section_length -= (TransportPacketSize - ndx - (TransportPacketSize == 192 ? 4 : 0));
				continue;
			}
		}

		// Read the section pointer field and use it to skip to the start of the section.
		ndx += buffer[ndx] + 1;

		// Now pointing to the start of the section. Check that the table id is correct.
		if (buffer[ndx] != (table_pid == PAT_PID ? 0 : (table_pid == PSIP_PID ? 0xc8 : 2)))
			continue;

another_section:

		// Check the section syntax indicator.
		if (table_pid == PSIP_PID)
		{
			if ((buffer[ndx+1] & 0xf0) != 0xf0)
				continue;
		}
		else
		{
			if ((buffer[ndx+1] & 0xc0) != 0x80)
				continue;
		}

		// Check and get section length.
		if ((buffer[ndx+1] & 0x0c) != 0)
			continue;
		section_length = ((buffer[ndx+1] & 0x03) << 8);
		section_length |= buffer[ndx+2];
		if (section_length > 0x3fd)
			continue;

		if (table_pid == PAT_PID)
			first_pat = false;
		else
			first_pmt = false;
		if (ndx + section_length + 3 <= (unsigned int) TransportPacketSize - (TransportPacketSize == 192 ? 4 : 0))
		{
			// The section is entirely contained in this packet.
			// Collect the section data.
			memcpy(section_ptr, &buffer[ndx], section_length);
			section_ptr += section_length;
			// Parse the section.
			switch (table_pid)
			{
			case PAT_PID:
				ret = ProcessPATSection();
				break;
			case PSIP_PID:
				ret = ProcessPSIPSection();
				break;
			default:
				ret = ProcessPMTSection();
				break;
			}
			if (ret)
				break;
			// Get ready for collecting the next section.
			section_ptr = section;
			// Check to see if another section follows in this packet
			// by looking for the table ID. It would be stuffing if
			// there was no following section.
			if (buffer[ndx+section_length+3] == 0x02)
			{
				ndx += section_length + 3;
				goto another_section;
			}
		}
		else
		{
			// This section spills over to the next transport packet.
			// Collect the first part and get ready to collect the
			// second part from the next packet.
			memcpy(section_ptr, &buffer[ndx], TransportPacketSize - ndx - (TransportPacketSize == 192 ? 4 : 0));
			section_ptr += (TransportPacketSize - ndx - (TransportPacketSize == 192 ? 4 : 0));
            section_length -= (TransportPacketSize - 3 - ndx - (TransportPacketSize == 192 ? 4 : 0));
			continue;
		}
	}
}
