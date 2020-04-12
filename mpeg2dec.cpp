/* 
 *  Mutated into DGIndex. Modifications Copyright (C) 2004, Donald Graft
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

#include "global.h"
#include "getbit.h"
#include "math.h"
#include "shlwapi.h"

static BOOL GOPBack(void);
static void InitialDecoder(void);
extern void StartVideoDemux(void);

#define DISABLE_EXCEPTION_HANDLING

DWORD WINAPI MPEG2Dec(LPVOID n)
{
	int i = (int) n; // Prevent compiler warning.
	extern unsigned int VideoPTS;
	extern unsigned int AudioPTS;
	extern FILE *mpafp, *mpvfp;
	__int64 saveloc;
    int savefile;
    int field;

	Pause_Flag = Stop_Flag = Start_Flag = HadAudioPTS = false;
    VideoPTS = AudioPTS = 0;
	Fault_Flag = 0;
	Frame_Number = Second_Field = 0;
	Sound_Max = 1; Bitrate_Monitor = 0;
	Bitrate_Average = 0.0;
    max_rate = 0.0;
	GOPSeen = false;
	AudioOnly_Flag = 0;
	AudioFilePath[0] = 0;
    LowestAudioId = 0xffffffff;
    StartLogging_Flag = 0;
    d2v_current.picture_structure = -1;

    for (i = 0; i < 0xc8; i++)
    {
        audio[i].type = 0;
        audio[i].rip = 0;
        audio[i].size = 0;
    }
	mpafp = mpvfp = NULL;

	switch (process.locate)
	{
		case LOCATE_FORWARD:
			process.startfile = process.file;
			process.startloc = (process.lba + 1) * SECTOR_SIZE;

			process.end = Infiletotal - SECTOR_SIZE;
			process.endfile = NumLoadedFiles - 1;
			process.endloc = (Infilelength[NumLoadedFiles-1]/SECTOR_SIZE - 1) * SECTOR_SIZE;
			break;

		case LOCATE_BACKWARD:
			process.startfile = process.file;
			process.startloc = process.lba * SECTOR_SIZE;
			process.end = Infiletotal - SECTOR_SIZE;
			process.endfile = NumLoadedFiles - 1;
			process.endloc = (Infilelength[NumLoadedFiles-1]/SECTOR_SIZE - 1) * SECTOR_SIZE;
			if (GOPBack() == false && CurrentFile > 0)
			{
				// Trying to step back to previous VOB file.
				process.startfile--;
				CurrentFile--;
				process.startloc = Infilelength[process.startfile];
				process.run = 0;
				for (i=0; i<process.startfile; i++)
					process.run += Infilelength[i];
				process.start = process.run + process.startloc;
				GOPBack();
			}
			break;

		case LOCATE_RIP:
		case LOCATE_DEMUX_AUDIO:
			process.startfile = process.leftfile;
			process.startloc = process.leftlba * SECTOR_SIZE;
			process.endfile = process.rightfile;
			process.endloc = (process.rightlba - 1) * SECTOR_SIZE;
			goto do_rip_play;
		case LOCATE_PLAY:
			process.startfile = process.file;
			process.startloc = process.lba * SECTOR_SIZE;
			process.endfile = NumLoadedFiles - 1;
			process.endloc = (Infilelength[NumLoadedFiles-1]/SECTOR_SIZE - 1) * SECTOR_SIZE;
			process.locate = LOCATE_RIP;
do_rip_play:
			process.run = 0;
			for (i=0; i<process.startfile; i++)
				process.run += Infilelength[i];
			process.start = process.run + process.startloc;

			process.end = 0;
			for (i=0; i<process.endfile; i++)
				process.end += Infilelength[i];
			process.end += process.endloc;
			break;

		case LOCATE_SCROLL:
			CurrentFile = process.startfile;
			_lseeki64(Infile[process.startfile], (process.startloc/SECTOR_SIZE)*SECTOR_SIZE, SEEK_SET);
			Initialize_Buffer();

			timing.op = 0;
			saveloc = process.startloc;

			for (;;)
			{
				Get_Hdr(0);
				if (Stop_Flag == true)
					ThreadKill(MISC_KILL);
				if (picture_coding_type == I_TYPE)
				{
					process.startloc = saveloc;
					break;
				}
			}
			break;
	}

	// If we are doing only audio demuxing (as for example, when we have
	// an audio-only stream), then bypass video parsing and just
	// do packet processing directly. We already know the stream type
	// because the !Check_Flag code below was performed when the file
	// was opened.
	if (process.locate == LOCATE_DEMUX_AUDIO)
	{
		Start_Flag = 1;
		AudioOnly_Flag = 1;
		AudioPktCount = 0;
		if (SystemStream_Flag == TRANSPORT_STREAM)
		{
			MPEG2_Transport_AudioType =
						pat_parser.GetAudioType(Infilename[0], MPEG2_Transport_AudioPID);
		}
		// Position to start of the first file.
		CurrentFile = 0;
		_lseeki64(Infile[0], 0, SEEK_SET);
		Initialize_Buffer();
		while (1)
		{
			Next_Packet();
			if (Stop_Flag)
			{
				ThreadKill(MISC_KILL);
			}
		}
	}

	// Check validity of the input file and collect some needed information.
	if (!Check_Flag)
	{
		int code;
		int i, count, Read;
		unsigned int show;
		unsigned char buf[32768], *b;

		// First see if it is a transport stream.

		// Skip any leading null characters, because some
		// captured transport files were seen to start with a large
		// number of nulls.

		_lseeki64(Infile[0], 0, SEEK_SET);
        for (;;)
		{
			if (_read(Infile[0], buf, 1) == 0)
			{
				// EOF
				MessageBox(hWnd, "File contains all nulls!", NULL, MB_OK | MB_ICONERROR);
				return 0;
			}
			if (buf[0] != 0)
			{
				// Unread the non-null byte and exit.
				_lseeki64(Infile[0], _lseeki64(Infile[0], 0, SEEK_CUR) - 1, SEEK_SET);
				break;
			}
		}

	    Read = _read(Infile[0], buf, 2048);
		TransportPacketSize = 188;
try_again:
        b = buf;
	    while (b + 4 * TransportPacketSize < buf + Read + 1)
	    {
		    if (*b == 0x47)
		    {
			    if ((b[TransportPacketSize] == 0x47) &&
					(b[2*TransportPacketSize] == 0x47) &&
					(b[3*TransportPacketSize] == 0x47) &&
					(b[4*TransportPacketSize] == 0x47))
			    {
					SystemStream_Flag = TRANSPORT_STREAM;
					// initial_parse is not called for transport streams, so
					// assume MPEG1 for now. It will be overridden to MPEG2 if
					// a sequence extension arrives.
					mpeg_type = IS_MPEG1;
					is_program_stream = 0;
					if (MPEG2_Transport_VideoPID == 0x02 &&
						MPEG2_Transport_AudioPID == 0x02 &&
						MPEG2_Transport_PCRPID == 0x02)
						pat_parser.DoInitialPids(Infilename[0]);
					break;
			    }
		    }
		    b++;
	    }
	    if (SystemStream_Flag != TRANSPORT_STREAM && TransportPacketSize == 188)
	    {
		    TransportPacketSize = 192;
		    goto try_again;
	    }
	    else if (SystemStream_Flag != TRANSPORT_STREAM && TransportPacketSize == 192)
	    {
		    TransportPacketSize = 204;
		    goto try_again;
	    }

		// Now try for PVA streams. Look for a packet start in the first 1024 bytes.
		if (SystemStream_Flag != TRANSPORT_STREAM)
		{
			CurrentFile = 0;
			_lseeki64(Infile[0], 0, SEEK_SET);
			Initialize_Buffer();

			for(i = 0; i < 1024; i++)
			{
				if (Rdbfr[i] == 0x41 && Rdbfr[i+1] == 0x56 && (Rdbfr[i+2] == 0x01 || Rdbfr[i+2] == 0x02))
				{
					SystemStream_Flag = PVA_STREAM;
					mpeg_type = IS_MPEG2;
					is_program_stream = 0;
					break;
				}
				Rdptr++;
			}
		}

		// If the file does not contain a sequence header start code, it can't be an MPEG file.
		// We're already byte aligned at the start of the file.
		CurrentFile = 0;
		_lseeki64(Infile[0], 0, SEEK_SET);
		Initialize_Buffer();
		count = 0;
		while ((show = Show_Bits(32)) != 0x1b3)
		{
			if (Stop_Flag || count > 10000000)
			{
				// We didn't find a sequence header.
				MessageBox(hWnd, "No video sequence header found!", NULL, MB_OK | MB_ICONERROR);
				ThreadKill(MISC_KILL);
			}
			Flush_Buffer(8);
			count++;
		}

		// Determine whether this is an MPEG2 file and whether it is a program stream.
		if (SystemStream_Flag != TRANSPORT_STREAM && SystemStream_Flag != PVA_STREAM)
		{
			mpeg_type = IS_NOT_MPEG;
			is_program_stream = 0;
			if (initial_parse(Infilename[0], &mpeg_type, &is_program_stream) == -1)
			{
				MessageBox(hWnd, "Cannot find video stream!", NULL, MB_OK | MB_ICONERROR);
				return 0;
			}
			if (is_program_stream)
			{
				SystemStream_Flag = PROGRAM_STREAM;
			}
		}

		CurrentFile = 0;
		_lseeki64(Infile[0], 0, SEEK_SET);
		Initialize_Buffer();

		// We know the stream type now, so our parsing is immune to
		// emulated video sequence headers in the pack layer.
		while (!Check_Flag)
		{
			// Move to the next video start code.
			next_start_code();
			code = Get_Bits(32);
			switch (code)
			{
			case SEQUENCE_HEADER_CODE:
				if (Stop_Flag == true)
					ThreadKill(MISC_KILL);
				sequence_header();
				InitialDecoder();
				Check_Flag = true;
				break;
			}
		}
	}

	Frame_Rate = (FO_Flag==FO_FILM) ? frame_rate * 0.8 : frame_rate;

	if (D2V_Flag)
	{
		i = NumLoadedFiles;

		// The first parameter is the format version number.
		// It must be coordinated with the test in DGDecode
		// and used to reject obsolete D2V files.
		fprintf(D2VFile, "DGIndexProjectFile%d\n%d\n", D2V_FILE_VERSION, i);
		while (i)
		{
			if (FullPathInFiles)
				fprintf(D2VFile, "%s\n", Infilename[NumLoadedFiles-i]);
			else
            {
				char path[DG_MAX_PATH];
				char* p = path;

                if (!PathRelativePathTo(path, D2VFilePath, 0, Infilename[NumLoadedFiles-i], 0))
					p = Infilename[NumLoadedFiles - i]; // different drives;
                // Delete leading ".\" if it is present.
                if (p[0] == '.' && p[1] == '\\')
                    p += 2;
				fprintf(D2VFile, "%s\n", p);
			}
			i--;
		}

		fprintf(D2VFile, "\nStream_Type=%d\n", SystemStream_Flag);
		if (SystemStream_Flag == TRANSPORT_STREAM)
		{
			fprintf(D2VFile, "MPEG2_Transport_PID=%x,%x,%x\n",
					MPEG2_Transport_VideoPID, MPEG2_Transport_AudioPID, MPEG2_Transport_PCRPID);
			fprintf(D2VFile, "Transport_Packet_Size=%d\n", TransportPacketSize);
		}

		fprintf(D2VFile, "MPEG_Type=%d\n", mpeg_type);
		fprintf(D2VFile, "iDCT_Algorithm=%d\n", iDCT_Flag);
		fprintf(D2VFile, "YUVRGB_Scale=%d\n", Scale_Flag);
		if (Luminance_Flag)
		{
			fprintf(D2VFile, "Luminance_Filter=%d,%d\n", LumGamma, LumOffset);
		}
		else
		{
			fprintf(D2VFile, "Luminance_Filter=0,0\n");
		}

		if (Cropping_Flag)
		{
			fprintf(D2VFile, "Clipping=%d,%d,%d,%d\n", Clip_Left, Clip_Right, Clip_Top, Clip_Bottom);
		}
		else
		{
			fprintf(D2VFile, "Clipping=0,0,0,0\n");
		}

		if (mpeg_type == IS_MPEG2)
			fprintf(D2VFile, "Aspect_Ratio=%s\n", AspectRatio[aspect_ratio_information]);
		else
			fprintf(D2VFile, "Aspect_Ratio=%s\n", AspectRatioMPEG1[aspect_ratio_information]);
		fprintf(D2VFile, "Picture_Size=%dx%d\n", horizontal_size, vertical_size);
		fprintf(D2VFile, "Field_Operation=%d\n", FO_Flag);
		if (FO_Flag == FO_FILM)
		{
			if (fabs(Frame_Rate - 23.976) < 0.001)
			{
				fr_num = 24000;
				fr_den = 1001;
			}
			else
			{
				fr_num = (unsigned int) (1000 * Frame_Rate);
				fr_den = 1000;
			}
		}
		fprintf(D2VFile, "Frame_Rate=%d (%u/%u)\n", (int)(Frame_Rate*1000), fr_num, fr_den);
		fprintf(D2VFile, "Location=%d,%I64x,%d,%I64x\n\n",
				process.leftfile, process.leftlba, process.rightfile, process.rightlba);

        // Determine the number of leading B frames from the start position. This will be used to
        // adjust VideoPTS so that it refers to the correct access unit.
	    PTSAdjustDone = 0;
        savefile = process.startfile;
        saveloc = process.startloc;
	    CurrentFile = process.startfile;
	    _lseeki64(Infile[process.startfile], (process.startloc/SECTOR_SIZE)*SECTOR_SIZE, SEEK_SET);
	    // We initialize the following variables to the current start position, because if the user
	    // has set a range, and the packs are large such that we won't hit a pack/packet start
	    // before we hit an I frame, we don't want the pack/packet position to remain at 0.
	    // It has to be at least equal to or greater than the starting position in the stream.
	    CurrentPackHeaderPosition = PackHeaderPosition = (process.startloc/SECTOR_SIZE)*SECTOR_SIZE;
		Initialize_Buffer();
		for (;;)
		{
			if (Stop_Flag) break;
			Get_Hdr(0);
			if (picture_coding_type == I_TYPE) break;
		}
		LeadingBFrames = 0;
		for (;;)
		{
			if (Get_Hdr(1) == 1)
			{
				// We hit a sequence end code or end of file, so there are no
				// more frames.
				break;
			}
			if (picture_coding_type != B_TYPE) break;
			if (picture_structure == FRAME_PICTURE)
    			LeadingBFrames += 2;
            else
    			LeadingBFrames += 1;
		}
        LeadingBFrames /= 2;
        process.startfile = savefile;
        process.startloc = saveloc;
	    Stop_Flag = false;
    }

	// Start normal decoding from the start position.
    if (Timestamps)
    {
        StartLogging_Flag = 1;
        fprintf(Timestamps, "leading B frames = %d\n", LeadingBFrames);
    }
    PTSAdjustDone = 0;
	CurrentFile = process.startfile;
	_lseeki64(Infile[process.startfile], (process.startloc/SECTOR_SIZE)*SECTOR_SIZE, SEEK_SET);
	// We initialize the following variables to the current start position, because if the user
	// has set a range, and the packs are large such that we won't hit a pack/packet start
	// before we hit an I frame, we don't want the pack/packet position to remain at 0.
	// It has to be at least equal to or greater than the starting position in the stream.
	CurrentPackHeaderPosition = PackHeaderPosition = (process.startloc/SECTOR_SIZE)*SECTOR_SIZE;
 	if (MuxFile == (FILE *) 0 && process.locate == LOCATE_RIP)
		StartVideoDemux();
   Initialize_Buffer();

	timing.op = 0;
	for (;;)
	{
		Get_Hdr(0);
		if (Stop_Flag == true)
			ThreadKill(MISC_KILL);
		if (picture_coding_type == I_TYPE) break;
	}
	Start_Flag = true;
	process.file = d2v_current.file;
	process.lba = d2v_current.lba;

	Decode_Picture();

	timing.op = timing.mi = timeGetTime();

    field = 0;
	for (;;)
	{
		Get_Hdr(0);
		if (Stop_Flag && (!field || picture_structure == FRAME_PICTURE))
		{
			// Stop only after every two fields if we are decoding field pictures.
			Write_Frame(current_frame, d2v_current, Frame_Number - 1);
			ThreadKill(MISC_KILL);
		}
		Decode_Picture();
		field ^= 1;
		if (Stop_Flag && (!field || picture_structure == FRAME_PICTURE))
		{
			// Stop only after every two fields if we are decoding field pictures.
			Write_Frame(current_frame, d2v_current, Frame_Number - 1);
			ThreadKill(MISC_KILL);
		}
	}
	return 0;
}

static BOOL GOPBack()
{
	int i;
	int startfile;
	__int64 startloc, endloc, curloc, startlba;

	startfile = process.startfile;
	startloc = process.startloc;
	startlba = process.lba;

	for (;;)
	{
		endloc = startloc;
		startloc -= SECTOR_SIZE<<4;

		if (startloc >= 0)
		{
			process.startfile = startfile;
			process.startloc = startloc;
		}
		else
		{
			process.startloc = 0;
			return false;
		}

		_lseeki64(Infile[process.startfile], process.startloc, SEEK_SET);
		Initialize_Buffer();

		for (;;)
		{
			curloc = _telli64(Infile[process.startfile]);
			if (curloc >= endloc) break;
			Get_Hdr(0);
			if (Stop_Flag == true)
				ThreadKill(MISC_KILL);
			if (picture_structure != FRAME_PICTURE)
			{
				Get_Hdr(0);
				if (Stop_Flag == true)
					ThreadKill(MISC_KILL);
			}
			if (picture_coding_type==I_TYPE)
			{
				if (d2v_current.file > startfile)
				{
					// We've crossed back into the next file and found
					// the first I frame. That's not the one we are looking for,
					// so go back again and keep looking.
					CurrentFile = startfile;
					process.run = 0;
					for (i=0; i < startfile; i++)
						process.run += Infilelength[i];
					break;
				}
				if (d2v_current.lba == startlba)
				{
					// We've found the I frame we are trying to step back from.
					// That's not the one we want, so keep looking.
					break;
				}
				// We've found the I frame we want!
				process.startfile = d2v_current.file;
				process.startloc = d2v_current.lba * SECTOR_SIZE;
				// This is a kludge. For PES streams, the pack start
				// might be in the previous LBA!
				if (SystemStream_Flag != ELEMENTARY_STREAM)
					process.startloc -= SECTOR_SIZE + 4;
				return true;
			}
		}
	}
}

static int ChromaFormat[4] = {
	0, 6, 8, 12
};

static void InitialDecoder()
{
	int i, size;
	extern int Clip_Width, Clip_Height;

	mb_width = (horizontal_size+15)/16;
	mb_height = progressive_sequence ? (vertical_size+15)/16 : 2*((vertical_size+31)/32);

	Coded_Picture_Width = 16 * mb_width;
	Coded_Picture_Height = 16 * mb_height;

	block_count = ChromaFormat[chroma_format];

	for (i=0; i<3; i++)
	{
		if (i==0)
			size = Coded_Picture_Width * Coded_Picture_Height;
		else
			size = ((chroma_format==CHROMA444) ? Coded_Picture_Width : Coded_Picture_Width>>1) *
				   ((chroma_format!=CHROMA420) ? Coded_Picture_Height : Coded_Picture_Height>>1);

		backward_reference_frame[i] = (unsigned char*)_aligned_malloc(size, 64);
		forward_reference_frame[i] = (unsigned char*)_aligned_malloc(size, 64);
		auxframe[i] = (unsigned char*)_aligned_malloc(size, 64);
	}

	u422 = (unsigned char*)_aligned_malloc(Coded_Picture_Width*Coded_Picture_Height/2, 64);
	v422 = (unsigned char*)_aligned_malloc(Coded_Picture_Width*Coded_Picture_Height/2, 64);
	u444 = (unsigned char*)_aligned_malloc(Coded_Picture_Width*Coded_Picture_Height, 64);
	v444 = (unsigned char*)_aligned_malloc(Coded_Picture_Width*Coded_Picture_Height, 64);
	rgb24 = (unsigned char*)_aligned_malloc(Coded_Picture_Width*Coded_Picture_Height*3, 64);
	rgb24small = (unsigned char*)_aligned_malloc(Coded_Picture_Width*Coded_Picture_Height*3, 64);
	yuy2 = (unsigned char*)_aligned_malloc(Coded_Picture_Width*Coded_Picture_Height*2, 64);
	lum = (unsigned char*)_aligned_malloc(Coded_Picture_Width*Coded_Picture_Height, 64);
	Clip_Width = horizontal_size;
	Clip_Height = vertical_size;
}

void setRGBValues()
{
	if (Scale_Flag)
	{
		RGB_Scale = 0x1000254310002543;
		RGB_Offset = 0x0010001000100010;
		if (matrix_coefficients == 7) // SMPTE 240M (1987)
		{
			RGB_CBU = 0x0000428500004285;
			RGB_CGX = 0xF7BFEEA3F7BFEEA3;
			RGB_CRV = 0x0000396900003969;
		}
		else if (matrix_coefficients == 6 || matrix_coefficients == 5) // SMPTE 170M/ITU-R BT.470-2 -- BT.601
		{
			RGB_CBU = 0x0000408D0000408D;
			RGB_CGX = 0xF377E5FCF377E5FC;
			RGB_CRV = 0x0000331300003313;
		}
		else if (matrix_coefficients == 4) // FCC
		{
			RGB_CBU = 0x000040D8000040D8;
			RGB_CGX = 0xF3E9E611F3E9E611;
			RGB_CRV = 0x0000330000003300;
		}
		else // ITU-R Rec.709 (1990) -- BT.709
		{
			RGB_CBU = 0x0000439A0000439A;
			RGB_CGX = 0xF92CEEF1F92CEEF1;
			RGB_CRV = 0x0000395F0000395F;
		}
	}
	else
	{
		RGB_Scale = 0x1000200010002000;
		RGB_Offset = 0x0000000000000000;
		if (matrix_coefficients == 7) // SMPTE 240M (1987)
		{
			RGB_CBU = 0x00003A6F00003A6F;
			RGB_CGX = 0xF8C0F0BFF8C0F0BF;
			RGB_CRV = 0x0000326E0000326E;
		}
		else if (matrix_coefficients == 6 || matrix_coefficients == 5) // SMPTE 170M/ITU-R BT.470-2 -- BT.601
		{
			RGB_CBU = 0x000038B4000038B4;
			RGB_CGX = 0xF4FDE926F4FDE926;
			RGB_CRV = 0x00002CDD00002CDD;
		}
		else if (matrix_coefficients == 4) // FCC
		{
			RGB_CBU = 0x000038F6000038F6;
			RGB_CGX = 0xF561E938F561E938;
			RGB_CRV = 0x00002CCD00002CCD;
		}
		else // ITU-R Rec.709 (1990) -- BT.709
		{
			RGB_CBU = 0x00003B6200003B62;
			RGB_CGX = 0xFA00F104FA00F104;
			RGB_CRV = 0x0000326600003266;
		}
	}
}
