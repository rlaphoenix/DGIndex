/* 
 *  Misc Stuff (profiling) for MPEG2Dec3
 *
 *	Copyright (C) 2002-2003 Marc Fauconneau <marc.fd@liberysurf.fr>
 *
 *  This file is part of MPEG2Dec3, a free MPEG-2 decoder
 *	
 *  MPEG2Dec3 is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  MPEG2Dec3 is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *
 */

#define ui64 unsigned __int64

struct ts_t
{
	ui64 idct;
	ui64 conv;
	ui64 mcpy;
	ui64 post;
	ui64 dec;
	ui64 bit;
	ui64 decMB;
	ui64 mcmp;
	ui64 addb;
	ui64 overall;
	ui64 sum;
	int div;
	ui64 freq;
};

typedef struct ts_t ts;

ui64 read_counter(void);
ui64 get_freq(void);
int dprintf(char* fmt, ...);

void init_first(ts* timers);
void init_timers(ts* timers);
void start_timer();
ui64 read_timer();
void start_timer2(ui64* timer);
void stop_timer(ui64* timer);
void stop_timer2(ui64* timer);
void timer_debug(ts* tim);
