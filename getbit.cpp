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

#define GETBIT_GLOBAL
#include "global.h"
#include "getbit.h"
#include "AC3Dec\ac3.h"

unsigned int start;

int _donread(int fd, void *buffer, unsigned int count)
{
	int bytes;
	bytes = _read(fd, buffer, count);
	return bytes;
}

#define MONO 3
#define STEREO 0
#define LAYER1 3
#define LAYER2 2
#define LAYER3 1
#define LAYERRESERVED 0
#define SAMPLE32K 2
#define SAMPLE44K 0
#define SAMPLERESERVED 3
#define BITRATERESERVED 0xf

int check_audio_syncword(unsigned int audio_id, int layer, int bitrate, int sample, int mode, int emphasis)
{
    // Try to validate this syncword by validating semantics of the audio header.
    // We're trying to filter out emulated syncwords.
    if (layer == LAYERRESERVED)
        return 1;
    if (bitrate == BITRATERESERVED)
        return 1;
    if (sample == SAMPLERESERVED)
        return 1;
    if (layer == LAYER2)
    {
        if ((bitrate == 1 || bitrate == 2 || bitrate == 3 || bitrate == 5) && mode != MONO)
            return 1;
        if ((bitrate == 11 || bitrate == 12 || bitrate == 13 || bitrate == 14) && mode == MONO)
            return 1;
    }
    // Emphasis is almost never used. It's less likely than hitting an emulated header. :-)
//    if (emphasis != 0)
//        return 1;
	// The header appears to be valid. Store the audio characteristics.
	audio[audio_id].layer = layer;
	audio[audio_id].rate = bitrate;
	audio[audio_id].sample = sample;
	audio[audio_id].mode = mode;
    return 0;
}

int PTSDifference(unsigned int apts, unsigned int vpts, int *result)
{
	int diff;

	if (apts > vpts)
	{
		diff = (apts - vpts) / 90;
//		if (diff > 5000) return 1;
		*result = diff;
	}
	else
	{
		diff = (vpts - apts) / 90;
//		if (diff > 5000) return 1;
		*result = -diff;
	}
    if (diff > 1000 && (D2V_Flag || AudioOnly_Flag) && !CLIActive)
    {
		MessageBox(hWnd,
                    "The calculated audio delay is unusually large. This is sometimes\n"
                    "caused by an initial black video lead in that does not have valid\n"
                    "timestamps. You can try setting your project range to skip this\n"
                    "portion of the video. Use the > button to skip GOPS and then hit\n"
                    "the [ button to set the start of the project. This may give you a\n"
                    "valid delay value.",
                     "Audio Delay Warning",
                     MB_OK | MB_ICONWARNING);
        // Reset data timeout.
        start = timeGetTime();
    }
	return 0;
}


FILE *OpenAudio(char *path, char *mode, unsigned int id)
{
	// Pick up the first audio file path for
	// use in the AVS template.
	if (id < LowestAudioId)
    {
		strcpy(AudioFilePath, path);
        LowestAudioId = id;
    }
	return fopen(path, mode);
}

#define LOCATE												\
while (Rdptr >= (Rdbfr + BUFFER_SIZE))						\
{															\
	Read = _donread(Infile[CurrentFile], Rdbfr, BUFFER_SIZE);	\
	if (Read < BUFFER_SIZE) Next_File();					\
	Rdptr -= BUFFER_SIZE;									\
}

#define DECODE_AC3																\
{																				\
	if (SystemStream_Flag == TRANSPORT_STREAM && TransportPacketSize == 204)	\
		Packet_Length -= 16;													\
	size = 0;																	\
	while (Packet_Length > 0)													\
	{																			\
		if (Packet_Length+Rdptr > BUFFER_SIZE+Rdbfr)							\
		{																		\
			size = ac3_decode_data(Rdptr, BUFFER_SIZE+Rdbfr-Rdptr, size);		\
			Packet_Length -= BUFFER_SIZE+Rdbfr-Rdptr;							\
			Read = _donread(Infile[CurrentFile], Rdbfr, BUFFER_SIZE);			\
			if (Read < BUFFER_SIZE) Next_File();								\
			Rdptr = Rdbfr;														\
		}																		\
		else																	\
		{																		\
			size = ac3_decode_data(Rdptr, Packet_Length, size);					\
			Rdptr += Packet_Length;												\
			Packet_Length = 0;													\
		}																		\
	}																			\
}

#define DEMUX_AC3																\
{																				\
	if (SystemStream_Flag == TRANSPORT_STREAM && TransportPacketSize == 204)	\
		Packet_Length -= 16;													\
	while (Packet_Length > 0)													\
	{																			\
		if (Packet_Length+Rdptr > BUFFER_SIZE+Rdbfr)							\
		{																		\
			fwrite(Rdptr, BUFFER_SIZE+Rdbfr-Rdptr, 1, audio[AUDIO_ID].file);	\
			Packet_Length -= BUFFER_SIZE+Rdbfr-Rdptr;							\
			Read = _donread(Infile[CurrentFile], Rdbfr, BUFFER_SIZE);			\
			if (Read < BUFFER_SIZE) Next_File();								\
			Rdptr = Rdbfr;														\
		}																		\
		else																	\
		{																		\
			fwrite(Rdptr, Packet_Length, 1, audio[AUDIO_ID].file);				\
			Rdptr += Packet_Length;												\
			Packet_Length = 0;													\
		}																		\
	}																			\
}

void DemuxLPCM(int *size, int *Packet_Length, unsigned char PCM_Buffer[], unsigned char format)
{
	unsigned char tmp[12];
	int i;

	*size = 0;
	while (*Packet_Length > 0)
	{
		if (*Packet_Length + Rdptr > BUFFER_SIZE + Rdbfr)
		{
			memcpy(PCM_Buffer + *size, Rdptr, BUFFER_SIZE + Rdbfr - Rdptr);
			*size += (BUFFER_SIZE + Rdbfr - Rdptr);
			*Packet_Length -= (BUFFER_SIZE + Rdbfr - Rdptr);
			Read = _donread(Infile[CurrentFile], Rdbfr, BUFFER_SIZE);
			if (Read < BUFFER_SIZE)
				Next_File();
			Rdptr = Rdbfr;
		}
		else
		{
			memcpy(PCM_Buffer + *size, Rdptr, *Packet_Length);
			Rdptr += *Packet_Length;
			*size += *Packet_Length;
			*Packet_Length = 0;
		}
	}
	if ((format & 0xc0) == 0)
	{
		// 16-bit LPCM.
		for (i = 0; i < *size; i += 2)
		{
			tmp[0] = PCM_Buffer[i];
			PCM_Buffer[i] = PCM_Buffer[i+1];
			PCM_Buffer[i+1] = tmp[0];
		}
	}
	else
	{
		// 24-bit LPCM.
		if ((format & 0x07) + 1 == 1)
		{
			// Mono
			for (i = 0; i < *size; i += 6)
			{
				tmp[0]=PCM_Buffer[i+4];
				tmp[1]=PCM_Buffer[i+1];
				tmp[2]=PCM_Buffer[i+0];
				tmp[3]=PCM_Buffer[i+5];
				tmp[4]=PCM_Buffer[i+3];
				tmp[5]=PCM_Buffer[i+2];
				memcpy(&PCM_Buffer[i], tmp, 6);
			}
		}
		else
		{
			// Stereo
			for (i = 0; i < *size; i += 12)
			{
				tmp[0] = PCM_Buffer[i+8];
				tmp[1] = PCM_Buffer[i+1];
				tmp[2] = PCM_Buffer[i+0];
				tmp[3] = PCM_Buffer[i+9];
				tmp[4] = PCM_Buffer[i+3];
				tmp[5] = PCM_Buffer[i+2];
				tmp[6] = PCM_Buffer[i+10];
				tmp[7] = PCM_Buffer[i+5];
				tmp[8] = PCM_Buffer[i+4];
				tmp[9] = PCM_Buffer[i+11];
				tmp[10] = PCM_Buffer[i+7];
				tmp[11] = PCM_Buffer[i+6];
				memcpy(&PCM_Buffer[i], tmp, 12);
			}
		}
	}
}

#define DEMUX_MPA_AAC(fp)														\
{																				\
	if (SystemStream_Flag == TRANSPORT_STREAM && TransportPacketSize == 204)	\
		Packet_Length -= 16;													\
	while (Packet_Length > 0)													\
	{																			\
		if (Packet_Length+Rdptr > BUFFER_SIZE+Rdbfr)							\
		{																		\
			fwrite(Rdptr, BUFFER_SIZE+Rdbfr-Rdptr, 1, (fp));					\
			Packet_Length -= BUFFER_SIZE+Rdbfr-Rdptr;							\
			Read = _donread(Infile[CurrentFile], Rdbfr, BUFFER_SIZE);			\
			if (Read < BUFFER_SIZE) Next_File();								\
			Rdptr = Rdbfr;														\
		}																		\
		else																	\
		{																		\
			fwrite(Rdptr, Packet_Length, 1, (fp));								\
			Rdptr += Packet_Length;												\
			Packet_Length = 0;													\
		}																		\
	}																			\
}

#define DEMUX_DTS																\
{																				\
	if (SystemStream_Flag == TRANSPORT_STREAM && TransportPacketSize == 204)	\
		Packet_Length -= 16;													\
	while (Packet_Length > 0)													\
	{																			\
		if (Packet_Length+Rdptr > BUFFER_SIZE+Rdbfr)							\
		{																		\
			fwrite(Rdptr, BUFFER_SIZE+Rdbfr-Rdptr, 1, audio[AUDIO_ID].file);	\
			Packet_Length -= BUFFER_SIZE+Rdbfr-Rdptr;							\
			Read = _donread(Infile[CurrentFile], Rdbfr, BUFFER_SIZE);			\
			if (Read < BUFFER_SIZE) Next_File();								\
			Rdptr = Rdbfr;														\
		}																		\
		else																	\
		{																		\
			fwrite(Rdptr, Packet_Length, 1, audio[AUDIO_ID].file);				\
			Rdptr += Packet_Length;												\
			Packet_Length = 0;													\
		}																		\
	}																			\
}

static char *FTType[5] = {
	"48KHz", "44.1KHz", "44.1KHz", "44.1KHz", "44.1KHz"
};

static char *AC3ModeDash[8] = {
	"1+1", "1_0", "2_0", "3_0", "2_1", "3_1", "2_2", "3_2"
};

static char *AC3Mode[8] = {
	"1+1", "1/0", "2/0", "3/0", "2/1", "3/1", "2/2", "3/2"
};

static int AC3Rate[32] = {
	32, 40, 48, 56, 64, 80, 96, 112, 128, 160,
	192, 224, 256, 320, 384, 448, 512, 576, 640,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static char *MPALayer[4] = {
	"", "L3", "L2", "L1"
};

static char *MPAExtension[4] = {
	"mpa", "mp3", "mp2", "mp1"
};

static char *MPAMode[4] = {
	"2ch", "2ch", "2ch", "mono"
};

static char *MPASample[3] = {
	"44.1", "48", "32"
};

static char *MPARate[4][15] = {
	{	"", "", "", "", "", "", "", "", "", "", "", "", "", "", "" }, // Layer reserved
	{	"free", "32", "40", "48", "56", "64", "80", "96", "112", "128", "160", "192", "224", "256", "320" }, // Layer 3
	{	"free", "32", "48", "56", "64", "80", "96", "112", "128", "160", "192", "224", "256", "320", "384" }, // Layer 2
	{	"free", "32", "64", "96", "128", "160", "192", "224", "256", "288", "320", "352", "384", "416", "448" }  // Layer 1
};

unsigned int VideoPTS, AudioPTS;
static unsigned char PCM_Buffer[SECTOR_SIZE];
static short *ptrPCM_Buffer = (short*)PCM_Buffer;

unsigned char *buffer_invalid;

void VideoDemux(void);

void Initialize_Buffer()
{
	Rdptr = Rdbfr + BUFFER_SIZE;
	Rdmax = Rdptr;
	buffer_invalid = (unsigned char *) 0xffffffff;

	if (SystemStream_Flag != ELEMENTARY_STREAM && !AudioOnly_Flag)
	{
		if (Rdptr >= Rdmax)
			Next_Packet();
		CurrentBfr = *Rdptr++ << 24;

		if (Rdptr >= Rdmax)
			Next_Packet();
		CurrentBfr += *Rdptr++ << 16;

		if (Rdptr >= Rdmax)
			Next_Packet();
		CurrentBfr += *Rdptr++ << 8;

		if (Rdptr >= Rdmax)
			Next_Packet();
		CurrentBfr += *Rdptr++;

		Fill_Next();
	}
	else
	{
		Fill_Buffer();

		CurrentBfr = (*Rdptr << 24) + (*(Rdptr+1) << 16) + (*(Rdptr+2) << 8) + *(Rdptr+3);
		Rdptr += 4;

		Fill_Next();
	}

	BitsLeft = 32;
}

// Skips ahead in transport stream by specified number of bytes.
#define SKIP_TRANSPORT_PACKET_BYTES(bytes_to_skip)					\
{																	\
	int temp = (bytes_to_skip);										\
	while (temp  > 0) 												\
	{ 																\
		if (temp + Rdptr > BUFFER_SIZE + Rdbfr)						\
		{ 															\
			temp  -= BUFFER_SIZE + Rdbfr - Rdptr;					\
			Read = _donread(Infile[CurrentFile], Rdbfr, BUFFER_SIZE);	\
			if (Read < BUFFER_SIZE) Next_File();					\
			Rdptr = Rdbfr;											\
		} 															\
		else														\
		{ 															\
			Rdptr += temp; 											\
			temp = 0;												\
		} 															\
	}																\
	Packet_Length -= (bytes_to_skip);								\
}

// Transport packet data structure.
typedef struct
{			
	// 1 byte
	unsigned char sync_byte;							// 8 bits

	// 2 bytes
	unsigned char transport_error_indicator;			// 1 bit
	unsigned char payload_unit_start_indicator;			// 1 bit
	unsigned char transport_priority;					// 1 bit
	unsigned short pid;									// 13 bits

	// 1 byte
	unsigned char transport_scrambling_control;			// 2 bits
	unsigned char adaptation_field_control;				// 2 bits
	unsigned char continuity_counter;					// 4 bits

	// 1 byte
	unsigned char adaptation_field_length;				// 8 bits 
	
	// 1 byte
	unsigned char discontinuity_indicator;				// 1 bit
	unsigned char random_access_indicator;				// 1 bit
	unsigned char elementary_stream_priority_indicator;	// 1 bit
	unsigned char PCR_flag;								// 1 bit
	unsigned char OPCR_flag;							// 1 bit
	unsigned char splicing_point_flag;					// 1 bit
	unsigned char transport_private_data_flag;			// 1 bit
	unsigned char adaptation_field_extension_flag;		// 1 bit
} transport_packet;

transport_packet tp_zeroed = { 0 };

FILE *mpafp = NULL;
FILE *mpvfp = NULL;
FILE *pcmfp = NULL;

// ATSC transport stream parser.
// We ignore the 'continuity counter' because with some DTV
// broadcasters, this isnt a reliable indicator.
void Next_Transport_Packet()
{
	static int i, Packet_Length, Packet_Header_Length, size;
	static unsigned int code, flags, VOBCELL_Count, AUDIO_ID = 0;
	__int64 PES_PTS, PES_DTS;
	unsigned int pts_stamp = 0, dts_stamp = 0;
	int PTSDiff;
	unsigned int bytes_left;
	transport_packet tp;
	unsigned int time;
	double picture_period;
	char ext[4], EXT[4];

	start = timeGetTime();
	for (;;)
	{
		PES_PTS = 0;
		bytes_left = 0;
		Packet_Length = TransportPacketSize; // total length of an MPEG-2 transport packet
		tp = tp_zeroed; // to avoid warnings

        // If the packet size is 192, then assume it is an M2TS (blueray) file, which has 4 extra bytes
        // in front of the sync byte.
        if (TransportPacketSize == 192)
        {
            // Suck up the extra bytes.
            Get_Byte();
            Get_Byte();
            Get_Byte();
            Get_Byte();
            Packet_Length -= 4;
        }
retry_sync:
		// Don't loop forever. If we don't get data
		// in a reasonable time (5 secs) we exit.
		time = timeGetTime();
		if (time - start > 5000)
		{
			MessageBox(hWnd, "Cannot find audio or video data. Ensure that your PIDs\nare set correctly in the Stream menu. Refer to the\nUsers Manual for details.",
					   NULL, MB_OK | MB_ICONERROR);
			ThreadKill(MISC_KILL);
		}

		// Search for a sync byte. Gives some protection against emulation.
		if (Stop_Flag)
			ThreadKill(MISC_KILL);
		if (Get_Byte() != 0x47)
			goto retry_sync;
		if (Rdptr - Rdbfr > TransportPacketSize)
		{
			if (Rdptr[-(TransportPacketSize+1)] != 0x47)
				goto retry_sync;
		}
		else if (Rdbfr + Read - Rdptr > TransportPacketSize - 1)
		{
			if (Rdptr[+(TransportPacketSize-1)] != 0x47)
				goto retry_sync;
		}
		else
		{
			// We can't check so just accept this sync byte.
		}

		// Record the location of the start of the packet. This will be used
		// for indexing when an I frame is detected.
		if (D2V_Flag)
		{
			PackHeaderPosition = _telli64(Infile[CurrentFile])
								 - (__int64)BUFFER_SIZE + (__int64)Rdptr - (__int64)Rdbfr - 1;
		}
        // For M2TS (blueray) files, index the extra 4 bytes in front of the sync byte,
        // because DGDecode will expect to see them.
        if (TransportPacketSize == 192)
            PackHeaderPosition -= 4;
		--Packet_Length; // swallow the sync_byte

		code = Get_Short();
		Packet_Length = Packet_Length - 2; // swallow the two bytes we just got
		tp.pid = (unsigned short) (code & 0x1FFF);
		tp.transport_error_indicator = (unsigned char) ((code >> 15) & 0x01);
		tp.payload_unit_start_indicator = (unsigned char) ((code >> 14) & 0x01);
		tp.transport_priority = (unsigned char) ((code >> 13) & 0x01);

		if (tp.transport_error_indicator)
		{
			// Skip remaining bytes in current packet.
			SKIP_TRANSPORT_PACKET_BYTES(Packet_Length);
			// Try the next packet in the stream.
			continue;
		}

		code = Get_Byte();
		--Packet_Length; // decrement the 1 byte we just got;
		tp.transport_scrambling_control = (unsigned char) ((code >> 6) & 0x03);//		2	bslbf
		tp.adaptation_field_control = (unsigned char) ((code >> 4 ) & 0x03);//		2	bslbf
		tp.continuity_counter = (unsigned char) (code & 0x0F);//		4	uimsbf

		// we don't care about the continuity counter

		if (tp.adaptation_field_control == 0)
		{
			// no payload
			// skip remaining bytes in current packet
			SKIP_TRANSPORT_PACKET_BYTES( Packet_Length )
			continue;
		}

		// 3) check the Adaptation-header, only so we can determine
		//    the exact #bytes to skip
		if ( tp.adaptation_field_control == 2 || tp.adaptation_field_control == 3)
		{
			// adaptation field is present
			tp.adaptation_field_length = (unsigned char) Get_Byte(); // 8-bits
			--Packet_Length; // decrement the 1 byte we just got;

			if ( tp.adaptation_field_length != 0 ) // end of field already?
			{
				code = Get_Byte();
				--Packet_Length; // decrement the 1 byte we just got;
				tp.discontinuity_indicator = (unsigned char) ((code >> 7) & 0x01); //	1	bslbf
				tp.random_access_indicator = (unsigned char) ((code >> 6) & 0x01); //	1	bslbf
				tp.elementary_stream_priority_indicator = (unsigned char) ((code >> 5) & 0x01); //	1	bslbf
				tp.PCR_flag = (unsigned char) ((code >> 4) & 0x01); //	1	bslbf
				tp.OPCR_flag = (unsigned char) ((code >> 3) & 0x01); //	1	bslbf
				tp.splicing_point_flag = (unsigned char) ((code >> 2) & 0x01); //	1	bslbf
				tp.transport_private_data_flag = (unsigned char) ((code >> 1) & 0x01); //	1	bslbf
				tp.adaptation_field_extension_flag = (unsigned char) ((code >> 0) & 0x01); //	1	bslbf
				bytes_left = tp.adaptation_field_length - 1;
				if (LogTimestamps_Flag && D2V_Flag && tp.PCR_flag && tp.pid == MPEG2_Transport_PCRPID  && StartLogging_Flag)
				{
					__int64 PCR, PCRbase, PCRext, tmp;
					char pcr[64];

					tmp = (__int64) Get_Byte();
					PCRbase = tmp << 25;
					tmp = (__int64) Get_Byte();
					PCRbase |= tmp << 17;
					tmp = (__int64) Get_Byte();
					PCRbase |= tmp << 9;
					tmp = (__int64) Get_Byte();
					PCRbase |= tmp << 1;
					tmp = (__int64) Get_Byte();
					PCRbase |= tmp >> 7;
					PCRext = (tmp & 0x01) << 8;
					tmp = (__int64) Get_Byte();
					PCRext |= tmp;
					PCR = 300 * PCRbase + PCRext;
					_i64toa(PCR, pcr, 10);
					fprintf(Timestamps, "PCR %s ", pcr);
					_i64toa(PCR/27000, pcr, 10);
					bytes_left -= 6;
                    Packet_Length -= 6;
					fprintf(Timestamps, "[%sms]\n", pcr);
				}

				// skip the remainder of the adaptation_field
				SKIP_TRANSPORT_PACKET_BYTES( bytes_left )
			} // if ( tp.adaptation_field_length != 0 )
		} // if ( tp.adaptation_field_control != 1 )

		// We've processed the MPEG-2 transport header. 
		// Any data left in the current transport packet?
		if (Packet_Length == 0)
			continue;

		if (tp.pid && tp.pid == MPEG2_Transport_VideoPID) 
		{
			LOCATE

			if (tp.payload_unit_start_indicator)
			{
				Get_Short();
				Get_Short();
				Get_Short(); // MPEG2-PES total Packet_Length
				Get_Byte(); // skip a byte
				flags = Get_Byte();
				Packet_Header_Length = Get_Byte();
				Packet_Length = Packet_Length - 9; // compensate the bytes we extracted
	
				// Get timestamp, and skip rest of PES-header.
				if ((flags & 0x80) && (Packet_Header_Length > 4))
				{
					PES_PTS = (Get_Byte() & 0x0e) << 29;
					PES_PTS |= (Get_Short() & 0xfffe) << 14;
					PES_PTS |= (Get_Short()>>1) & 0x7fff;
					pts_stamp = (unsigned int) (PES_PTS & 0xffffffff);
					if (LogTimestamps_Flag && D2V_Flag && StartLogging_Flag)
						fprintf(Timestamps, "V PTS %u [%ums]\n", pts_stamp, pts_stamp/90);
					Packet_Length -= 5;
					// DTS is not used. The code is here for analysis and debugging.
					if ((flags & 0xc0) == 0xc0)
					{
						PES_DTS = (Get_Byte() & 0x0e) << 29;
						PES_DTS |= (Get_Short() & 0xfffe) << 14;
						PES_DTS |= (Get_Short()>>1) & 0x7fff;
						dts_stamp = (unsigned int) (PES_DTS & 0xffffffff);
						if (LogTimestamps_Flag && D2V_Flag && StartLogging_Flag)
							fprintf(Timestamps, "V DTS %u [%ums]\n", dts_stamp, dts_stamp/90);
						Packet_Length -= 5;
						SKIP_TRANSPORT_PACKET_BYTES(Packet_Header_Length - 10)
					}
					else
						SKIP_TRANSPORT_PACKET_BYTES(Packet_Header_Length - 5)
				}
				else
					SKIP_TRANSPORT_PACKET_BYTES(Packet_Header_Length);
				if (!Start_Flag)
				{
					// Start_Flag becomes true after the first I frame.
					// So VideoPTS will be left at the value corresponding to
					// the first I frame.
					VideoPTS = pts_stamp;
				}
			}

			Rdmax = Rdptr + Packet_Length;
			if (TransportPacketSize == 204)
				Rdmax -= 16;

			Bitrate_Monitor += (Rdmax - Rdptr);
            if (AudioOnly_Flag)
				SKIP_TRANSPORT_PACKET_BYTES(Packet_Length);
			return;
		}

		else if ((Method_Flag == AUDIO_DEMUXALL || Method_Flag == AUDIO_DEMUX) && Start_Flag &&
				 tp.pid && (tp.pid == MPEG2_Transport_AudioPID) &&
				 (MPEG2_Transport_AudioType == 3 || MPEG2_Transport_AudioType == 4 || MPEG2_Transport_AudioType == 0xffffffff)) 
		{
			// Both MPEG and AAC audio come here. The sync word will be checked to
			// distinguish between them.
			if (mpafp)
			{
				if (tp.payload_unit_start_indicator)
				{
					Get_Short(); // start code and stream id
					Get_Short();
					Get_Short(); // packet length
					Get_Byte(); // flags
					code = Get_Byte(); // more flags
					Packet_Header_Length = Get_Byte();
					Packet_Length -= 9;
					if (code & 0x80)
					{
//						unsigned int dts_stamp;
						code = Get_Byte();
						PES_PTS = (code & 0x0e) << 29;
						PES_PTS |= (Get_Short() & 0xfffe) << 14;
						PES_PTS |= (Get_Short()>>1) & 0x7fff;
						AudioPTS = (unsigned int) (PES_PTS & 0xffffffff);
						if (LogTimestamps_Flag && D2V_Flag && StartLogging_Flag)
							fprintf(Timestamps, " A00 PTS %u [%ums]\n", AudioPTS, AudioPTS/90);
						Packet_Length -= 5;
#if 0
						if ((code & 0xc0) == 0xc0)
						{
							PES_DTS = (Get_Byte() & 0x0e) << 29;
							PES_DTS |= (Get_Short() & 0xfffe) << 14;
							PES_DTS |= (Get_Short()>>1) & 0x7fff;
							dts_stamp = (unsigned int) (PES_DTS & 0xffffffff);
							if (LogTimestamps_Flag && D2V_Flag && StartLogging_Flag)
								fprintf(Timestamps, "A DTS %u [%ums]\n", dts_stamp, dts_stamp/90);
							Packet_Length -= 5;
							SKIP_TRANSPORT_PACKET_BYTES(Packet_Header_Length - 10)
						}
						else
#endif
							SKIP_TRANSPORT_PACKET_BYTES(Packet_Header_Length-5)
					}
				}
				DEMUX_MPA_AAC(mpafp);
			}
			else if (tp.payload_unit_start_indicator)
			{
				Get_Short(); // start code
				Get_Short(); // rest of start code and stream id
				Get_Short(); // packet length
				Get_Byte(); // flags
				code = Get_Byte(); // more flags
				Packet_Header_Length = Get_Byte();
				Packet_Length -= 9;
				if (code & 0x80)
				{
//					unsigned int dts_stamp;
					code = Get_Byte();
					PES_PTS = (code & 0x0e) << 29;
					PES_PTS |= (Get_Short() & 0xfffe) << 14;
					PES_PTS |= (Get_Short()>>1) & 0x7fff;
					AudioPTS = (unsigned int) (PES_PTS & 0xffffffff);
					if (LogTimestamps_Flag && D2V_Flag && StartLogging_Flag)
						fprintf(Timestamps, " A00 PTS %u [%ums]\n", AudioPTS, AudioPTS/90);
					Packet_Length = Packet_Length - 5;
#if 0
					if ((code & 0xc0) == 0xc0)
					{
						PES_DTS = (Get_Byte() & 0x0e) << 29;
						PES_DTS |= (Get_Short() & 0xfffe) << 14;
						PES_DTS |= (Get_Short()>>1) & 0x7fff;
						dts_stamp = (unsigned int) (PES_DTS & 0xffffffff);
						if (LogTimestamps_Flag && D2V_Flag && StartLogging_Flag)
							fprintf(Timestamps, "A DTS %u [%ums]\n", dts_stamp, dts_stamp/90);
						Packet_Length -= 5;
						SKIP_TRANSPORT_PACKET_BYTES(Packet_Header_Length - 10)
					}
					else
#endif
						SKIP_TRANSPORT_PACKET_BYTES(Packet_Header_Length-5)

					// Now we're at the start of the audio.
					// Find the audio header.
					code = Get_Byte();
					code = ((code & 0xff) << 8) | Get_Byte();
					Packet_Length -= 2;
					while ((code & 0xfff8) != 0xfff8 && Packet_Length > 0)
					{
emulated3:
						code = ((code & 0xff) << 8) | Get_Byte();
						Packet_Length--;
					}
					if ((code & 0xfff8) != 0xfff8)
					{
						SKIP_TRANSPORT_PACKET_BYTES(Packet_Length);
						continue;
					}
					// Found the audio header. Now check the layer field.
					// For MPEG, layer is 1, 2, or 3. For AAC, it is 0.
					// We demux the same for both; only the filename we create is different.
					if (((code & 6) >> 1) == 0x00)
					{
						// AAC audio.
						audio[0].type = FORMAT_AAC;
					}
					else
					{
						// MPEG audio.
                        // Try to detect emulated sync words by enforcing semantics.
                        if (check_audio_syncword(0, (code >> 1) & 3,  (Rdptr[0] >> 4) & 0xf, (Rdptr[0] >> 2) & 3, (Rdptr[1] >> 6) & 3, Rdptr[1] & 3))
                        {
                                goto emulated3;
                        }
						audio[0].type = FORMAT_MPA;
					}
					if (AudioOnly_Flag)
					{
						char *p;
						strcpy(szOutput, Infilename[0]);
						p = &szOutput[sizeof(szOutput)];
						while (*p != '.') p--;
						*p = 0;
						if (audio[0].type == FORMAT_AAC)
							sprintf(szBuffer, "%s PID %03x.aac", szOutput, MPEG2_Transport_AudioPID);
						else
							sprintf(szBuffer, "%s PID %03x %s %s %s %s.%s", szOutput, MPEG2_Transport_AudioPID,
									MPALayer[audio[0].layer], MPAMode[audio[0].mode], MPASample[audio[0].sample],
									MPARate[audio[0].layer][audio[0].rate],
                                    UseMPAExtensions ? MPAExtension[0] : MPAExtension[audio[0].layer]);
					}
					else
					{
						// Adjust the VideoPTS to account for frame reordering.
						if (!PTSAdjustDone)
						{
							PTSAdjustDone = 1;
							picture_period = 1.0 / frame_rate;
							VideoPTS -= (int) (LeadingBFrames * picture_period * 90000);
						}

						if (PTSDifference(AudioPTS, VideoPTS, &PTSDiff))
						{
							if (audio[0].type == FORMAT_AAC)
								sprintf(szBuffer, "%s PID %03x.aac", szOutput, MPEG2_Transport_AudioPID);
							else
								sprintf(szBuffer, "%s PID %03x %s %s %s %s.%s", szOutput, MPEG2_Transport_AudioPID,
										MPALayer[audio[0].layer], MPAMode[audio[0].mode], MPASample[audio[0].sample],
										MPARate[audio[0].layer][audio[0].rate],
                                        UseMPAExtensions ? MPAExtension[0] : MPAExtension[audio[0].layer]);
						}
						else
						{
							if (audio[0].type == FORMAT_AAC)
								sprintf(szBuffer, "%s PID %03x DELAY %dms.aac", szOutput, MPEG2_Transport_AudioPID, PTSDiff);
							else
								sprintf(szBuffer, "%s PID %03x %s %s %s %s DELAY %dms.%s", szOutput, MPEG2_Transport_AudioPID,
										MPALayer[audio[0].layer], MPAMode[audio[0].mode], MPASample[audio[0].sample],
										MPARate[audio[0].layer][audio[0].rate], PTSDiff,
                                        UseMPAExtensions ? MPAExtension[0] : MPAExtension[audio[0].layer]);
 						}
					}
					Packet_Length += 2;
					Rdptr -= 2;
					if (D2V_Flag || AudioOnly_Flag)
					{
						mpafp = OpenAudio(szBuffer, "wb", 0);
						if (mpafp == NULL)
						{
							// Cannot open the output file, Disable further audio processing.
							MPEG2_Transport_AudioType = 0xff;
							SKIP_TRANSPORT_PACKET_BYTES(Packet_Length);
							continue;
						}
						DEMUX_MPA_AAC(mpafp);
					}
				}
			}
			if (AudioOnly_Flag && Info_Flag && !(AudioPktCount++ % 128))
				UpdateInfo();
		}

		else if ((Method_Flag == AUDIO_DEMUXALL || Method_Flag == AUDIO_DEMUX) && Start_Flag &&
				 tp.pid && (tp.pid == MPEG2_Transport_AudioPID) && (MPEG2_Transport_AudioType == 0x80)) 
		{
			if (pcmfp)
			{
				if (tp.payload_unit_start_indicator)
				{
					Get_Short(); // start code and stream id
					Get_Short();
					Get_Short(); // packet length
					Get_Byte(); // flags
					code = Get_Byte(); // more flags
					Packet_Header_Length = Get_Byte();
					Packet_Length -= 9;
					if (code & 0x80)
					{
						code = Get_Byte();
						PES_PTS = (code & 0x0e) << 29;
						PES_PTS |= (Get_Short() & 0xfffe) << 14;
						PES_PTS |= (Get_Short()>>1) & 0x7fff;
						AudioPTS = (unsigned int) (PES_PTS & 0xffffffff);
						if (LogTimestamps_Flag && D2V_Flag && StartLogging_Flag)
							fprintf(Timestamps, " A00 PTS %u [%ums]\n", AudioPTS, AudioPTS/90);
						Packet_Length -= 5;
						SKIP_TRANSPORT_PACKET_BYTES(Packet_Header_Length-5)
					}
					// Now we're at the start of the audio.
					code = Get_Byte();
					code = Get_Byte();
					audio[0].format_m2ts = Get_Short();
					Packet_Length -= 4;
				}
				DEMUX_MPA_AAC(pcmfp);
			}
			else if (tp.payload_unit_start_indicator)
			{
				audio[0].type = FORMAT_LPCM_M2TS;
				strcpy(ext, "pcm");
				strcpy(EXT, "pcm");
				_strupr(EXT);
				Get_Short(); // start code
				Get_Short(); // rest of start code and stream id
				Get_Short(); // packet length
				Get_Byte(); // flags
				code = Get_Byte(); // more flags
				Packet_Header_Length = Get_Byte();
				Packet_Length -= 9;
				if (code & 0x80)
				{
					code = Get_Byte();
					PES_PTS = (code & 0x0e) << 29;
					PES_PTS |= (Get_Short() & 0xfffe) << 14;
					PES_PTS |= (Get_Short()>>1) & 0x7fff;
					AudioPTS = (unsigned int) (PES_PTS & 0xffffffff);
					if (LogTimestamps_Flag && D2V_Flag && StartLogging_Flag)
						fprintf(Timestamps, " A00 PTS %u [%ums]\n", AudioPTS, AudioPTS/90);
					Packet_Length = Packet_Length - 5;
					SKIP_TRANSPORT_PACKET_BYTES(Packet_Header_Length-5)

					// Now we're at the start of the audio.
					code = Get_Byte();
					code = Get_Byte();
					audio[0].format_m2ts = Get_Short();
					Packet_Length -= 4;

					if (AudioOnly_Flag)
					{
						char *p;
						strcpy(szOutput, Infilename[0]);
						p = &szOutput[sizeof(szOutput)];
						while (*p != '.') p--;
						*p = 0;
						sprintf(szBuffer, "%s %s PID %03x.%s", szOutput, EXT, MPEG2_Transport_AudioPID, ext);
					}
					else
					{
						// Adjust the VideoPTS to account for frame reordering.
						if (!PTSAdjustDone)
						{
							PTSAdjustDone = 1;
							picture_period = 1.0 / frame_rate;
							VideoPTS -= (int) (LeadingBFrames * picture_period * 90000);
						}

						if (PTSDifference(AudioPTS, VideoPTS, &PTSDiff))
							sprintf(szBuffer, "%s %s PID %03x.%s",
									szOutput, EXT, MPEG2_Transport_AudioPID, ext);
						else
							sprintf(szBuffer, "%s %s PID %03x DELAY %dms.%s",
									szOutput, EXT, MPEG2_Transport_AudioPID, PTSDiff, ext);
					}
					if (D2V_Flag || AudioOnly_Flag)
					{
						pcmfp = OpenAudio(szBuffer, "wb", 0);
						if (pcmfp == NULL)
						{
							// Cannot open the output file, Disable further audio processing.
							MPEG2_Transport_AudioType = 0xff;
							SKIP_TRANSPORT_PACKET_BYTES(Packet_Length);
							continue;
						}
						DEMUX_MPA_AAC(pcmfp);
					}
				}
			}
			if (AudioOnly_Flag && Info_Flag && !(AudioPktCount++ % 128))
				UpdateInfo();
		}

		else if (tp.pid && tp.pid == MPEG2_Transport_AudioPID && (MPEG2_Transport_AudioType == 0x81))
		{
			// We are demuxing AC3 audio.
			// search for an MPEG-PES packet header
			if (tp.random_access_indicator || tp.payload_unit_start_indicator )
			{
				LOCATE

				code = Get_Short();
				code = (code & 0xffff)<<16 | Get_Short();
				Packet_Length = Packet_Length - 4; // remove these two bytes

				// Check for MPEG2-PES packet header. This may contains a PTS.
				if ((TransportPacketSize != 192 && code != PRIVATE_STREAM_1) ||
                    (TransportPacketSize == 192 && (code != BLURAY_STREAM_1 && code != PRIVATE_STREAM_1)))
				{
					// No, move the buffer-pointer back.
					Rdptr -= 4; 
					Packet_Length = Packet_Length + 4;
				}
				else
				{
					// YES, pull out PTS 
					//Packet_Length = Get_Short();
					Get_Short(); // MPEG2-PES total Packet_Length
					Get_Byte(); // skip a byte
	
					code = Get_Byte();
					Packet_Header_Length = Get_Byte();
					Packet_Length = Packet_Length - 5; // compensate the bytes we extracted
	
					// get PTS, and skip rest of PES-header
					if (code >= 0x80 && Packet_Header_Length > 4 )
					{
//						unsigned int dts_stamp;
						code = Get_Byte();
						PES_PTS = (code & 0x0e) << 29;
						PES_PTS |= (Get_Short() & 0xfffe) << 14;
						PES_PTS |= (Get_Short()>>1) & 0x7fff;
						AudioPTS = (unsigned int) (PES_PTS & 0xffffffff);	
						if (LogTimestamps_Flag && D2V_Flag && StartLogging_Flag)
							fprintf(Timestamps, " A00 PTS %u [%ums]\n", AudioPTS, AudioPTS/90);
						Packet_Length = Packet_Length - 5;
#if 0
						if ((code & 0xc0) == 0xc0)
						{
							PES_DTS = (Get_Byte() & 0x0e) << 29;
							PES_DTS |= (Get_Short() & 0xfffe) << 14;
							PES_DTS |= (Get_Short()>>1) & 0x7fff;
							dts_stamp = (unsigned int) (PES_DTS & 0xffffffff);
							if (LogTimestamps_Flag && D2V_Flag && StartLogging_Flag)
								fprintf(Timestamps, "A DTS %u [%ums]\n", dts_stamp, dts_stamp/90);
							Packet_Length -= 5;
							SKIP_TRANSPORT_PACKET_BYTES(Packet_Header_Length - 10)
						}
						else
#endif
							SKIP_TRANSPORT_PACKET_BYTES(Packet_Header_Length - 5)
					}
					else
						SKIP_TRANSPORT_PACKET_BYTES( Packet_Header_Length )
				}
			}

			// Done processing the MPEG-2 PES header, now for the *real* audio-data

			LOCATE
			// if this is the *first* observation, we will seek to the
			// first valid AC3-frame, then decode its header.  
			// We tried checking for tp.payload_unit_start_indicator, but this
			// indicator isn't reliable on a lot of DTV-stations!
			// Instead, we'll manually search for an AC3-sync word.
			if (!audio[0].rip && Start_Flag && !audio[0].type && 
				(tp.random_access_indicator || tp.payload_unit_start_indicator) 
				&& Packet_Length > 5 )
			{
				code = Get_Byte();
				code = (code & 0xff)<<8 | Get_Byte();
				Packet_Length = Packet_Length - 2; // remove these two bytes

				// search for an AC3-sync word.  We wouldn't have to do this if
				// DTV-stations made proper use of tp.payload_unit_start_indicator;
#if 0
				if (tp.payload_unit_start_indicator)
				{
					MessageBox(hWnd, "This is a harmless condition but I want to know if it\n"
						       "ever occurs. Please email me at neuron2@comcast.net to inform\n"
							   "me that you saw this message box. Thank you.",
						"Please tell me about this!", MB_OK | MB_ICONINFORMATION);
				}
#endif
				while (code != 0xb77 && Packet_Length > 0)
				{
					
oops2:
					code = (code & 0xff)<<8 | Get_Byte();
					--Packet_Length;
				}

				if ( code != 0xb77 )  // did we find the sync-header?
				{
					// no, we searched the *ENTIRE* transport-packet and came up empty!
					SKIP_TRANSPORT_PACKET_BYTES( Packet_Length )
					continue;  
				}

				// First time that we detected this particular channel? yes
				audio[0].type = FORMAT_AC3;

				//Packet_Length = Packet_Length - 5; // remove five bytes
				Get_Short(); 
				audio[0].rate = (Get_Byte()>>1) & 0x1f;
				if (audio[0].rate > 0x12)
					goto oops2;
				Get_Byte();
				audio[0].mode = (Get_Byte()>>5) & 0x07;
				//Packet_Length = Packet_Length + 5; // restore these five bytes
				Rdptr -= 5; // restore these five bytes

				// ok, now move "backward" by two more bytes, so we're back at the
				// start of the AC3-sync header

				Packet_Length += 2;
				Rdptr -= 2; 

				if (D2V_Flag || Decision_Flag || AudioOnly_Flag)
				{
					// For transport streams, the audio is always track 1.
					if (Decision_Flag && audio[0].selected_for_demux == true)
					{
						InitialAC3();

						DECODE_AC3

						audio[0].rip = 1;
					}
					else if (Method_Flag==AUDIO_DECODE && audio[0].selected_for_demux == true)
					{
						InitialAC3();

						sprintf(szBuffer, "%s PID %03x %sch %dKbps %s.wav", szOutput, MPEG2_Transport_AudioPID, 
							AC3ModeDash[audio[0].mode], AC3Rate[audio[0].rate], FTType[SRC_Flag]);

						strcpy(audio[0].filename, szBuffer);
						audio[0].file = OpenAudio(szBuffer, "wb", 0);
						if (audio[0].file == NULL)
						{
							// Cannot open the output file, Disable further audio processing.
							MPEG2_Transport_AudioType = 0xff;
							SKIP_TRANSPORT_PACKET_BYTES(Packet_Length);
							continue;
						}

						StartWAV(audio[0].file, 0x01);	// 48K, 16bit, 2ch

						// Adjust the VideoPTS to account for frame reordering.
						if (!PTSAdjustDone)
						{
							PTSAdjustDone = 1;
							picture_period = 1.0 / frame_rate;
							VideoPTS -= (int) (LeadingBFrames * picture_period * 90000);
						}

						if (PTSDifference(AudioPTS, VideoPTS, &PTSDiff))
							audio[0].delay = 0;
						else
							audio[0].delay = PTSDiff * 192;

						if (SRC_Flag)
						{
							DownWAV(audio[0].file);
							InitialSRC();
						}

						if (audio[0].delay > 0)
						{
							if (SRC_Flag)
								audio[0].delay = ((int)(0.91875*audio[0].delay)>>2)<<2;

							for (i=0; i<audio[0].delay; i++)
								fputc(0, audio[0].file);

							audio[0].size += audio[0].delay;
							audio[0].delay = 0;
						}

						DECODE_AC3

						if (-audio[0].delay > size)
							audio[0].delay += size;
						else
						{
							if (SRC_Flag)
								Wavefs44(audio[0].file, size+audio[0].delay, AC3Dec_Buffer-audio[0].delay);
							else
								fwrite(AC3Dec_Buffer-audio[0].delay, size+audio[0].delay, 1, audio[0].file);

							audio[0].size += size+audio[0].delay;
							audio[0].delay = 0;
						}

						audio[0].rip = 1;
					}
					else if (Method_Flag == AUDIO_DEMUXALL || (Method_Flag==AUDIO_DEMUX && audio[0].selected_for_demux == true))
					{
						if (AudioOnly_Flag)
						{
							char *p;
							strcpy(szOutput, Infilename[0]);
							p = &szOutput[sizeof(szOutput)];
							while (*p != '.') p--;
							*p = 0;
							sprintf(szBuffer, "%s PID %03x %sch %dKbps.ac3", szOutput, MPEG2_Transport_AudioPID, 
								AC3ModeDash[audio[0].mode], AC3Rate[audio[0].rate]);
						}
						else
						{
							// Adjust the VideoPTS to account for frame reordering.
							if (!PTSAdjustDone)
							{
								PTSAdjustDone = 1;
								picture_period = 1.0 / frame_rate;
								VideoPTS -= (int) (LeadingBFrames * picture_period * 90000);
							}

							if (PTSDifference(AudioPTS, VideoPTS, &PTSDiff))
								sprintf(szBuffer, "%s PID %03x %sch %dKbps.ac3", szOutput, MPEG2_Transport_AudioPID,
									AC3ModeDash[audio[0].mode], AC3Rate[audio[0].rate]);
							else
								sprintf(szBuffer, "%s PID %03x %sch %dKbps DELAY %dms.ac3", szOutput, MPEG2_Transport_AudioPID, 
									AC3ModeDash[audio[0].mode], AC3Rate[audio[0].rate], PTSDiff);
						}

						audio[0].file = OpenAudio(szBuffer, "wb", 0);
						if (audio[0].file == NULL)
						{
							// Cannot open the output file, Disable further audio processing.
							MPEG2_Transport_AudioType = 0xff;
							SKIP_TRANSPORT_PACKET_BYTES(Packet_Length);
							continue;
						}

						DEMUX_AC3

						audio[0].rip = 1;
					}
				}
			}
			else if (audio[0].rip)
			{
// NOT WORKING YET -- DO NOT ENABLE THIS CODE!
//#define DETECT_AC3_MODE_CHANGE
#ifdef DETECT_AC3_MODE_CHANGE
				if (tp.payload_unit_start_indicator && Rdptr < Rdbfr + BUFFER_SIZE - 20)
				{
					unsigned char *ptr = Rdptr;
					code = Get_Byte();
					code = (code & 0xff)<<8 | Get_Byte();
					if (code == 0xb77)  // did we find the sync-header?
					{
						unsigned char tmp1, tmp2;
						Get_Short(); 
						tmp1 = (Get_Byte()>>1) & 0x1f;
						Get_Byte();
						tmp2 = (Get_Byte()>>5) & 0x07;
						if (audio[0].rate != tmp1 || audio[0].mode != tmp2)
						{
							sprintf(szBuffer,
									"Audio format changed from AC3 %s %d to AC3 %s %d.\n"
									"You should revise your project range so that it does\n"
									"not include an audio format change.",
									AC3Mode[ac3[0].mode], AC3Rate[ac3[0].rate], AC3Mode[tmp2], AC3Rate[tmp1]);
							MessageBox(hWnd, szBuffer, "Warning: Audio format changed", MB_OK | MB_ICONWARNING);
						}
						audio[0].rate = tmp1;
						audio[0].mode = tmp2;
						Rdptr -= 5; // restore 5 bytes
					}
					Rdptr -= 2; // restore 2 bytes
#if 0
					if (Rdptr != ptr)
					{
						_lseek(Infile[CurrentFile], -BUFFER_SIZE, SEEK_CUR);
						_donread(Infile[CurrentFile], Rdbfr, BUFFER_SIZE);
						Rdptr = ptr;
					}
#endif
				}
#endif
				if (Decision_Flag)
					DECODE_AC3
				else if (Method_Flag==AUDIO_DECODE)
				{
					DECODE_AC3

					if (-audio[0].delay > size)
						audio[0].delay += size;
					else
					{
						if (SRC_Flag)
							Wavefs44(audio[0].file, size+audio[0].delay, AC3Dec_Buffer-audio[0].delay);
						else
							fwrite(AC3Dec_Buffer-audio[0].delay, size+audio[0].delay, 1, audio[0].file);

						audio[0].size += size+audio[0].delay;
						audio[0].delay = 0;
					}
				}
				else
				{
					DEMUX_AC3
				}
			}
			if (AudioOnly_Flag && Info_Flag && !(AudioPktCount++ % 128))
				UpdateInfo();
		}
		else if ((Method_Flag == AUDIO_DEMUXALL || Method_Flag == AUDIO_DEMUX) && Start_Flag &&
				 tp.pid && (tp.pid == MPEG2_Transport_AudioPID) &&
				 (MPEG2_Transport_AudioType == 0xfe || MPEG2_Transport_AudioType == 0xffffffff)) 
		{
			// We are demuxing DTS audio.
			if (audio[0].file)
			{
				if (tp.payload_unit_start_indicator)
				{
					Get_Short(); // start code and stream id
					Get_Short();
					Get_Short(); // packet length
					Get_Byte(); // flags
					code = Get_Byte(); // more flags
					Packet_Header_Length = Get_Byte();
					Packet_Length -= 9;
					if (code & 0x80)
					{
						code = Get_Byte();
						PES_PTS = (code & 0x0e) << 29;
						PES_PTS |= (Get_Short() & 0xfffe) << 14;
						PES_PTS |= (Get_Short()>>1) & 0x7fff;
						AudioPTS = (unsigned int) (PES_PTS & 0xffffffff);
						if (LogTimestamps_Flag && D2V_Flag && StartLogging_Flag)
							fprintf(Timestamps, " A00 PTS %u [%ums]\n", AudioPTS, AudioPTS/90);
						Packet_Length -= 5;
						SKIP_TRANSPORT_PACKET_BYTES(Packet_Header_Length-5)
					}
				}
				DEMUX_DTS;
			}
			else if (tp.payload_unit_start_indicator)
			{
				Get_Short(); // start code
				Get_Short(); // rest of start code and stream id
				Get_Short(); // packet length
				Get_Byte(); // flags
				code = Get_Byte(); // more flags
				Packet_Header_Length = Get_Byte();
				Packet_Length -= 9;
				if (code & 0x80)
				{
					code = Get_Byte();
					PES_PTS = (code & 0x0e) << 29;
					PES_PTS |= (Get_Short() & 0xfffe) << 14;
					PES_PTS |= (Get_Short()>>1) & 0x7fff;
					AudioPTS = (unsigned int) (PES_PTS & 0xffffffff);
					if (LogTimestamps_Flag && D2V_Flag && StartLogging_Flag)
						fprintf(Timestamps, " A00 PTS %u [%ums]\n", AudioPTS, AudioPTS/90);
					Packet_Length = Packet_Length - 5;
					SKIP_TRANSPORT_PACKET_BYTES(Packet_Header_Length-5)

					// Now we're at the start of the audio.
					audio[0].type = FORMAT_DTS;

					if (AudioOnly_Flag)
					{
						char *p;
						strcpy(szOutput, Infilename[0]);
						p = &szOutput[sizeof(szOutput)];
						while (*p != '.') p--;
						*p = 0;
						sprintf(szBuffer, "%s PID %03x.dts", szOutput, MPEG2_Transport_AudioPID);
					}
					else
					{
						// Adjust the VideoPTS to account for frame reordering.
						if (!PTSAdjustDone)
						{
							PTSAdjustDone = 1;
							picture_period = 1.0 / frame_rate;
							VideoPTS -= (int) (LeadingBFrames * picture_period * 90000);
						}

						if (PTSDifference(AudioPTS, VideoPTS, &PTSDiff))
							sprintf(szBuffer, "%s PID %03x.dts", szOutput, MPEG2_Transport_AudioPID);
						else
							sprintf(szBuffer, "%s PID %03x DELAY %dms.dts", szOutput, MPEG2_Transport_AudioPID, PTSDiff);
					}
					if (D2V_Flag || AudioOnly_Flag)
					{
						audio[0].file = OpenAudio(szBuffer, "wb", 0);
						if (audio[0].file == NULL)
						{
							// Cannot open the output file, Disable further audio processing.
							MPEG2_Transport_AudioType = 0xff;
							SKIP_TRANSPORT_PACKET_BYTES(Packet_Length);
							continue;
						}
						DEMUX_DTS;
					}
				}
			}
			if (AudioOnly_Flag && Info_Flag && !(AudioPktCount++ % 128))
				UpdateInfo();
		}
		// fallthrough case
		// skip remaining bytes in current packet
		SKIP_TRANSPORT_PACKET_BYTES( Packet_Length )
	}
}

// PVA packet data structure.
typedef struct
{			
	unsigned short sync_byte;
	unsigned char stream_id;
	unsigned char counter;
	unsigned char reserved;
	unsigned char flags;
	unsigned short length;
} pva_packet;

// PVA transport stream parser.
void Next_PVA_Packet()
{
	unsigned int Packet_Length;
	unsigned int time, start;
	pva_packet pva;
	unsigned int code, PTS, PES_PTS, Packet_Header_Length;
	int PTSDiff;
	double picture_period;

	start = timeGetTime();
	for (;;)
	{
		// Don't loop forever. If we don't get data
		// in a reasonable time (1 secs) we exit.
		time = timeGetTime();
		if (time - start > 2000)
		{
			MessageBox(hWnd, "Cannot find video data.", NULL, MB_OK | MB_ICONERROR);
			ThreadKill(MISC_KILL);
		}

		// Search for a good sync.
		for(;;)
		{
			if (Stop_Flag)
				ThreadKill(MISC_KILL);
			// Sync word is 0x4156.
			if (Get_Byte() != 0x41) continue;
			if (Get_Byte() != 0x56)
			{
				// This byte might be a 0x41, so back up by one.
				Rdptr--;
				continue;
			}
			// To protect against emulation of the sync word,
			// also check that the stream says audio or video.
			pva.stream_id = Get_Byte();
			if (pva.stream_id != 0x01 && pva.stream_id != 0x02)
			{
				// This byte might be a 0x41, so back up by one.
				Rdptr--;
				continue;
			}
			break;
		}

		// Record the location of the start of the packet. This will be used
		// for indexing when an I frame is detected.
		if (D2V_Flag)
		{
			PackHeaderPosition = _telli64(Infile[CurrentFile])
								 - (__int64)BUFFER_SIZE + (__int64)Rdptr - (__int64)Rdbfr - 3;
		}

		// Pick up the remaining packet header fields.
		pva.counter = Get_Byte();
		pva.reserved = Get_Byte();
		pva.flags = Get_Byte();
		pva.length = Get_Byte() << 8;
		pva.length |= Get_Byte();
		Packet_Length = pva.length;
		Rdmax = Rdptr + Packet_Length;

		// Any payload?
		if (Packet_Length == 0 || pva.reserved != 0x55)
			continue;  // No, try the next packet.

		// Check stream id for video.
		if (pva.stream_id == 1) 
		{
			// This is a video packet.
			// Extract the PTS if it exists.
			if (pva.flags & 0x10)
			{
				PTS = (int) ((Get_Byte() << 24) | (Get_Byte() << 16) | (Get_Byte() << 8) | Get_Byte());
				if (LogTimestamps_Flag && D2V_Flag && StartLogging_Flag)
					fprintf(Timestamps, "V PTS %u [%ums]\n", PTS, PTS/90);
				Packet_Length -= 4;
				if (pva.flags & 0x03)
				{
					// Suck up pre-bytes if any.
					int i;
					for (i = 0; i < (pva.flags & 0x03); i ++)
						Get_Byte();
					Packet_Length -= i;
				}
				if (!Start_Flag)
				{
					VideoPTS = PTS;
				}
			}

			// Deliver the video to the ES parsing layer. 
			Bitrate_Monitor += (Rdmax - Rdptr);
            if (AudioOnly_Flag)
				SKIP_TRANSPORT_PACKET_BYTES(Packet_Length);
			return;
		}
		
		// Check stream id for audio.
		else if ((Method_Flag == AUDIO_DEMUXALL || Method_Flag == AUDIO_DEMUX) &&
				 Start_Flag &&
				 pva.stream_id == 2)
		{
			// This is an audio packet.
			if (mpafp)
			{
				// For audio, this flag bit means an embedded audio PES packet starts
				// immediately in this PVA packet.
				if (pva.flags & 0x10)
				{
					Get_Short(); // start code and stream id
					Get_Short();
					Get_Short(); // packet length
					Get_Byte(); // flags
					code = Get_Byte(); // more flags
					Packet_Header_Length = Get_Byte();
					Packet_Length -= 9;
					if (code & 0x80)
					{
						code = Get_Byte();
						PES_PTS = (code & 0x0e) << 29;
						PES_PTS |= (Get_Short() & 0xfffe) << 14;
						PES_PTS |= (Get_Short()>>1) & 0x7fff;
						AudioPTS = (unsigned int) (PES_PTS & 0xffffffff);
						if (LogTimestamps_Flag && D2V_Flag && StartLogging_Flag)
							fprintf(Timestamps, " A00 PTS %u [%ums]\n", AudioPTS, AudioPTS/90);
						Packet_Length = Packet_Length - 5;
						SKIP_TRANSPORT_PACKET_BYTES(Packet_Header_Length-5)
					}
				}
				DEMUX_MPA_AAC(mpafp);
			}
			else if (pva.flags & 0x10)
			{
				Get_Short(); // start code
				Get_Short(); // rest of start code and stream id
				Get_Short(); // packet length
				Get_Byte(); // flags
				code = Get_Byte(); // more flags
				Packet_Header_Length = Get_Byte();
				Packet_Length -= 9;
				if (code & 0x80)
				{
					code = Get_Byte();
					PES_PTS = (code & 0x0e) << 29;
					PES_PTS |= (Get_Short() & 0xfffe) << 14;
					PES_PTS |= (Get_Short()>>1) & 0x7fff;
					AudioPTS = (unsigned int) (PES_PTS & 0xffffffff);
					if (LogTimestamps_Flag && D2V_Flag && StartLogging_Flag)
						fprintf(Timestamps, " A00 PTS %u [%ums]\n", AudioPTS, AudioPTS/90);
					Packet_Length = Packet_Length - 5;
					SKIP_TRANSPORT_PACKET_BYTES(Packet_Header_Length-5)
					// Now we're at the start of the audio.
					// Find the audio header.
					code = Get_Byte();
					code = ((code & 0xff) << 8) | Get_Byte();
					Packet_Length -= 2;
					while ((code & 0xfff8) != 0xfff8 && Packet_Length > 0)
					{
emulated0:
						code = ((code & 0xff) << 8) | Get_Byte();
						Packet_Length--;
					}
					if ((code & 0xfff8) != 0xfff8)
					{
						SKIP_TRANSPORT_PACKET_BYTES(Packet_Length);
						continue;
					}
					// Found the audio header. Now check the layer field.
					// For MPEG, layer is 1, 2, or 3. For AAC, it is 0.
					// We demux the same for both; only the file name we create is different.
					if (((code & 6) >> 1) == 0x00)
					{
						// AAC audio.
						audio[0].type = FORMAT_AAC;
					}
					else
					{
                        // Try to detect emulated sync words by enforcing semantics.
                        if (check_audio_syncword(0, (code >> 1) & 3,  (Rdptr[0] >> 4) & 0xf, (Rdptr[0] >> 2) & 3, (Rdptr[1] >> 6) & 3, Rdptr[1] & 3))
                        {
                                goto emulated0;
                        }

						// MPEG audio.
						audio[0].type = FORMAT_MPA;
					}

					if (AudioOnly_Flag)
					{
						char *p;
						strcpy(szOutput, Infilename[0]);
						p = &szOutput[sizeof(szOutput)];
						while (*p != '.') p--;
						*p = 0;
						if (audio[0].type == FORMAT_AAC)
							sprintf(szBuffer, "%s.aac", szOutput);
						else
							sprintf(szBuffer, "%s %s %s %s %s.%s", szOutput,
									MPALayer[audio[0].layer], MPAMode[audio[0].mode], MPASample[audio[0].sample],
									MPARate[audio[0].layer][audio[0].rate],
                                    UseMPAExtensions ? MPAExtension[0] : MPAExtension[audio[0].layer]);
					}
					else
					{
						// Adjust the VideoPTS to account for frame reordering. && StartTemporalReference != -1 && StartTemporalReference < 18)
						{
							PTSAdjustDone = 1;
							picture_period = 1.0 / frame_rate;
							VideoPTS -= (int) (LeadingBFrames * picture_period * 90000);
						}

						if (PTSDifference(AudioPTS, VideoPTS, &PTSDiff))
						{
							if (audio[0].type == FORMAT_AAC)
								sprintf(szBuffer, "%s.aac", szOutput);
							else
								sprintf(szBuffer, "%s %s %s %s %s.%s", szOutput,
										MPALayer[audio[0].layer], MPAMode[audio[0].mode], MPASample[audio[0].sample],
										MPARate[audio[0].layer][audio[0].rate],
                                        UseMPAExtensions ? MPAExtension[0] : MPAExtension[audio[0].layer]);
						}
						else
						{
							if (audio[0].type == FORMAT_AAC)
								sprintf(szBuffer, "%s DELAY %dms.aac", szOutput, PTSDiff);
							else
								sprintf(szBuffer, "%s %s %s %s %s DELAY %dms.%s", szOutput,
										MPALayer[audio[0].layer], MPAMode[audio[0].mode], MPASample[audio[0].sample],
										MPARate[audio[0].layer][audio[0].rate], PTSDiff,
                                        UseMPAExtensions ? MPAExtension[0] : MPAExtension[audio[0].layer]);
						}
					}

					// Unread the audio header bytes.
					Packet_Length += 2;
					Rdptr -= 2;
					if (D2V_Flag || AudioOnly_Flag)
					{
						mpafp = OpenAudio(szBuffer, "wb", 0);
						if (mpafp == NULL)
						{
							// Cannot open the output file.
							SKIP_TRANSPORT_PACKET_BYTES(Packet_Length);
							continue;
						}
						DEMUX_MPA_AAC(mpafp);
					}
				}
			}
			if (AudioOnly_Flag && Info_Flag && !(AudioPktCount++ % 128))
				UpdateInfo();
		}

		// Not an video packet or an audio packet to be demultiplexed. Keep looking.
		SKIP_TRANSPORT_PACKET_BYTES(Packet_Length);
	}
}

// MPEG2 program stream parser.
void Next_Packet()
{
	static int i, Packet_Length, Packet_Header_Length, size;
	static unsigned int code, AUDIO_ID, VOBCELL_Count;
	static int stream_type;
	int PTSDiff;
	double picture_period;
	unsigned int dts_stamp = 0;
	__int64 tmp;
	__int64 SCRbase, SCRext, SCR;
	char buf[64];

	if (SystemStream_Flag == TRANSPORT_STREAM)
	{
		Next_Transport_Packet();
		return;
	}
	else if (SystemStream_Flag == PVA_STREAM)
	{
		Next_PVA_Packet();
		return;
	}

	for (;;)
	{
		code = Get_Short();
		code = (code << 16) | Get_Short();

		while ((code & 0xffffff00) != 0x00000100)
		{
			if (Stop_Flag)
				return;
			code = (code<<8) + Get_Byte();
		}

		switch (code)
		{
			case PACK_START_CODE:
				if (D2V_Flag)
				{
					PackHeaderPosition = _telli64(Infile[CurrentFile]);
					PackHeaderPosition = PackHeaderPosition - (__int64)BUFFER_SIZE + (__int64)Rdptr - 4 - (__int64)Rdbfr;
				}
				if (((tmp = Get_Byte()) & 0xf0) == 0x20)
				{
					// MPEG1 program stream
					stream_type = MPEG1_PROGRAM_STREAM;
					if (LogTimestamps_Flag && D2V_Flag && StartLogging_Flag)
					{
						SCR = (tmp & 0x0e) << 29;
						tmp = (__int64) Get_Byte();
						SCR |= tmp << 22;
						tmp = (__int64) Get_Byte();
						SCR |= (tmp & 0xfe) << 14;
						tmp = (__int64) Get_Byte();
						SCR |= tmp << 7;
						tmp = (__int64) Get_Byte();
						SCR |= tmp >> 1;
						tmp = (__int64) Get_Byte();
						tmp = (__int64) Get_Byte();
						tmp = (__int64) Get_Byte();
						SCR = 300 * SCR;
						_i64toa(SCR, buf, 10);
						fprintf(Timestamps, "SCR %s, ", buf);
						_i64toa(SCR/27000, buf, 10);
						fprintf(Timestamps, "[%sms]\n", buf);
					}
					else
						Rdptr += 7;
				}
				else
				{
					// MPEG2 program stream
					stream_type = MPEG2_PROGRAM_STREAM;
					if (LogTimestamps_Flag && D2V_Flag && StartLogging_Flag)
					{
						SCRbase = (tmp & 0x38) << 27;
						SCRbase |= (tmp & 0x03) << 28;
						tmp = (__int64) Get_Byte();
						SCRbase |= tmp << 20;
						tmp = (__int64) Get_Byte();
						SCRbase |= (tmp & 0xf8) << 12;
						SCRbase |= (tmp & 0x03) << 13;
						tmp = (__int64) Get_Byte();
						SCRbase |= tmp << 5;
						tmp = (__int64) Get_Byte();
						SCRbase |= tmp >> 3;
						SCRext = (tmp & 0x03) << 7;
						tmp = (__int64) Get_Byte();
						SCRext |= tmp >> 1;
						tmp = (__int64) Get_Byte();
						tmp = (__int64) Get_Byte();
						tmp = (__int64) Get_Byte();
						SCR = 300 * SCRbase + SCRext;
						_i64toa(SCR, buf, 10);
						fprintf(Timestamps, "SCR %s, ", buf);
						_i64toa(SCR/27000, buf, 10);
						fprintf(Timestamps, "[%sms]\n", buf);
					}
					else
						Rdptr += 8;
				}
				VOBCELL_Count = 0;
				break;

			case PRIVATE_STREAM_2:
				Packet_Length = Get_Short();

				if (++VOBCELL_Count==2)
				{
					Rdptr += 25;
					VOB_ID = Get_Short();
					Get_Byte();
					CELL_ID = Get_Byte();
					Rdptr += Packet_Length - 29;

					sprintf(szBuffer, "%d", VOB_ID);
					SetDlgItemText(hDlg, IDC_VOB_ID, szBuffer);

					sprintf(szBuffer, "%d", CELL_ID);
					SetDlgItemText(hDlg, IDC_CELL_ID, szBuffer);
				}
				else
					Rdptr += Packet_Length;
				break;

			case PRIVATE_STREAM_1:
				Packet_Length = Get_Short();

				Rdptr ++;	// +1
				code = Get_Byte();	// +1
				Packet_Header_Length = Get_Byte();	// +1

				if (code>=0x80)
				{
					__int64 PES_PTS;

					PES_PTS = (Get_Byte() & 0x0e) << 29;
					PES_PTS |= (Get_Short() & 0xfffe) << 14;
					PES_PTS |= (Get_Short()>>1) & 0x7fff;
					AudioPTS = (unsigned int) (PES_PTS & 0xffffffff);
                    HadAudioPTS = true;
					Rdptr += Packet_Header_Length - 5;
				}
				else
					Rdptr += Packet_Header_Length;

				// Private stream 1 on a DVD has an audio substream number,
				// but straight MPEG may not, in which case the audio sync word
				// will appear where the substream number would be. We will have
				// to parse differently for these two structures. For now, it's
				// controlled by a GUI option.
				if (FusionAudio)
				{
					// Determine the audio type from the audio sync word.
					if (Rdptr[0] == 0x0b && Rdptr[1] == 0x77)
						AUDIO_ID = SUB_AC3;
					else if (Rdptr[0] == 0x7f && Rdptr[1] == 0xfe)
						AUDIO_ID = SUB_DTS;
					else
						// Nothing else is supported. Force it to fail.
						AUDIO_ID = 0;
					Packet_Length -= Packet_Header_Length + 3;
				}
				else
				{
					AUDIO_ID = Get_Byte();	// +1
					Packet_Length -= Packet_Header_Length + 4;
				}

				if (AUDIO_ID>=SUB_AC3 && AUDIO_ID<SUB_AC3+CHANNEL)
				{
					if (LogTimestamps_Flag && D2V_Flag && code >= 0x80 && StartLogging_Flag)
						fprintf(Timestamps, " A%2x PTS %u [%ums]\n", AUDIO_ID, AudioPTS, AudioPTS/90);
					if (!FusionAudio)
					{
						Rdptr += 3; Packet_Length -= 3;
					}

					LOCATE

					if (!audio[AUDIO_ID].rip && Start_Flag && !audio[AUDIO_ID].type && HadAudioPTS == true)
					{
						audio[AUDIO_ID].type = FORMAT_AC3;

						code = Get_Byte();
						code = (code & 0xff)<<8 | Get_Byte();
						i = 0;

						while (code!=0xb77)
						{
oops:
							code = (code & 0xff)<<8 | Get_Byte();
							i++;
						}

						Get_Short();
						audio[AUDIO_ID].rate = (Get_Byte()>>1) & 0x1f;
						if (audio[AUDIO_ID].rate > 0x12)
							goto oops;
						Get_Byte();
						audio[AUDIO_ID].mode = (Get_Byte()>>5) & 0x07;

						Rdptr -= 7; Packet_Length -= i;

						if (D2V_Flag || Decision_Flag || AudioOnly_Flag)
						{
							if (Decision_Flag && audio[AUDIO_ID].selected_for_demux == true)
							{
								InitialAC3();

								DECODE_AC3

								audio[AUDIO_ID].rip = true;
							}
							else if (Method_Flag==AUDIO_DECODE && audio[AUDIO_ID].selected_for_demux == true)
							{
								InitialAC3();

								sprintf(szBuffer, "%s T%x %sch %dKbps %s.wav", szOutput, AUDIO_ID, 
									AC3ModeDash[audio[AUDIO_ID].mode], AC3Rate[audio[AUDIO_ID].rate], FTType[SRC_Flag]);

								strcpy(audio[AUDIO_ID].filename, szBuffer);
								audio[AUDIO_ID].file = OpenAudio(szBuffer, "wb", AUDIO_ID);
								if (audio[AUDIO_ID].file == NULL)
								{
									MessageBox(hWnd, "Cannot open file for audio demux output.\nAborting...", NULL, MB_OK | MB_ICONERROR);
									ThreadKill(MISC_KILL);
								}

								StartWAV(audio[AUDIO_ID].file, 0x01);	// 48K, 16bit, 2ch

								// Adjust the VideoPTS to account for frame reordering.
								if (!PTSAdjustDone)
								{
									PTSAdjustDone = 1;
									picture_period = 1.0 / frame_rate;
									VideoPTS -= (int) (LeadingBFrames * picture_period * 90000);
								}

								if (PTSDifference(AudioPTS, VideoPTS, &PTSDiff))
									audio[AUDIO_ID].delay = 0;
								else
									audio[AUDIO_ID].delay = PTSDiff * 192;

								if (SRC_Flag)
								{
									DownWAV(audio[AUDIO_ID].file);
									InitialSRC();
								}

								if (audio[AUDIO_ID].delay > 0)
								{
									if (SRC_Flag)
										audio[AUDIO_ID].delay = ((int)(0.91875*audio[AUDIO_ID].delay)>>2)<<2;

									for (i=0; i<audio[AUDIO_ID].delay; i++)
										fputc(0, audio[AUDIO_ID].file);

									audio[AUDIO_ID].size += audio[AUDIO_ID].delay;
									audio[AUDIO_ID].delay = 0;
								}

								DECODE_AC3

								if (-audio[AUDIO_ID].delay > size)
									audio[AUDIO_ID].delay += size;
								else
								{
									if (SRC_Flag)
										Wavefs44(audio[AUDIO_ID].file, size+audio[AUDIO_ID].delay, AC3Dec_Buffer-audio[AUDIO_ID].delay);
									else
										fwrite(AC3Dec_Buffer-audio[AUDIO_ID].delay, size+audio[AUDIO_ID].delay, 1, audio[AUDIO_ID].file);

									audio[AUDIO_ID].size += size+audio[AUDIO_ID].delay;
 									audio[AUDIO_ID].delay = 0;
								}

								audio[AUDIO_ID].rip = true;
							}
							else if (Method_Flag==AUDIO_DEMUXALL || (Method_Flag==AUDIO_DEMUX && audio[AUDIO_ID].selected_for_demux == true))
							{
								if (AudioOnly_Flag)
								{
									char *p;
									strcpy(szOutput, Infilename[0]);
									p = &szOutput[sizeof(szOutput)];
									while (*p != '.') p--;
									*p = 0;
									sprintf(szBuffer, "%s T%x %sch %dKbps.ac3", szOutput, AUDIO_ID, 
										AC3ModeDash[audio[AUDIO_ID].mode], AC3Rate[audio[AUDIO_ID].rate]);
								}
								else
								{
									// Adjust the VideoPTS to account for frame reordering.
									if (!PTSAdjustDone)
									{
										PTSAdjustDone = 1;
										picture_period = 1.0 / frame_rate;
										VideoPTS -= (int) (LeadingBFrames * picture_period * 90000);
									}

									if (PTSDifference(AudioPTS, VideoPTS, &PTSDiff))
										sprintf(szBuffer, "%s T%x %sch %dKbps.ac3", szOutput, AUDIO_ID, 
											AC3ModeDash[audio[AUDIO_ID].mode], AC3Rate[audio[AUDIO_ID].rate]);
									else
										sprintf(szBuffer, "%s T%x %sch %dKbps DELAY %dms.ac3", szOutput, AUDIO_ID, 
											AC3ModeDash[audio[AUDIO_ID].mode], AC3Rate[audio[AUDIO_ID].rate], PTSDiff);

	//								dprintf("DGIndex: Using Video PTS = %d, Audio PTS = %d [%d], reference = %d, rate = %f\n",
	//										VideoPTS/90, AudioPTS/90, PTSDiff, StartTemporalReference, frame_rate);
								}
								audio[AUDIO_ID].file = OpenAudio(szBuffer, "wb", AUDIO_ID);
								if (audio[AUDIO_ID].file == NULL)
								{
									MessageBox(hWnd, "Cannot open file for audio demux output.\nAborting...", NULL, MB_OK | MB_ICONERROR);
									ThreadKill(MISC_KILL);
								}

								DEMUX_AC3

								audio[AUDIO_ID].rip = true;
							}
						}
					}
					else if (audio[AUDIO_ID].rip)
					{
						if (Decision_Flag)
							DECODE_AC3
						else if (Method_Flag==AUDIO_DECODE)
						{
							DECODE_AC3

							if (-audio[AUDIO_ID].delay > size)
								audio[AUDIO_ID].delay += size;
							else
							{
								if (SRC_Flag)
									Wavefs44(audio[AUDIO_ID].file, size+audio[AUDIO_ID].delay, AC3Dec_Buffer-audio[AUDIO_ID].delay);
								else
									fwrite(AC3Dec_Buffer-audio[AUDIO_ID].delay, size+audio[AUDIO_ID].delay, 1, audio[AUDIO_ID].file);

								audio[AUDIO_ID].size += size+audio[AUDIO_ID].delay;
 								audio[AUDIO_ID].delay = 0;
							}
						}
						else
						{
							DEMUX_AC3
						}
					}
				}
				else if (AUDIO_ID>=SUB_PCM && AUDIO_ID<SUB_PCM+CHANNEL)
				{
					if (LogTimestamps_Flag && D2V_Flag && code >= 0x80 && StartLogging_Flag)
						fprintf(Timestamps, " A%2x PTS %u [%ums]\n", AUDIO_ID, AudioPTS, AudioPTS/90);
					Rdptr += 6; Packet_Length -= 6;

					LOCATE

					if (!audio[AUDIO_ID].rip && Start_Flag && !audio[AUDIO_ID].type)
					{
						audio[AUDIO_ID].type = FORMAT_LPCM;

						// Pick up the audio format byte.
						audio[AUDIO_ID].format = Rdptr[-2];

						if (D2V_Flag || AudioOnly_Flag)
						{
							if (Method_Flag==AUDIO_DEMUXALL || (Method_Flag == AUDIO_DEMUX && audio[AUDIO_ID].selected_for_demux == true))
							{
								if (AudioOnly_Flag)
								{
									char *p;
									strcpy(szOutput, Infilename[0]);
									p = &szOutput[sizeof(szOutput)];
									while (*p != '.') p--;
									*p = 0;
									sprintf(szBuffer, "%s T%x %s %s %dch.wav",
										szOutput,
										AUDIO_ID,
										(audio[AUDIO_ID].format & 0x30) == 0 ? "48K" : "96K",
										(audio[AUDIO_ID].format & 0xc0) == 0 ? "16bit" : ((audio[AUDIO_ID].format & 0xc0) == 0x40 ? "20bit" : "24bit"),
										(audio[AUDIO_ID].format & 0x07) + 1);
									strcpy(audio[AUDIO_ID].filename, szBuffer);
								}
								else
								{
									// Adjust the VideoPTS to account for frame reordering.
									if (!PTSAdjustDone)
									{
										PTSAdjustDone = 1;
										picture_period = 1.0 / frame_rate;
										VideoPTS -= (int) (LeadingBFrames * picture_period * 90000);
									}

									if (PTSDifference(AudioPTS, VideoPTS, &PTSDiff))
										audio[AUDIO_ID].delay = 0;
									else
										audio[AUDIO_ID].delay = PTSDiff * 192;


									sprintf(szBuffer, "%s T%x %s %s %dch.wav",
										szOutput,
										AUDIO_ID,
										(audio[AUDIO_ID].format & 0x30) == 0 ? "48K" : "96K",
										(audio[AUDIO_ID].format & 0xc0) == 0 ? "16bit" : ((audio[AUDIO_ID].format & 0xc0) == 0x40 ? "20bit" : "24bit"),
										(audio[AUDIO_ID].format & 0x07) + 1);
									strcpy(audio[AUDIO_ID].filename, szBuffer);
								}

								audio[AUDIO_ID].file = OpenAudio(szBuffer, "wb", AUDIO_ID);
								if (audio[AUDIO_ID].file == NULL)
								{
									MessageBox(hWnd, "Cannot open file for audio demux output.\nAborting...", NULL, MB_OK | MB_ICONERROR);
									ThreadKill(MISC_KILL);
								}
								StartWAV(audio[AUDIO_ID].file, audio[AUDIO_ID].format);

								if (audio[AUDIO_ID].delay > 0)
								{
									for (i=0; i<audio[AUDIO_ID].delay; i++)
										fputc(0, audio[AUDIO_ID].file);

									audio[AUDIO_ID].size += audio[AUDIO_ID].delay;
									audio[AUDIO_ID].delay = 0;
								}

								if (-audio[AUDIO_ID].delay > Packet_Length)
									audio[AUDIO_ID].delay += Packet_Length;
								else
								{
									DemuxLPCM(&size, &Packet_Length, PCM_Buffer, audio[AUDIO_ID].format);
									fwrite(PCM_Buffer-audio[AUDIO_ID].delay, size+audio[AUDIO_ID].delay, 1, audio[AUDIO_ID].file);
	
									audio[AUDIO_ID].size += size+audio[AUDIO_ID].delay;
									audio[AUDIO_ID].delay = 0;
								}

								audio[AUDIO_ID].rip = true;
							}
						}
					}
					else if (audio[AUDIO_ID].rip)
					{
						if (-audio[AUDIO_ID].delay > Packet_Length)
							audio[AUDIO_ID].delay += Packet_Length;
						else
						{
							DemuxLPCM(&size, &Packet_Length, PCM_Buffer, audio[AUDIO_ID].format);
							fwrite(PCM_Buffer-audio[AUDIO_ID].delay, size+audio[AUDIO_ID].delay, 1, audio[AUDIO_ID].file);

							audio[AUDIO_ID].size += size+audio[AUDIO_ID].delay;
							audio[AUDIO_ID].delay = 0;
						}
					}
				}
				else if (AUDIO_ID>=SUB_DTS && AUDIO_ID<SUB_DTS+CHANNEL)
				{
					if (LogTimestamps_Flag && D2V_Flag && code >= 0x80 && StartLogging_Flag)
						fprintf(Timestamps, " A%2x PTS %u [%ums]\n", AUDIO_ID, AudioPTS, AudioPTS/90);
					if (!FusionAudio)
					{
						Rdptr += 3; Packet_Length -= 3;
					}

					LOCATE

					if (!audio[AUDIO_ID].rip && Start_Flag && !audio[AUDIO_ID].type)
					{
						audio[AUDIO_ID].type = FORMAT_DTS;

						if (D2V_Flag || AudioOnly_Flag)
						{
							if (Method_Flag==AUDIO_DEMUXALL || (Method_Flag==AUDIO_DEMUX && audio[AUDIO_ID].selected_for_demux == true))
							{
								if (AudioOnly_Flag)
								{
									char *p;
									strcpy(szOutput, Infilename[0]);
									p = &szOutput[sizeof(szOutput)];
									while (*p != '.') p--;
									*p = 0;
									sprintf(szBuffer, "%s T%x.dts", szOutput, AUDIO_ID);
								}
								else
								{
									// Adjust the VideoPTS to account for frame reordering.
									if (!PTSAdjustDone)
									{
										PTSAdjustDone = 1;
										picture_period = 1.0 / frame_rate;
										VideoPTS -= (int) (LeadingBFrames * picture_period * 90000);
									}

									if (PTSDifference(AudioPTS, VideoPTS, &PTSDiff))
										sprintf(szBuffer, "%s T%x.dts", szOutput, AUDIO_ID);
									else
										sprintf(szBuffer, "%s T%x DELAY %dms.dts", szOutput, AUDIO_ID, PTSDiff);
								}

								audio[AUDIO_ID].file = OpenAudio(szBuffer, "wb", AUDIO_ID);
								if (audio[AUDIO_ID].file == NULL)
								{
									MessageBox(hWnd, "Cannot open file for audio demux output.\nAborting...", NULL, MB_OK | MB_ICONERROR);
									ThreadKill(MISC_KILL);
								}

								DEMUX_DTS

								audio[AUDIO_ID].rip = true;
							}
						}
					}
					else if (audio[AUDIO_ID].rip)
						DEMUX_DTS
				}
				Rdptr += Packet_Length;
				if (AudioOnly_Flag && Info_Flag && !(AudioPktCount++ % 128))
					UpdateInfo();
				break;

			case AUDIO_ELEMENTARY_STREAM_7:
			case AUDIO_ELEMENTARY_STREAM_6:
			case AUDIO_ELEMENTARY_STREAM_5:
			case AUDIO_ELEMENTARY_STREAM_4:
			case AUDIO_ELEMENTARY_STREAM_3:
			case AUDIO_ELEMENTARY_STREAM_2:
			case AUDIO_ELEMENTARY_STREAM_1:
			case AUDIO_ELEMENTARY_STREAM_0:
                AUDIO_ID = code & 0xff;
				if (stream_type == MPEG1_PROGRAM_STREAM)
				{
					// MPEG1 program stream.
					Packet_Length = Get_Short();

					Packet_Header_Length = 0;
					// Stuffing bytes.
					do 
					{
						code = Get_Byte();
						Packet_Header_Length += 1;
					} while (code == 0xff);
					if ((code & 0xc0) == 0x40)
					{
						// STD bytes.
						Get_Byte();
						code = Get_Byte();
						Packet_Header_Length += 2;
					}
					if ((code & 0xf0) == 0x20)
					{
						// PTS bytes.
						__int64 PES_PTS;

						PES_PTS = (code & 0x0e) << 29;
						PES_PTS |= (Get_Short() & 0xfffe) << 14;
						PES_PTS |= (Get_Short()>>1) & 0x7fff;
						AudioPTS = (unsigned int) (PES_PTS & 0xffffffff);
						if (LogTimestamps_Flag && D2V_Flag && StartLogging_Flag)
							fprintf(Timestamps, " A%2x PTS %u [%ums]\n", AUDIO_ID, AudioPTS, AudioPTS/90);
						Packet_Header_Length += 4;
					}
					else if ((code & 0xf0) == 0x30)
					{
						// PTS bytes.
						__int64 PES_PTS, PES_DTS;

						PES_PTS = (code & 0x0e) << 29;
						PES_PTS |= (Get_Short() & 0xfffe) << 14;
						PES_PTS |= (Get_Short()>>1) & 0x7fff;
						AudioPTS = (unsigned int) (PES_PTS & 0xffffffff);
						if (LogTimestamps_Flag && D2V_Flag && StartLogging_Flag)
							fprintf(Timestamps, " A%2x PTS %u [%ums]\n", AUDIO_ID, AudioPTS, AudioPTS/90);
						PES_DTS = (Get_Byte() & 0x0e) << 29;
						PES_DTS |= (Get_Short() & 0xfffe) << 14;
						PES_DTS |= (Get_Short()>>1) & 0x7fff;
						dts_stamp = (unsigned int) (PES_DTS & 0xffffffff);
						if (LogTimestamps_Flag && D2V_Flag && StartLogging_Flag)
							fprintf(Timestamps, " A%2x DTS %u [%ums]\n", AUDIO_ID, dts_stamp, dts_stamp/90);
						Packet_Header_Length += 9;
					}
					Packet_Length -= Packet_Header_Length;

					LOCATE

					if (!audio[AUDIO_ID].rip && Start_Flag && !audio[AUDIO_ID].type)
					{
                        audio[AUDIO_ID].type = FORMAT_MPA;

						code = Get_Byte();
						code = (code & 0xff)<<8 | Get_Byte();
						i = 0;

						while (code<0xfff0)
						{
emulated1:
							code = (code & 0xff)<<8 | Get_Byte();
							i++;
						}
                        // Try to detect emulated sync words by enforcing semantics.
                        if (check_audio_syncword(AUDIO_ID, (code >> 1) & 3,  (Rdptr[0] >> 4) & 0xf, (Rdptr[0] >> 2) & 3, (Rdptr[1] >> 6) & 3, Rdptr[1] & 3))
                        {
                                goto emulated1;
                        }

						Rdptr -= 2; Packet_Length -= i;

						if (D2V_Flag || AudioOnly_Flag)
						{
							if (Method_Flag==AUDIO_DEMUXALL || (Method_Flag==AUDIO_DEMUX && audio[AUDIO_ID].selected_for_demux == true))
							{
								if (AudioOnly_Flag)
								{
									char *p;
									strcpy(szOutput, Infilename[0]);
									p = &szOutput[sizeof(szOutput)];
									while (*p != '.') p--;
									*p = 0;
									sprintf(szBuffer, "%s T%x %s %s %s %s.%s", szOutput, AUDIO_ID,
											MPALayer[audio[AUDIO_ID].layer], MPAMode[audio[AUDIO_ID].mode], MPASample[audio[AUDIO_ID].sample],
											MPARate[audio[AUDIO_ID].layer][audio[AUDIO_ID].rate],
                                            UseMPAExtensions ? MPAExtension[0] : MPAExtension[audio[AUDIO_ID].layer]);
								}
								else
								{
									// Adjust the VideoPTS to account for frame reordering.
									if (!PTSAdjustDone)
									{
										PTSAdjustDone = 1;
										picture_period = 1.0 / frame_rate;
										VideoPTS -= (int) (LeadingBFrames * picture_period * 90000);
									}

									if (PTSDifference(AudioPTS, VideoPTS, &PTSDiff))
										sprintf(szBuffer, "%s T%x %s %s %s %s.%s", szOutput, AUDIO_ID,
												MPALayer[audio[AUDIO_ID].layer], MPAMode[audio[AUDIO_ID].mode], MPASample[audio[AUDIO_ID].sample],
												MPARate[audio[AUDIO_ID].layer][audio[AUDIO_ID].rate],
                                                UseMPAExtensions ? MPAExtension[0] : MPAExtension[audio[AUDIO_ID].layer]);
 									else
										sprintf(szBuffer, "%s T%x %s %s %s %s DELAY %dms.%s", szOutput, AUDIO_ID,
												MPALayer[audio[AUDIO_ID].layer], MPAMode[audio[AUDIO_ID].mode], MPASample[audio[AUDIO_ID].sample],
												MPARate[audio[AUDIO_ID].layer][audio[AUDIO_ID].rate], PTSDiff,
                                                UseMPAExtensions ? MPAExtension[0] : MPAExtension[audio[AUDIO_ID].layer]);
								}								
								audio[AUDIO_ID].file = OpenAudio(szBuffer, "wb", AUDIO_ID);
								if (audio[AUDIO_ID].file == NULL)
								{
									MessageBox(hWnd, "Cannot open file for audio demux output.\nAborting...", NULL, MB_OK | MB_ICONERROR);
									ThreadKill(MISC_KILL);
								}

								DEMUX_MPA_AAC(audio[AUDIO_ID].file);

								audio[AUDIO_ID].rip = true;
							}
						}
					}
					else if (audio[AUDIO_ID].rip)
						DEMUX_MPA_AAC(audio[AUDIO_ID].file);
					Rdptr += Packet_Length;
				}
				else
				{
					Packet_Length = Get_Short()-1;
					code = Get_Byte();

					if ((code & 0xc0)==0x80)
					{
						code = Get_Byte();	// +1
						Packet_Header_Length = Get_Byte();	// +1

						if (code>=0x80)
						{
							__int64 PES_PTS, PES_DTS;

							PES_PTS = (Get_Byte() & 0x0e) << 29;
							PES_PTS |= (Get_Short() & 0xfffe) << 14;
							PES_PTS |= (Get_Short()>>1) & 0x7fff;
							if (LogTimestamps_Flag && D2V_Flag && StartLogging_Flag)
								fprintf(Timestamps, " A%2x PTS %u [%ums]\n", AUDIO_ID, AudioPTS, AudioPTS/90);
							AudioPTS = (unsigned int) (PES_PTS & 0xffffffff);
							// DTS is not used. The code is here for analysis and debugging.
							if ((code & 0xc0) == 0xc0)
							{
								PES_DTS = (Get_Byte() & 0x0e) << 29;
								PES_DTS |= (Get_Short() & 0xfffe) << 14;
								PES_DTS |= (Get_Short()>>1) & 0x7fff;
								dts_stamp = (unsigned int) (PES_DTS & 0xffffffff);
								if (LogTimestamps_Flag && D2V_Flag && StartLogging_Flag)
									fprintf(Timestamps, "A%2x DTS %u [%ums]\n", AUDIO_ID, dts_stamp, dts_stamp/90);
								Rdptr += Packet_Header_Length - 10;
							}
							else
								Rdptr += Packet_Header_Length - 5;
						}
						else
							Rdptr += Packet_Header_Length;

						Packet_Length -= Packet_Header_Length+2;

						LOCATE

					    if (!audio[AUDIO_ID].rip && Start_Flag && !audio[AUDIO_ID].type)
						{
                            audio[AUDIO_ID].type = FORMAT_MPA;

							code = Get_Byte();
							code = (code & 0xff)<<8 | Get_Byte();
							i = 0;
							while (code < 0xfff0 && i < Packet_Length - 2)
							{
emulated2:
								code = (code & 0xff)<<8 | Get_Byte();
								i++;
							}
							if (i >= Packet_Length - 2)
							{
								break;
							}
                            // Try to detect emulated sync words by enforcing semantics.
                            if (check_audio_syncword(AUDIO_ID, (code >> 1) & 3,  (Rdptr[0] >> 4) & 0xf, (Rdptr[0] >> 2) & 3, (Rdptr[1] >> 6) & 3, Rdptr[1] & 3))
                            {
                                    goto emulated2;
                            }

							Rdptr -= 2; Packet_Length -= i;

							if (D2V_Flag || AudioOnly_Flag)
							{
							    if (Method_Flag==AUDIO_DEMUXALL || (Method_Flag==AUDIO_DEMUX && audio[AUDIO_ID].selected_for_demux == true))
								{
									if (AudioOnly_Flag)
									{
										char *p;
										strcpy(szOutput, Infilename[0]);
										p = &szOutput[sizeof(szOutput)];
										while (*p != '.') p--;
										*p = 0;
										sprintf(szBuffer, "%s T%x %s %s %s %s.%s", szOutput, AUDIO_ID,
												MPALayer[audio[AUDIO_ID].layer], MPAMode[audio[AUDIO_ID].mode], MPASample[audio[AUDIO_ID].sample],
												MPARate[audio[AUDIO_ID].layer][audio[AUDIO_ID].rate],
                                                UseMPAExtensions ? MPAExtension[0] : MPAExtension[audio[AUDIO_ID].layer]);
									}
									else
									{
										// Adjust the VideoPTS to account for frame reordering.
										if (!PTSAdjustDone)
										{
											PTSAdjustDone = 1;
											picture_period = 1.0 / frame_rate;
											VideoPTS -= (int) (LeadingBFrames * picture_period * 90000);
										}

										if (PTSDifference(AudioPTS, VideoPTS, &PTSDiff))
											sprintf(szBuffer, "%s T%x %s %s %s %s.%s", szOutput, AUDIO_ID,
													MPALayer[audio[AUDIO_ID].layer], MPAMode[audio[AUDIO_ID].mode], MPASample[audio[AUDIO_ID].sample],
													MPARate[audio[AUDIO_ID].layer][audio[AUDIO_ID].rate],
                                                    UseMPAExtensions ? MPAExtension[0] : MPAExtension[audio[AUDIO_ID].layer]);
										else
										    sprintf(szBuffer, "%s T%x %s %s %s %s DELAY %dms.%s", szOutput, AUDIO_ID,
												    MPALayer[audio[AUDIO_ID].layer], MPAMode[audio[AUDIO_ID].mode], MPASample[audio[AUDIO_ID].sample],
												    MPARate[audio[AUDIO_ID].layer][audio[AUDIO_ID].rate], PTSDiff,
                                                    UseMPAExtensions ? MPAExtension[0] : MPAExtension[audio[AUDIO_ID].layer]);
									}
									audio[AUDIO_ID].file = OpenAudio(szBuffer, "wb", AUDIO_ID);
									if (audio[AUDIO_ID].file == NULL)
									{
										MessageBox(hWnd, "Cannot open file for audio demux output.\nAborting...", NULL, MB_OK | MB_ICONERROR);
										ThreadKill(MISC_KILL);
									}

									DEMUX_MPA_AAC(audio[AUDIO_ID].file);

									audio[AUDIO_ID].rip = true;
								}
							}
						}
						else if (audio[AUDIO_ID].rip)
							DEMUX_MPA_AAC(audio[AUDIO_ID].file);
					}
					Rdptr += Packet_Length;
				}
				if (AudioOnly_Flag && Info_Flag && !(AudioPktCount++ % 128))
					UpdateInfo();

				break;

			default:
				if ((code & 0xfffffff0) == VIDEO_ELEMENTARY_STREAM)
				{
					Packet_Length = Get_Short();
					Rdmax = Rdptr + Packet_Length;

					if (stream_type == MPEG1_PROGRAM_STREAM)
					{
						__int64 PES_PTS, PES_DTS;
						unsigned int pts_stamp;

						// MPEG1 program stream.
						Packet_Header_Length = 0;
						// Stuffing bytes.
						do 
						{
							code = Get_Byte();
							Packet_Header_Length += 1;
						} while (code == 0xff);
						if ((code & 0xc0) == 0x40)
						{
							// STD bytes.
							Get_Byte();
							code = Get_Byte();
							Packet_Header_Length += 2;
						}
						if ((code & 0xf0) == 0x20)
						{
							// PTS bytes.
							PES_PTS = (code & 0x0e) << 29;
							PES_PTS |= (Get_Short() & 0xfffe) << 14;
							PES_PTS |= (Get_Short()>>1) & 0x7fff;
							pts_stamp = (unsigned int) (PES_PTS & 0xffffffff);
							if (LogTimestamps_Flag && D2V_Flag && StartLogging_Flag)
								fprintf(Timestamps, "V PTS %u [%ums]\n", pts_stamp, pts_stamp/90);
							Packet_Header_Length += 4;
						}
						else if ((code & 0xf0) == 0x30)
						{
							// PTS/DTS bytes.
							PES_PTS = (code & 0x0e) << 29;
							PES_PTS |= (Get_Short() & 0xfffe) << 14;
							PES_PTS |= (Get_Short()>>1) & 0x7fff;
							pts_stamp = (unsigned int) (PES_PTS & 0xffffffff);
							if (LogTimestamps_Flag && D2V_Flag && StartLogging_Flag)
								fprintf(Timestamps, "V PTS %u [%ums]\n", pts_stamp, pts_stamp/90);
							PES_DTS = (Get_Byte() & 0x0e) << 29;
							PES_DTS |= (Get_Short() & 0xfffe) << 14;
							PES_DTS |= (Get_Short()>>1) & 0x7fff;
							dts_stamp = (unsigned int) (PES_DTS & 0xffffffff);
							if (LogTimestamps_Flag && D2V_Flag && StartLogging_Flag)
								fprintf(Timestamps, "V DTS %u [%ums]\n", dts_stamp, dts_stamp/90);
							Packet_Header_Length += 9;
						}
						else
						{
							// Just to kill a compiler warning.
							pts_stamp = 0;
						}
						if (!Start_Flag)
						{
							// Start_Flag becomes true after the first I frame.
							// So VideoPTS will be left at the value corresponding to
							// the first I frame.
							VideoPTS = pts_stamp;
						}
						Bitrate_Monitor += Rdmax - Rdptr;
						return;
					}
					else
					{
						// MPEG2 program stream.
						code = Get_Byte();
						if ((code & 0xc0) == 0x80)
						{
							__int64 PES_PTS, PES_DTS;
							unsigned int pts_stamp, dts_stamp;

							code = Get_Byte();
							Packet_Header_Length = Get_Byte();

							if (code >= 0x80)
							{
								PES_PTS = (Get_Byte() & 0x0e) << 29;
								PES_PTS |= (Get_Short() & 0xfffe) << 14;
								PES_PTS |= (Get_Short()>>1) & 0x7fff;
								pts_stamp = (unsigned int) (PES_PTS & 0xffffffff);
								if (LogTimestamps_Flag && D2V_Flag && StartLogging_Flag)
									fprintf(Timestamps, "V PTS %u [%ums]\n", pts_stamp, pts_stamp/90);
								// DTS is not used. The code is here for analysis and debugging.
								if ((code & 0xc0) == 0xc0)
								{
									PES_DTS = (Get_Byte() & 0x0e) << 29;
									PES_DTS |= (Get_Short() & 0xfffe) << 14;
									PES_DTS |= (Get_Short()>>1) & 0x7fff;
									dts_stamp = (unsigned int) (PES_DTS & 0xffffffff);
									if (LogTimestamps_Flag && D2V_Flag && StartLogging_Flag)
										fprintf(Timestamps, "V DTS %u [%ums]\n", dts_stamp, dts_stamp/90);
									Rdptr += Packet_Header_Length - 10;
								}
								else
									Rdptr += Packet_Header_Length - 5;

								if (!Start_Flag)
								{
									// Start_Flag becomes true after the first I frame.
									// So VideoPTS will be left at the value corresponding to
									// the first I frame.
									VideoPTS = pts_stamp;
								}
							}
							else
								Rdptr += Packet_Header_Length;

							Bitrate_Monitor += Rdmax - Rdptr;
							return;
						}
						else
						{
							Rdptr += Packet_Length-1;
						}
					}
				}
				else if (code>=SYSTEM_START_CODE)
				{
					Packet_Length = Get_Short();
                    if (code == 0x1be && Packet_Length > 2048)
                    {
                        continue;
                    }
					Rdptr += Packet_Length;
				}
				break;
		}
	}
}

unsigned int Get_Bits_All(unsigned int N)
{
	N -= BitsLeft;
	Val = (CurrentBfr << (32 - BitsLeft)) >> (32 - BitsLeft);

	if (N)
		Val = (Val << N) + (NextBfr >> (32 - N));

	VideoDemux();
	CurrentBfr = NextBfr;
	BitsLeft = 32 - N;
	Fill_Next();

	return Val;
}

void Flush_Buffer_All(unsigned int N)
{
	VideoDemux();
	CurrentBfr = NextBfr;
	BitsLeft = BitsLeft + 32 - N;
	Fill_Next();
}

void Fill_Buffer()
{
	Read = _donread(Infile[CurrentFile], Rdbfr, BUFFER_SIZE);

//	dprintf("DGIndex: Fill buffer\n");
	if (Read < BUFFER_SIZE)	Next_File();

	Rdptr = Rdbfr;

	if (SystemStream_Flag != ELEMENTARY_STREAM)
	{
		Rdmax -= BUFFER_SIZE;
	}
	else
		Bitrate_Monitor += Read;
}

void Next_File()
{
	int i, bytes;
	unsigned char *p;
	
	if (CurrentFile < NumLoadedFiles-1)
	{
		CurrentFile++;
		process.run = 0;
		for (i=0; i<CurrentFile; i++) process.run += Infilelength[i];
		_lseeki64(Infile[CurrentFile], 0, SEEK_SET);
		bytes = _donread(Infile[CurrentFile], Rdbfr + Read, BUFFER_SIZE - Read);
//		dprintf("DGIndex: Next file at %d\n", Rdbfr + Read);
		if (Read + bytes == BUFFER_SIZE)
			// The whole buffer has valid data.
			buffer_invalid = (unsigned char *) 0xffffffff;
		else
		{
			// Point to the first invalid buffer location.
			buffer_invalid = Rdbfr + Read + bytes;
			p = Rdbfr + Read + bytes;
			while (p < Rdbfr + BUFFER_SIZE) *p++ = 0xFF;
		}
	}
	else
	{
		buffer_invalid = Rdbfr + Read;
		p = Rdbfr + Read;
		while (p < Rdbfr + BUFFER_SIZE) *p++ = 0xFF;
	}
}

void UpdateInfo()
{
	static int Old_VIDEO_Purity;
	unsigned int pts;
	int profile, level;
	extern int Clip_Width, Clip_Height, profile_and_level_indication;
	int i;
    double video_percent, film_percent;

	if (!AudioOnly_Flag)
	{
		switch (SystemStream_Flag)
		{
		case ELEMENTARY_STREAM:
			sprintf(szBuffer, "Elementary");
			break;
		case PROGRAM_STREAM:
			if (program_stream_type == MPEG1_PROGRAM_STREAM)
				sprintf(szBuffer, "MPEG1 Program");
			else
				sprintf(szBuffer, "MPEG2 Program");
			break;
		case TRANSPORT_STREAM:
			sprintf(szBuffer, "Transport [%d]", TransportPacketSize);
			break;
		case PVA_STREAM:
			sprintf(szBuffer, "PVA");
			break;
		}
		SetDlgItemText(hDlg, IDC_STREAM_TYPE, szBuffer);

		if (mpeg_type == IS_MPEG2)
        {
            if (aspect_ratio_information < sizeof(AspectRatio) / sizeof(char *))
                sprintf(szBuffer, "%s [%d]", AspectRatio[aspect_ratio_information], aspect_ratio_information);
            else
                sprintf(szBuffer, "Reserved");
        }
		else
			sprintf(szBuffer, "%s", AspectRatioMPEG1[aspect_ratio_information]);
		SetDlgItemText(hDlg, IDC_ASPECT_RATIO, szBuffer);

		sprintf(szBuffer, "%dx%d", Clip_Width, Clip_Height);
		SetDlgItemText(hDlg, IDC_FRAME_SIZE, szBuffer);

        if (display_horizontal_size == 0)
        {
		    sprintf(szBuffer, "[not specified]");
        }
        else
        {
		    sprintf(szBuffer, "%dx%d", display_horizontal_size, display_vertical_size);
        }
		SetDlgItemText(hDlg, IDC_DISPLAY_SIZE, szBuffer);

		if (mpeg_type == IS_MPEG2)
		{
			profile = (profile_and_level_indication >> 4) & 0x7;
			level = profile_and_level_indication & 0xf;
			switch (profile)
			{
			case 1: // high
				strcpy(szBuffer, "high@");
				break;
			case 2: // spatial
				strcpy(szBuffer, "spatial@");
				break;
			case 3: // SNR
				strcpy(szBuffer, "snr@");
				break;
			case 4: // main
				strcpy(szBuffer, "main@");
				break;
			case 5: // simple
				strcpy(szBuffer, "simple@");
				break;
			default:
				sprintf(szBuffer, "esc (%d@%d)", profile, level);
				break;
			}
			switch (level)
			{
			case 4:
				strcat(szBuffer, "high");
				break;
			case 6:
				strcat(szBuffer, "high1440");
				break;
			case 8:
				strcat(szBuffer, "main");
				break;
			case 10:
				strcat(szBuffer, "low");
				break;
			default:
				break;
			}
			SetDlgItemText(hDlg, IDC_PROFILE, szBuffer);
		}
		else
		{
			SetDlgItemText(hDlg, IDC_PROFILE, "[MPEG1]");
		}

		sprintf(szBuffer, "%.6f fps", Frame_Rate);
		SetDlgItemText(hDlg, IDC_FRAME_RATE, szBuffer);
	}

	while (SendDlgItemMessage(hDlg, IDC_AUDIO_LIST, LB_DELETESTRING, 0, 0) != LB_ERR);
    for (i = 0; i < 0xc8; i++)
    {
        if (audio[i].type != 0)
        {
		    switch (audio[i].type)
		    {
			    case FORMAT_AC3:
				    sprintf(szBuffer, "%x: AC3 %s %d", i, AC3Mode[audio[i].mode], AC3Rate[audio[i].rate]);
				    break;

			    case FORMAT_MPA:
			        sprintf(szBuffer, "%x: MPA %s %s %s %s", i,
							MPALayer[audio[i].layer], MPAMode[audio[i].mode], MPASample[audio[i].sample],
							MPARate[audio[i].layer][audio[i].rate]);
				    break;

			    case FORMAT_AAC:
			        sprintf(szBuffer, "%x: AAC Audio", i);
				    break;

                case FORMAT_LPCM:
				    sprintf(szBuffer, "%x: PCM %s %s %dch", i,
					    (audio[i].format & 0x30) == 0 ? "48K" : "96K",
					    (audio[i].format & 0xc0) == 0 ? "16bit" : ((audio[i].format & 0xc0) == 0x40 ? "20bit" : "24bit"),
					    (audio[i].format & 0x07) + 1);
				    break;

			    case FORMAT_LPCM_M2TS:
			    {
				    char sample_rate[16];
				    char bits_per_sample[16];
				    char num_channels[16];
				    switch ((audio[i].format_m2ts & 0xf00) >> 8)
				    {
					    case 1:
						    sprintf(sample_rate, "48K");
						    break;
					    case 4:
						    sprintf(sample_rate, "96K");
						    break;
					    case 5:
						    sprintf(sample_rate, "192K");
						    break;
					    default:
						    sprintf(sample_rate, "RES");
						    break;
				    }
				    switch ((audio[i].format_m2ts & 0xc0) >> 6)
				    {
					    case 1:
						    sprintf(bits_per_sample, "16bit");
						    break;
					    case 2:
						    sprintf(bits_per_sample, "20bit");
						    break;
					    case 3:
						    sprintf(bits_per_sample, "24bit");
						    break;
					    default:
						    sprintf(bits_per_sample, "RES");
						    break;
				    }
				    switch ((audio[i].format_m2ts & 0xf000) >> 12)
				    {
					    case 1:
						    sprintf(num_channels, "1/0");
						    break;
					    case 3:
						    sprintf(num_channels, "2/0");
						    break;
					    case 4:
						    sprintf(num_channels, "3/0");
						    break;
					    case 5:
						    sprintf(num_channels, "2/1");
						    break;
					    case 6:
						    sprintf(num_channels, "3/1");
						    break;
					    case 7:
						    sprintf(num_channels, "2/2");
						    break;
					    case 8:
						    sprintf(num_channels, "3/2");
						    break;
					    case 9:
						    sprintf(num_channels, "3/2lfe");
						    break;
					    case 10:
						    sprintf(num_channels, "3/4");
						    break;
					    case 11:
						    sprintf(num_channels, "3/4lfe");
						    break;
					    default:
						    printf("LPCM Audio Mode = reserved\n");
						    break;
				    }
				    sprintf(szBuffer, "%x: PCM %s %s %s", i, sample_rate, bits_per_sample, num_channels);
				    break;
			    }

			    case FORMAT_DTS:
			        sprintf(szBuffer, "%x: DTS Audio", i);
				    break;

			    default:
			        sprintf(szBuffer, "");
				    break;
		    }
            SendDlgItemMessage(hDlg, IDC_AUDIO_LIST, LB_ADDSTRING, 0, (LPARAM)szBuffer);
		    if (SystemStream_Flag == TRANSPORT_STREAM)
			    break;
        }
    }

	if (AudioOnly_Flag || (SystemStream_Flag != ELEMENTARY_STREAM && process.locate != LOCATE_INIT))
	{
		unsigned int hours, mins, secs;
		__int64 processed;
		int i, trackpos;

		pts = AudioPTS/90000;
		hours = pts / 3600;
		mins = (pts % 3600) / 60;
		secs = pts % 60;
		sprintf(szBuffer, "%d:%02d:%02d", hours, mins, secs);
		SetDlgItemText(hDlg, IDC_TIMESTAMP, szBuffer);
		for (i = 0, processed = 0; i < CurrentFile; i++)
		{
			processed += Infilelength[i];
		}
		processed += _telli64(Infile[CurrentFile]);
		processed *= TRACK_PITCH;
		processed /= Infiletotal;
		trackpos = (int) processed;
		SendMessage(hTrack, TBM_SETPOS, (WPARAM)true, trackpos);
    	InvalidateRect(hwndSelect, NULL, TRUE);
	}
	else
		SetDlgItemText(hDlg, IDC_TIMESTAMP, "");

	if (process.locate==LOCATE_RIP)
	{
		if (VIDEO_Purity || FILM_Purity)
		{
			if (!FILM_Purity)
			{
				if (frame_rate==25 || frame_rate==50)
					sprintf(szBuffer, "PAL");
				else
					sprintf(szBuffer, "NTSC");
			}
			else if (!VIDEO_Purity)
				sprintf(szBuffer, "Film");
			else if (VIDEO_Purity > Old_VIDEO_Purity)
			{
				video_percent = 100.0 - (FILM_Purity*100.0)/(FILM_Purity+VIDEO_Purity);
				if (video_percent > 50)
					sprintf(szBuffer, "Video %.2f%%", video_percent);
				else
					sprintf(szBuffer, "Film %.2f%%", 100.0 - video_percent);
			}
			else
			{
				film_percent = (FILM_Purity*100.0)/(FILM_Purity+VIDEO_Purity);
				if (film_percent > 50)
					sprintf(szBuffer, "Film %.2f%%", film_percent);
				else
					sprintf(szBuffer, "Video %.2f%%", 100.0 - film_percent);
			}

			Old_VIDEO_Purity = VIDEO_Purity;
			SetDlgItemText(hDlg, IDC_VIDEO_TYPE, szBuffer);
		}
	}
	else
	{
		SetDlgItemText(hDlg, IDC_VIDEO_TYPE, "");
		SetDlgItemText(hDlg, IDC_FRAME_TYPE, "");
		SetDlgItemText(hDlg, IDC_CODED_NUMBER, "");
		SetDlgItemText(hDlg, IDC_PLAYBACK_NUMBER, "");
		SetDlgItemText(hDlg, IDC_BITRATE,"");
		SetDlgItemText(hDlg, IDC_BITRATE_AVG,"");
		SetDlgItemText(hDlg, IDC_BITRATE_MAX,"");
		SetDlgItemText(hDlg, IDC_ELAPSED, "");
		SetDlgItemText(hDlg, IDC_REMAIN, "");
		SetDlgItemText(hDlg, IDC_FPS, "");
	}
}

// Video demuxing functions.
static int first_video_demux;

void StartVideoDemux(void)
{
	char path[1024];
	char *p;

	strcpy(path, D2VFilePath);
	p = path + strlen(path);
	while (*p != '.' && p >= path) p--;
	if (p < path)
	{
		// No extension in this name. WTF?
		p = path;
		strcat(path, ".");
	}
	else
		p[1] = 0;
	if (mpeg_type == IS_MPEG2)
		strcat(p, "demuxed.m2v");
	else
		strcat(p, "demuxed.m1v");
	MuxFile = fopen(path, "wb");
	if (MuxFile == (FILE *) 0)
	{
		MessageBox(hWnd, "Cannot open file for video demux output.", NULL, MB_OK | MB_ICONERROR);
		MuxFile = (FILE *) 0xffffffff;
		return;
	}
    first_video_demux = 1;
}

void StopVideoDemux(void)
{
	unsigned char c;

    // Flush the last usable bytes in NextBfr.
	c = NextBfr >> 24;
	if (c != 0xff)
	{
		fwrite(&c, 1, 1, MuxFile);
		c = (NextBfr >> 16) & 0xff;
		if (c != 0xff)
		{
			fwrite(&c, 1, 1, MuxFile);
			c = (NextBfr >> 8) & 0xff;
			if (c != 0xff)
			{
				fwrite(&c, 1, 1, MuxFile);
				c = NextBfr & 0xff;
				if (c != 0xff)
				{
					fwrite(&c, 1, 1, MuxFile);
				}
			}
		}
	}
}

void VideoDemux(void)
{
	unsigned char buf[8];

	if (MuxFile == (FILE *) 0xffffffff || MuxFile <= 0)
		return;
	buf[0] = CurrentBfr >> 24;
	buf[1] = (CurrentBfr >> 16) & 0xff;
	buf[2] = (CurrentBfr >> 8) & 0xff;
	buf[3] = CurrentBfr & 0xff;
    if (first_video_demux == 1)
    {
        // Start demuxing at the first sequence header.
	    buf[4] = NextBfr >> 24;
	    buf[5] = (NextBfr >> 16) & 0xff;
	    buf[6] = (NextBfr >> 8) & 0xff;
	    buf[7] = NextBfr & 0xff;
        first_video_demux = 0;
        if (buf[0] == 0 && buf[1] == 0 && buf[2] == 1 && buf[3] == 0xb3)
 	        fwrite(&buf[0], 1, 4, MuxFile);
        else if (buf[1] == 0 && buf[2] == 0 && buf[3] == 1 && buf[4] == 0xb3)
	        fwrite(&buf[1], 1, 3, MuxFile);
        else if (buf[2] == 0 && buf[3] == 0 && buf[4] == 1 && buf[5] == 0xb3)
	        fwrite(&buf[2], 1, 2, MuxFile);
        else if (buf[3] == 0 && buf[4] == 0 && buf[5] == 1 && buf[6] == 0xb3)
	        fwrite(&buf[3], 1, 1, MuxFile);
        else
            first_video_demux = 1;
    }
    else
	    fwrite(&buf[0], 1, 4, MuxFile);
}
