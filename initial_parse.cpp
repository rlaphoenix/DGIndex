/* 
 *  Copyright (C) 2005, Donald Graft
 *
 *  This file is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  This file is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 */

#include <windows.h>
#include "resource.h"
#include <stdio.h> 
#include <io.h> 
#include <fcntl.h>
#include "global.h"

// Defines for the start code detection state machine.
#define NEED_FIRST_0  0
#define NEED_SECOND_0 1
#define NEED_1        2

static void determine_stream_type(void);
static void video_parser(int *);
static void pack_parser(void);
static unsigned char get_byte(void);

static int file;
static int state, found;
// Should hold maximum size PES packet.
static unsigned char buffer[256000];
static int buffer_length, buffer_ndx;
static int EOF_reached;

int initial_parse(char *input_file, int *mpeg_type_p, int *is_program_stream_p)
{
	// Open the input file.
	if (input_file[0] == 0 || (file = _open(input_file, _O_RDONLY | _O_BINARY | _O_SEQUENTIAL)) == -1)
	{
		return -1;
	}

	// Determine the stream type: ES or program.
	determine_stream_type();

	// Re-open the input file.
	_close(file);
	file = _open(input_file, _O_RDONLY | _O_BINARY | _O_SEQUENTIAL);
	if (file == -1)
	{
		return -1;
	}

	// Determine MPEG type and find location of first video sequence header.
	EOF_reached = 0;
	video_parser(mpeg_type_p);
	_close(file);
	if (EOF_reached)
	{
		return -1;
	}
	*is_program_stream_p = program_stream_type;
	return 0;
}

static unsigned char get_byte(void)
{
	unsigned char val;

	if (program_stream_type != ELEMENTARY_STREAM)
	{
		if (buffer_ndx >= buffer_length)
		{
			// Need more data. Fill buffer with the next
			// packet's worth of elementary data.
			pack_parser();
		}
		if (EOF_reached == 1) 
		{
			return 0;
		}
		return buffer[buffer_ndx++];
	}
	else
	{
		// No program stream parsing needed. Just read from
		// the input file.
		if (_read(file, &val, 1) != 1)
		{
			EOF_reached = 1;
			return 0;
		}
		return val;
	}
}

static void video_parser(int *mpeg_type_p)
{
	unsigned char val;
	int sequence_header_active = 0;

	buffer_length = buffer_ndx = 0;

	// Inits.
	state = NEED_FIRST_0;
	found = 0;

	// Let's go! Start by assuming it's not MPEG at all.
	*mpeg_type_p = IS_NOT_MPEG;
	for (;;)
	{
		// Parse for start codes.
		val = get_byte();
		if (EOF_reached) return;
		switch (state)
		{
		case NEED_FIRST_0:
			if (val == 0)
				state = NEED_SECOND_0;
			break;
		case NEED_SECOND_0:
			if (val == 0)
				state = NEED_1;
			else
				state = NEED_FIRST_0;
			break;
		case NEED_1:
			if (val == 1)
			{
				found = 1;
				state = NEED_FIRST_0;
			}
			else if (val != 0)
				state = NEED_FIRST_0;
			break;
		}
		if (found == 1)
		{
			// Found a start code.
			found = 0;
			val = get_byte();
			if (EOF_reached) return;

			if (val == 0xB3)
			{
				// Found a sequence header, so it's at least MPEG1.
				*mpeg_type_p = IS_MPEG1;
				// Sequence header.
				if (sequence_header_active)
				{
					// Got another sequence header but didn't see a
					// sequence header extension, so it's MPEG1.
					return;
				}
				sequence_header_active = 1;
			}
			else if (val == 0xB5)
			{
				val = get_byte();
				if (EOF_reached) return;
				if ((val & 0xf0) == 0x10)
				{
					// Sequence extension.
					if (sequence_header_active)
					{
						// Must be MPEG2.
						*mpeg_type_p = IS_MPEG2;
						return;
					}
				}
				else if (sequence_header_active)
				{
					// Got some other extension. Must be MPEG1.
					*mpeg_type_p = IS_MPEG1;
					return;
				}
			}
			else
			{
				if (sequence_header_active)
				{
					// No sequence header extension. Must be MPEG1.
					*mpeg_type_p = IS_MPEG1;
					return;
				}
			}
		}
	}
	return;
}

static void pack_parser(void)
{
	unsigned char val, val2;
	int state, found = 0;
	int length;
	unsigned short pes_packet_length, system_header_length;
	unsigned char pes_packet_header_length, pack_stuffing_length;
	unsigned char code;


	// Look for start codes.
	state = NEED_FIRST_0;
	for (;;)
	{
		if (_read(file, &val, 1) != 1) { EOF_reached = 1; return; }
		switch (state)
		{
		case NEED_FIRST_0:
			if (val == 0)
				state = NEED_SECOND_0;
			break;
		case NEED_SECOND_0:
			if (val == 0)
				state = NEED_1;
			else
				state = NEED_FIRST_0;
			break;
		case NEED_1:
			if (val == 1)
			{
				found = 1;
				state = NEED_FIRST_0;
			}
			else if (val != 0)
				state = NEED_FIRST_0;
			break;
		}
		if (found == 1)
		{
			// Found a start code.
			found = 0;
			// Get the start code.
			if (_read(file, &val, 1) != 1) { EOF_reached = 1; return; }
//			dprintf("DGIndex: code = %x\n", val);
			if (val == 0xba)
			{
				// Pack header. Skip it.
				if (_read(file, &val, 1) != 1) { EOF_reached = 1; return; }
				if (program_stream_type == MPEG1_PROGRAM_STREAM)
				{
					// MPEG1 program stream.
					if (_read(file, buffer, 7) != 7) { EOF_reached = 1; return; }
				}
				else
				{
					// MPEG2 program stream.
					if (_read(file, buffer, 8) != 8) { EOF_reached = 1; return; }
					if (_read(file, &pack_stuffing_length, 1) != 1) { EOF_reached = 1; return; }
					pack_stuffing_length &= 0x03;
					if (_read(file, buffer, pack_stuffing_length) != pack_stuffing_length) { EOF_reached = 1; return; }
				}
			}
			if (val == 0xbb)
			{
				// System header. Skip it.
				if (_read(file, &val, 1) != 1)  { EOF_reached = 1; return; }
				system_header_length = (unsigned short) (val << 8);
				if (_read(file, &val2, 1) != 1)  { EOF_reached = 1; return; }
				system_header_length |= val2;
				if (_read(file, buffer, system_header_length) != system_header_length) { EOF_reached = 1; return; }
			}
			else if (val >= 0xE0 && val <= 0xEF)
			{
				// Video stream.
				// Packet length.
				if (_read(file, &val, 1) != 1)  { EOF_reached = 1; return; }
				pes_packet_length = (unsigned short) (val << 8);
				if (_read(file, &val2, 1) != 1)  { EOF_reached = 1; return; }
				pes_packet_length |= val2;
 //               if (pes_packet_length == 0)
 //                   pes_packet_length = 256;
				if (program_stream_type == MPEG1_PROGRAM_STREAM)
				{
					// MPEG1 program stream.
					pes_packet_header_length = 0;
					// Stuffing bytes.
					do 
					{
						if (_read(file, &val, 1) != 1)  { EOF_reached = 1; return; }
						pes_packet_header_length += 1;
					} while (val == 0xff);
					if ((val & 0xc0) == 0x40)
					{
						// STD bytes.
						if (_read(file, &val, 1) != 1)  { EOF_reached = 1; return; }
						if (_read(file, &val, 1) != 1)  { EOF_reached = 1; return; }
						pes_packet_header_length += 2;
					}
					if ((val & 0xf0) == 0x20)
					{
						// PTS bytes.
						if (_read(file, buffer, 4) != 4)  { EOF_reached = 1; return; }
						pes_packet_header_length += 4;
					}
					else if ((val & 0xf0) == 0x30)
					{
						// PTS/DTS bytes.
						if (_read(file, buffer, 9) != 9)  { EOF_reached = 1; return; }
						pes_packet_header_length += 9;
					}
					// Send the video elementary data down the pipe to the video parser.
					buffer_length = pes_packet_length - pes_packet_header_length;
					if (buffer_length)
					{
						buffer_ndx = 0;
						if (_read(file, buffer, buffer_length) != buffer_length)  { EOF_reached = 1; return; }
						return;
					}
				}
				else
				{
					// MPEG2 program stream.
					// Flags.
					if (_read(file, &code, 1) != 1)  { EOF_reached = 1; return; }
					if ((code & 0xc0) == 0x80)
					{
						if (_read(file, &code, 1) != 1)  { EOF_reached = 1; return; }
						if (_read(file, &pes_packet_header_length, 1) != 1) { EOF_reached = 1; return; }
						// Skip the PES packet header.
						if (_read(file, buffer, pes_packet_header_length) != pes_packet_header_length)  { EOF_reached = 1; return; }
						// Send the video elementary data down the pipe to the video parser.
						buffer_length = pes_packet_length - pes_packet_header_length - 3;
						buffer_ndx = 0;
						if (_read(file, buffer, buffer_length) != buffer_length)  { EOF_reached = 1; return; }
						return;
					}
					else
					{
						// No video data here. Skip it.
						length = pes_packet_length - 1;
						if (_read(file, buffer, length) != length)  { EOF_reached = 1; return; }
					}
				}
			}
			else if (val > 0xbb)
			{
				// Not a stream that we are interested in. Skip it.
				if (_read(file, &val, 1) != 1)  { EOF_reached = 1; return; }
				pes_packet_length = (unsigned short) (val << 8);
				if (_read(file, &val2, 1) != 1)  { EOF_reached = 1; return; }
				pes_packet_length |= val2;
				length = pes_packet_length;
				if (_read(file, buffer, length) != length)  { EOF_reached = 1; return; }
			}
		}
	}
}

unsigned char stream[2500000];
static void determine_stream_type(void)
{
	unsigned char val;
	int num_sequence_header = 0;
	int num_picture_header = 0;
	unsigned char *p;
	unsigned int code, Read;

	// Start by assuming ES. Then look for a valid pack start. If one
	// is found declare a program stream.
	program_stream_type = ELEMENTARY_STREAM;

	Read = _read(file, stream, 2500000);
	p = stream;
	code = (p[0] << 16) + (p[1] << 8) + p[2];
	p += 3;
	while (p < stream + Read)
	{
		code = ((code << 8) + *p++);
		switch (code)
		{
		case 0x1ba:
			val = *p++;
			if ((val & 0xf0) == 0x20)
			{
				// Check all the marker bits just to be sure.
				if (!(val & 1)) continue;
				val = *p++;
				val = *p++;
				if (!(val & 1)) continue;
				val = *p++;
				val = *p++;
				if (!(val & 1)) continue;
				val = *p++;
				if (!(val & 0x80)) continue;
				val = *p++;
				val = *p++;
				if (!(val & 1)) continue;
				// MPEG1 program stream.
				program_stream_type = MPEG1_PROGRAM_STREAM;
				return;
			}
			else if ((val & 0xc0) == 0x40)
			{
				// Check all the marker bits just to be sure.
				if (!(val & 0x04)) continue;
				val = *p++;
				val = *p++;
				if (!(val & 0x04)) continue;
				val = *p++;
				val = *p++;
				if (!(val & 0x04)) continue;
				val = *p++;
				if (!(val & 0x01)) continue;
				val = *p++;
				val = *p++;
				val = *p++;
				if (!(val & 0x03)) continue;
				// MPEG2 program stream.
				program_stream_type = MPEG2_PROGRAM_STREAM;
				return;
			}
			break;
		case 0x1b3:
			// Sequence header.
			num_sequence_header++;
			break;
		case 0x100:
			// Picture header.
			num_picture_header++;
			break;
		}
		if (num_sequence_header >= 2 && num_picture_header >= 2)
		{
			// We're seeing a lot of elementary stream data but we haven't seen
			// a pack header yet. Declare ES.
			return;
		}
	}
}
