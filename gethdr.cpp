/* Copyright (C) 1996, MPEG Software Simulation Group. All Rights Reserved. */

/*
 * Disclaimer of Warranty
 *
 * These software programs are available to the user without any license fee or
 * royalty on an "as is" basis.  The MPEG Software Simulation Group disclaims
 * any and all warranties, whether express, implied, or statuary, including any
 * implied warranties or merchantability or of fitness for a particular
 * purpose.  In no event shall the copyright-holder be liable for any
 * incidental, punitive, or consequential damages of any kind whatsoever
 * arising from the use of these programs.
 *
 * This disclaimer of warranty extends to the user of these programs and user's
 * customers, employees, agents, transferees, successors, and assigns.
 *
 * The MPEG Software Simulation Group does not represent or warrant that the
 * programs furnished hereunder are free of infringement of any third-party
 * patents.
 *
 * Commercial implementations of MPEG-1 and MPEG-2 video, including shareware,
 * are subject to royalty fees to patent holders.  Many of these patents are
 * general enough such that they are unavoidable regardless of implementation
 * design.
 *
 */

#include "global.h"
#include "getbit.h"

static int load_intra_quantizer_matrix;
static int load_non_intra_quantizer_matrix;
static int load_chroma_intra_quantizer_matrix;
static int load_chroma_non_intra_quantizer_matrix;
static int frame_rate_code;

static double frame_rate_Table[16] = 
{
	0.0,
	((24.0*1000.0)/1001.0),
	24.0,
	25.0,
	((30.0*1000.0)/1001.0),
	30.0,
	50.0,
	((60.0*1000.0)/1001.0),
	60.0,
	-1,		// reserved
	-1,
	-1,
	-1,
	-1,
	-1,
	-1
};

static unsigned int frame_rate_Table_Num[16] = 
{
	0,
	24000,
	24,
	25,
	30000,
	30,
	50,
	60000,
	60,
	0xFFFFFFFF,		// reserved
	0xFFFFFFFF,
	0xFFFFFFFF,
	0xFFFFFFFF,
	0xFFFFFFFF,
	0xFFFFFFFF,
	0xFFFFFFFF
};

static unsigned int frame_rate_Table_Den[16] = 
{
	0,
	1001,
	1,
	1,
	1001,
	1,
	1,
	1001,
	1,
	0xFFFFFFFF,		// reserved
	0xFFFFFFFF,
	0xFFFFFFFF,
	0xFFFFFFFF,
	0xFFFFFFFF,
	0xFFFFFFFF,
	0xFFFFFFFF
};

__forceinline static void group_of_pictures_header(void);
__forceinline static void picture_header(__int64, boolean, boolean);

static void sequence_extension(void);
static void sequence_display_extension(void);
static void quant_matrix_extension(void);
static void picture_display_extension(void);
static void picture_coding_extension(void);
static void copyright_extension(void);
static int  extra_bit_information(void);
static void extension_and_user_data(void);
void StartVideoDemux(void);

//////////
// We index the on-disk file position for accessing each I picture. 
// We prefer to index a sequence header immediately accompanying
// the GOP, because it might carry new quant matrices that we would need
// to read if we randomly access the picture. If no sequence header is
// there, we index the I picture header start code. For ES, we index
// directly each indexed location. For PES, we index the position of
// the previous packet start code for each indexed location.
//
// There are two different indexing strategies, one for ES and one for PES.
// The ES scheme directly calculates the required file offset.
// The PES scheme uses CurrentPackHeaderPosition for the on-disk file
// offset of the last pack start code. This variable is irrelevant for ES!
// The pack header position is double-buffered through PackHeaderPosition
// and CurrentPackHeaderPosition to avoid a vulnerability in which
// PackHeaderPosition can become invalid due to a Fill_Next() call
// triggering a Next_Packet() call, which would rewrite PackHeaderPosition.
// So we record PackHeaderPosition to CurrentPackHeaderPosition as the
// first thing we do in Fill_Next().
//////////

// Decode headers up to a picture header and then return.
// There are two modes of operation. Normally, we return only
// when we get a picture header and if we hit EOF file, we
// thread kill ourselves in here. But when checking for leading
// B frames we don't want to kill in order to properly handle
// files with only one I frame. So in the second mode, we detect
// the sequence end code and return with an indication.

int Get_Hdr(int mode)
{
	int code;
	__int64 position = 0;
	boolean HadSequenceHeader = false;
	boolean HadGopHeader = false;

	for (;;)
	{
		// Look for next_start_code.
		if (Stop_Flag == true)
			return 1;
		next_start_code();

		code = Show_Bits(32);
		switch (code)
		{
            case 0x1be:
                break;

			case SEQUENCE_HEADER_CODE:
				// Index the location of the sequence header for the D2V file.
				// We prefer to index the sequence header corresponding to this
				// GOP, but if one doesn't exist, we index the picture header of the I frame.
				if (SystemStream_Flag != ELEMENTARY_STREAM)
					d2v_current.position = CurrentPackHeaderPosition;
				else
				{
//					dprintf("DGIndex: Index sequence header at %d\n", Rdptr - 8 + (32 - BitsLeft)/8);
					d2v_current.position = _ftelli64(Infile[CurrentFile])
										   - (BUFFER_SIZE - (Rdptr - Rdbfr))
										   - 8
										   + (32 - BitsLeft)/8;
				}
				Get_Bits(32);
				sequence_header();
				HadSequenceHeader = true;
				break;

#if 0
			case SEQUENCE_END_CODE:
				Get_Bits(32);
				if (mode == 1)
					return 1;
				break;
#endif

			case GROUP_START_CODE:
				Get_Bits(32);
				group_of_pictures_header();
				HadGopHeader = true;
				GOPSeen = true;
				break;

			case PICTURE_START_CODE:
				if (SystemStream_Flag != ELEMENTARY_STREAM)
					position = CurrentPackHeaderPosition;
				else
					position = _ftelli64(Infile[CurrentFile])
										   - (BUFFER_SIZE - (Rdptr - Rdbfr))
										   - 8
										   + (32 - BitsLeft)/8;
				Get_Bits(32);
				picture_header(position, HadSequenceHeader, HadGopHeader);
				return 0;
				break;

			default:
				Get_Bits(32);
				break;
		}
	}
}

/* decode sequence header */
void sequence_header()
{
	int constrained_parameters_flag;
	int bit_rate_value;
	int vbv_buffer_size;
	int i;

    // These will become nonzero if we receive a sequence display extension
	display_horizontal_size = 0;
	display_vertical_size = 0;

	horizontal_size             = Get_Bits(12);
	vertical_size               = Get_Bits(12);
	aspect_ratio_information    = Get_Bits(4);
	frame_rate_code             = Get_Bits(4);

	// This is default MPEG1 handling.
	// It may be overridden to MPEG2 if a
	// sequence extension arrives.
	frame_rate = frame_rate_Table[frame_rate_code];
	fr_num = frame_rate_Table_Num[frame_rate_code];
	fr_den = frame_rate_Table_Den[frame_rate_code];

	bit_rate_value              = Get_Bits(18);
	Flush_Buffer(1);	// marker bit
	vbv_buffer_size             = Get_Bits(10);
	constrained_parameters_flag = Get_Bits(1);

	if ((load_intra_quantizer_matrix = Get_Bits(1)) && (Show_Bits(8) == 8))
	{
		for (i=0; i<64; i++)
			intra_quantizer_matrix[scan[ZIG_ZAG][i]] = Get_Bits(8);
	}
	else
	{
		for (i=0; i<64; i++)
			intra_quantizer_matrix[i] = default_intra_quantizer_matrix[i];
	}

	if ((load_non_intra_quantizer_matrix = Get_Bits(1)) && (Show_Bits(8) != 0))
	{
		for (i=0; i<64; i++)
			non_intra_quantizer_matrix[scan[ZIG_ZAG][i]] = Get_Bits(8);
	}
	else
	{
		for (i=0; i<64; i++)
			non_intra_quantizer_matrix[i] = 16;
	}

	/* copy luminance to chrominance matrices */
	for (i=0; i<64; i++)
	{
		chroma_intra_quantizer_matrix[i] = intra_quantizer_matrix[i];
		chroma_non_intra_quantizer_matrix[i] = non_intra_quantizer_matrix[i];
	}

	if (D2V_Flag && LogQuants_Flag)
	{
		// Log the quant matrix changes.
		// Intra luma.
		for (i=0; i<64; i++)
		{
			if (intra_quantizer_matrix[i] != intra_quantizer_matrix_log[i])
				break;
		}
		if (i < 64)
		{
			// The matrix changed, so log it.
			fprintf(Quants, "Intra Luma and Chroma Matrix at encoded frame %d:\n", Frame_Number);
			for (i=0; i<64; i++)
			{
				fprintf(Quants, "%d ", intra_quantizer_matrix[i]);
				if ((i % 8) == 7)
					fprintf(Quants, "\n");
			}
			fprintf(Quants, "\n");
			// Record the new matrix for change detection.
			memcpy(intra_quantizer_matrix_log, intra_quantizer_matrix, sizeof(intra_quantizer_matrix));
			memcpy(chroma_intra_quantizer_matrix_log, chroma_intra_quantizer_matrix, sizeof(chroma_intra_quantizer_matrix));
		}
		// Non intra luma.
		for (i=0; i<64; i++)
		{
			if (non_intra_quantizer_matrix[i] != non_intra_quantizer_matrix_log[i])
				break;
		}
		if (i < 64)
		{
			// The matrix changed, so log it.
			fprintf(Quants, "NonIntra Luma and Chroma Matrix at encoded frame %d:\n", Frame_Number);
			for (i=0; i<64; i++)
			{
				fprintf(Quants, "%d ", non_intra_quantizer_matrix[i]);
				if ((i % 8) == 7)
					fprintf(Quants, "\n");
			}
			fprintf(Quants, "\n");
			// Record the new matrix for change detection.
			memcpy(non_intra_quantizer_matrix_log, non_intra_quantizer_matrix, sizeof(non_intra_quantizer_matrix));
			memcpy(chroma_non_intra_quantizer_matrix_log, chroma_non_intra_quantizer_matrix, sizeof(chroma_non_intra_quantizer_matrix));
		}
	}

	// This is default MPEG1 handling.
	// It may be overridden to MPEG2 if a
	// sequence extension arrives.
	matrix_coefficients = 5;
    default_matrix_coefficients = true;

	setRGBValues();
	// These are MPEG1 defaults. These will be overridden if we have MPEG2
	// when the sequence header extension is parsed.
	progressive_sequence = 1;
	chroma_format = CHROMA420;

	extension_and_user_data();

	// Special case for 1080i video.
	if (vertical_size == 1088)
	{
        if (CLIActive)
        {
            crop1088_warned = true;
            crop1088 = true;
        }
		if (crop1088_warned == false)
		{
			char buf[255];
			sprintf(buf, "Your stream specifies a display height of 1088.\n"
				"This is sometimes an encoding mistake and the last 8 lines are garbage.\n"
				"Do you want to treat it as if it specified a height of 1080?");
			if (MessageBox(hWnd, buf, "Display Height 1088 Warning", MB_YESNO | MB_ICONINFORMATION) == IDYES)
				crop1088 = true;
			else
				crop1088 = false;
			crop1088_warned = true;
		}
		if (crop1088 == true)
			vertical_size = 1080;
	}
}

/* decode group of pictures header */
/* ISO/IEC 13818-2 section 6.2.2.6 */
static void group_of_pictures_header()
{
	int gop_hour;
	int gop_minute;
	int gop_sec;
	int gop_frame;
	int drop_flag;
	int broken_link;

	if (LogTimestamps_Flag && D2V_Flag && StartLogging_Flag)
		fprintf(Timestamps, "GOP start\n");
	drop_flag   = Get_Bits(1);
	gop_hour    = Get_Bits(5);
	gop_minute  = Get_Bits(6);
	Flush_Buffer(1);	// marker bit
	gop_sec     = Get_Bits(6);
	gop_frame	= Get_Bits(6);
	closed_gop  = Get_Bits(1);
	broken_link = Get_Bits(1);

//	extension_and_user_data();
}

/* decode picture header */
/* ISO/IEC 13818-2 section 6.2.3 */
static void picture_header(__int64 start, boolean HadSequenceHeader, boolean HadGopHeader)
{
	int vbv_delay;
	int Extra_Information_Byte_Count;
	int trackpos;
	double track;

	temporal_reference  = Get_Bits(10);
	picture_coding_type = Get_Bits(3);
	if (LogTimestamps_Flag && D2V_Flag && StartLogging_Flag)
	{
		fprintf(Timestamps, "Decode picture: temporal reference %d", temporal_reference);
		switch (picture_coding_type)
		{
		case I_TYPE:
			fprintf(Timestamps, "[I]\n");
			break;
		case P_TYPE:
			fprintf(Timestamps, "[P]\n");
			break;
		case B_TYPE:
			fprintf(Timestamps, "[B]\n");
			break;
		}
	}

	d2v_current.type = picture_coding_type;

	if (d2v_current.type == I_TYPE)
	{
		d2v_current.file = process.startfile = CurrentFile;
		process.startloc = _ftelli64(Infile[CurrentFile]);
		d2v_current.lba = process.startloc/SECTOR_SIZE - 1;
		if (d2v_current.lba < 0)
		{
			d2v_current.lba = 0;
		}
		track = (double) d2v_current.lba;
		track *= SECTOR_SIZE;
		track += process.run;
		track *= TRACK_PITCH;
		track /= Infiletotal;
		trackpos = (int) track;
		SendMessage(hTrack, TBM_SETPOS, (WPARAM)true, trackpos);
    	InvalidateRect(hwndSelect, NULL, TRUE);
#if 0
		{
			char buf[80];
			char total[80], run[80];
			_i64toa(Infiletotal, total, 10);
			_i64toa(process.run, run, 10);
			sprintf(buf, "DGIndex: d2v_current.lba = %x\n", d2v_current.lba);
			OutputDebugString(buf);
			sprintf(buf, "DGIndex: trackpos = %d\n", trackpos);
			OutputDebugString(buf);
			sprintf(buf, "DGIndex: process.run = %s\n", run);
			OutputDebugString(buf);
			sprintf(buf, "DGIndex: Infiletotal = %s\n", total);
			OutputDebugString(buf);
		}
#endif

		if (process.locate==LOCATE_RIP)
		{
			process.file = d2v_current.file;
			process.lba = d2v_current.lba;
		}

		// This triggers if we reach the right marker position.
		if (CurrentFile==process.endfile && process.startloc>=process.endloc)		// D2V END
		{
			ThreadKill(END_OF_DATA_KILL);
		}
	}

	if (!D2V_Flag || !(Frame_Number % 1000))
	{
		if (Info_Flag)
			UpdateInfo();
		UpdateWindowText();
	}

	vbv_delay = Get_Bits(16);

	if (picture_coding_type == P_TYPE || picture_coding_type == B_TYPE)
	{
		full_pel_forward_vector = Get_Bits(1);
		forward_f_code = Get_Bits(3);
	}

	if (picture_coding_type == B_TYPE)
	{
		full_pel_backward_vector = Get_Bits(1);
		backward_f_code = Get_Bits(3);
	}

	// MPEG1 defaults. May be overriden by picture coding extension.
	intra_dc_precision = 0;
	picture_structure = FRAME_PICTURE;
	top_field_first = 1;
	frame_pred_frame_dct = 1;
	concealment_motion_vectors = 0;
	q_scale_type = 0;
	intra_vlc_format = 0;
	alternate_scan = 0;
	repeat_first_field = 0;
	progressive_frame = 1;

	d2v_current.pf = progressive_frame;
	d2v_current.trf = (top_field_first<<1) + repeat_first_field;

	Extra_Information_Byte_Count = extra_bit_information();
	extension_and_user_data();

    // We prefer to index the sequence header, but if one doesn't exist,
	// we index the picture header of the I frame.
	if (HadSequenceHeader == false)
	{
		// Indexing for the D2V file.
		if (picture_coding_type == I_TYPE && !Second_Field)
		{
//			dprintf("DGIndex: Index picture header at %d\n", Rdptr - Rdbfr);
			d2v_current.position = start;
		}
	}
}

/* decode slice header */
/* ISO/IEC 13818-2 section 6.2.4 */
int slice_header()
{
	int slice_vertical_position_extension;
	int quantizer_scale_code;

	if (mpeg_type == IS_MPEG2)
		slice_vertical_position_extension = vertical_size>2800 ? Get_Bits(3) : 0;
	else
		slice_vertical_position_extension = 0;

	quantizer_scale_code = Get_Bits(5);
	if (mpeg_type == IS_MPEG2)
		quantizer_scale = q_scale_type ? Non_Linear_quantizer_scale[quantizer_scale_code] : quantizer_scale_code<<1;
	else
		quantizer_scale = quantizer_scale_code;

	while (Get_Bits(1)) Flush_Buffer(8);

	return slice_vertical_position_extension;
}

/* decode extension and user data */
/* ISO/IEC 13818-2 section 6.2.2.2 */
static void extension_and_user_data()
{
	int code, ext_ID;

	if (Stop_Flag == true)
		return;
	next_start_code();

	while ((code = Show_Bits(32))==EXTENSION_START_CODE || code==USER_DATA_START_CODE)
	{
		if (code==EXTENSION_START_CODE)
		{
			Flush_Buffer(32);
			ext_ID = Get_Bits(4);

			switch (ext_ID)
			{
				case SEQUENCE_EXTENSION_ID:
					sequence_extension();
					break;
				case SEQUENCE_DISPLAY_EXTENSION_ID:
					sequence_display_extension();
					break;
				case QUANT_MATRIX_EXTENSION_ID:
					quant_matrix_extension();
					break;
				case PICTURE_DISPLAY_EXTENSION_ID:
					picture_display_extension();
					break;
				case PICTURE_CODING_EXTENSION_ID:
					picture_coding_extension();
					break;
				case COPYRIGHT_EXTENSION_ID:
					copyright_extension();
					break;
			}
			if (Stop_Flag == true)
				return;
			next_start_code();
		}
		else
		{
			Flush_Buffer(32);	// ISO/IEC 13818-2  sections 6.3.4.1 and 6.2.2.2.2
			if (Stop_Flag == true)
				return;
			next_start_code();	// skip user data
		}
	}
}

/* decode sequence extension */
/* ISO/IEC 13818-2 section 6.2.2.3 */
int profile_and_level_indication;
static void sequence_extension()
{
	int low_delay;
	int frame_rate_extension_n;
	int frame_rate_extension_d;
	int horizontal_size_extension;
	int vertical_size_extension;
	int bit_rate_extension;
	int vbv_buffer_size_extension;

	// This extension means we must have MPEG2, so
	// override the earlier assumption of MPEG1 for
	// transport streams.
	mpeg_type = IS_MPEG2;
   	setRGBValues();

	profile_and_level_indication = Get_Bits(8);
	progressive_sequence         = Get_Bits(1);
	chroma_format                = Get_Bits(2);
	horizontal_size_extension    = Get_Bits(2);
	vertical_size_extension      = Get_Bits(2);
	bit_rate_extension           = Get_Bits(12);
	Flush_Buffer(1);	// marker bit
	vbv_buffer_size_extension    = Get_Bits(8);
	low_delay                    = Get_Bits(1);
 
	frame_rate_extension_n       = Get_Bits(2);
	frame_rate_extension_d       = Get_Bits(5);
	frame_rate = frame_rate_Table[frame_rate_code] * (frame_rate_extension_n+1)/(frame_rate_extension_d+1);
	fr_num = frame_rate_Table_Num[frame_rate_code] * (frame_rate_extension_n+1);
	fr_den = frame_rate_Table_Den[frame_rate_code] * (frame_rate_extension_d+1);

	horizontal_size = (horizontal_size_extension<<12) | (horizontal_size&0x0fff);
	vertical_size = (vertical_size_extension<<12) | (vertical_size&0x0fff);
    // This is the default case and may be overridden by a sequence display extension.
    if (horizontal_size > 720 || vertical_size > 576)
        // HD
	    matrix_coefficients = 1;
    else
        // SD
       	matrix_coefficients = 5;
}

/* decode sequence display extension */
static void sequence_display_extension()
{
	int video_format;  
	int color_description;
	int color_primaries;
	int transfer_characteristics;
    int matrix;

	video_format      = Get_Bits(3);
	color_description = Get_Bits(1);

	if (color_description)
	{
		color_primaries          = Get_Bits(8);
		transfer_characteristics = Get_Bits(8);
		matrix = Get_Bits(8);
        // If the stream specifies "reserved" or "unspecified" then leave things set to our default
        // based on HD versus SD.
        if (matrix == 1 || (matrix >= 4 && matrix <= 7))
        {
		    matrix_coefficients  = matrix;
            default_matrix_coefficients = false;
        }
		setRGBValues();
	}

	display_horizontal_size = Get_Bits(14);
	Flush_Buffer(1);	// marker bit
	display_vertical_size   = Get_Bits(14);
}

/* decode quant matrix entension */
/* ISO/IEC 13818-2 section 6.2.3.2 */
static void quant_matrix_extension()
{
	int i;

	if (load_intra_quantizer_matrix = Get_Bits(1))
		for (i=0; i<64; i++)
			chroma_intra_quantizer_matrix[scan[ZIG_ZAG][i]]
				= intra_quantizer_matrix[scan[ZIG_ZAG][i]] = Get_Bits(8);

	if (load_non_intra_quantizer_matrix = Get_Bits(1))
		for (i=0; i<64; i++)
			chroma_non_intra_quantizer_matrix[scan[ZIG_ZAG][i]]
				= non_intra_quantizer_matrix[scan[ZIG_ZAG][i]] = Get_Bits(8);

	if (load_chroma_intra_quantizer_matrix = Get_Bits(1))
		for (i=0; i<64; i++)
			chroma_intra_quantizer_matrix[scan[ZIG_ZAG][i]] = Get_Bits(8);

	if (load_chroma_non_intra_quantizer_matrix = Get_Bits(1))
		for (i=0; i<64; i++)
			chroma_non_intra_quantizer_matrix[scan[ZIG_ZAG][i]] = Get_Bits(8);

	if (D2V_Flag && LogQuants_Flag)
	{
		// Log the quant matrix changes.
		// Intra luma.
		for (i=0; i<64; i++)
		{
			if (intra_quantizer_matrix[i] != intra_quantizer_matrix_log[i])
				break;
		}
		if (i < 64)
		{
			// The matrix changed, so log it.
			fprintf(Quants, "Intra Luma Matrix at encoded frame %d:\n", Frame_Number);
			for (i=0; i<64; i++)
			{
				fprintf(Quants, "%d ", intra_quantizer_matrix[i]);
				if ((i % 8) == 7)
					fprintf(Quants, "\n");
			}
			fprintf(Quants, "\n");
			// Record the new matrix for change detection.
			memcpy(intra_quantizer_matrix_log, intra_quantizer_matrix, sizeof(intra_quantizer_matrix));
		}
		// Non intra luma.
		for (i=0; i<64; i++)
		{
			if (non_intra_quantizer_matrix[i] != non_intra_quantizer_matrix_log[i])
				break;
		}
		if (i < 64)
		{
			// The matrix changed, so log it.
			fprintf(Quants, "NonIntra Luma Matrix at encoded frame %d:\n", Frame_Number);
			for (i=0; i<64; i++)
			{
				fprintf(Quants, "%d ", non_intra_quantizer_matrix[i]);
				if ((i % 8) == 7)
					fprintf(Quants, "\n");
			}
			fprintf(Quants, "\n");
			// Record the new matrix for change detection.
			memcpy(non_intra_quantizer_matrix_log, non_intra_quantizer_matrix, sizeof(non_intra_quantizer_matrix));
		}
		// Intra chroma.
		for (i=0; i<64; i++)
		{
			if (chroma_intra_quantizer_matrix[i] != chroma_intra_quantizer_matrix_log[i])
				break;
		}
		if (i < 64)
		{
			// The matrix changed, so log it.
			fprintf(Quants, "Intra Chroma Matrix at encoded frame %d:\n", Frame_Number);
			for (i=0; i<64; i++)
			{
				fprintf(Quants, "%d ", chroma_intra_quantizer_matrix[i]);
				if ((i % 8) == 7)
					fprintf(Quants, "\n");
			}
			fprintf(Quants, "\n");
			// Record the new matrix for change detection.
			memcpy(chroma_intra_quantizer_matrix_log, chroma_intra_quantizer_matrix, sizeof(chroma_intra_quantizer_matrix));
		}
		// Non intra chroma.
		for (i=0; i<64; i++)
		{
			if (chroma_non_intra_quantizer_matrix[i] != chroma_non_intra_quantizer_matrix_log[i])
				break;
		}
		if (i < 64)
		{
			// The matrix changed, so log it.
			fprintf(Quants, "NonIntra Chroma Matrix at encoded frame %d:\n", Frame_Number);
			for (i=0; i<64; i++)
			{
				fprintf(Quants, "%d ", chroma_non_intra_quantizer_matrix[i]);
				if ((i % 8) == 7)
					fprintf(Quants, "\n");
			}
			fprintf(Quants, "\n");
			// Record the new matrix for change detection.
			memcpy(chroma_non_intra_quantizer_matrix_log, chroma_non_intra_quantizer_matrix, sizeof(chroma_non_intra_quantizer_matrix));
		}
	}
}

/* decode picture display extension */
/* ISO/IEC 13818-2 section 6.2.3.3. */
static void picture_display_extension()
{
	int frame_center_horizontal_offset[3];
	int frame_center_vertical_offset[3];

	int i;
	int number_of_frame_center_offsets;

	/* based on ISO/IEC 13818-2 section 6.3.12 
	   (November 1994) Picture display extensions */

	/* derive number_of_frame_center_offsets */
	if (progressive_sequence)
	{
		if (repeat_first_field)
		{
			if (top_field_first)
				number_of_frame_center_offsets = 3;
			else
				number_of_frame_center_offsets = 2;
		}
		else
			number_of_frame_center_offsets = 1;
	}
	else
	{
		if (picture_structure!=FRAME_PICTURE)
			number_of_frame_center_offsets = 1;
		else
		{
			if (repeat_first_field)
				number_of_frame_center_offsets = 3;
			else
				number_of_frame_center_offsets = 2;
		}
	}

	/* now parse */
	for (i=0; i<number_of_frame_center_offsets; i++)
	{
		frame_center_horizontal_offset[i] = Get_Bits(16);
		Flush_Buffer(1);	// marker bit

		frame_center_vertical_offset[i] = Get_Bits(16);
		Flush_Buffer(1);	// marker bit
	}
}

/* decode picture coding extension */
static void picture_coding_extension()
{
	int chroma_420_type;
	int composite_display_flag;
	int v_axis;
	int field_sequence;
	int sub_carrier;
	int burst_amplitude;
	int sub_carrier_phase;

	f_code[0][0] = Get_Bits(4);
	f_code[0][1] = Get_Bits(4);
	f_code[1][0] = Get_Bits(4);
	f_code[1][1] = Get_Bits(4);

	intra_dc_precision			= Get_Bits(2);
	picture_structure			= Get_Bits(2);
	top_field_first				= Get_Bits(1);
    frame_pred_frame_dct		= Get_Bits(1);
	concealment_motion_vectors	= Get_Bits(1);
	q_scale_type				= Get_Bits(1);
	intra_vlc_format			= Get_Bits(1);
	alternate_scan				= Get_Bits(1);
	repeat_first_field			= Get_Bits(1);
#if 0
    // Not working right
	if (D2V_Flag && progressive_sequence && repeat_first_field)
	{
		if (FO_Flag == FO_FILM)
		{
			MessageBox(hWnd, "Frame repeats were detected in the source stream.\n"
							 "Proper handling of them requires that you turn off Force Film mode.\n"
							 "After turning it off, restart this operation.", NULL, MB_OK | MB_ICONERROR);
			ThreadKill(MISC_KILL);
		}
	}
#endif
	chroma_420_type				= Get_Bits(1);
	progressive_frame			= Get_Bits(1);
	composite_display_flag		= Get_Bits(1);

	d2v_current.pf = progressive_frame;
	d2v_current.trf = (top_field_first<<1) + repeat_first_field;
    // Store just the first picture structure encountered. We'll
    // use it to report the field order for field structure clips.
    if (d2v_current.picture_structure == -1)
	    d2v_current.picture_structure = picture_structure;

	if (composite_display_flag)
	{
		v_axis            = Get_Bits(1);
		field_sequence    = Get_Bits(3);
		sub_carrier       = Get_Bits(1);
		burst_amplitude   = Get_Bits(7);
		sub_carrier_phase = Get_Bits(8);
	}
}

/* decode extra bit information */
/* ISO/IEC 13818-2 section 6.2.3.4. */
static int extra_bit_information()
{
	int Byte_Count = 0;

	while (Get_Bits(1))
	{
        if (Stop_Flag)
            break;
		Flush_Buffer(8);
		Byte_Count++;
	}

	return Byte_Count;
}

/* Copyright extension */
/* ISO/IEC 13818-2 section 6.2.3.6. */
/* (header added in November, 1994 to the IS document) */
static void copyright_extension()
{
	int copyright_flag;
	int copyright_identifier;
	int original_or_copy;
	int copyright_number_1;
	int copyright_number_2;
	int copyright_number_3;

	int reserved_data;

	copyright_flag =       Get_Bits(1); 
	copyright_identifier = Get_Bits(8);
	original_or_copy =     Get_Bits(1);
  
	/* reserved */
	reserved_data = Get_Bits(7);

	Flush_Buffer(1); // marker bit
	copyright_number_1 =   Get_Bits(20);
	Flush_Buffer(1); // marker bit
	copyright_number_2 =   Get_Bits(22);
	Flush_Buffer(1); // marker bit
	copyright_number_3 =   Get_Bits(22);
}
