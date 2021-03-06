/*
 * delaep7x8.h - Cartridge handling, Dela EP7x8 cart.
 *
 * Written by
 *  Marco van den Heuvel <blackystardust68@yahoo.com>
 *
 * This file is part of VICE, the Versatile Commodore Emulator.
 * See README for copyright notice.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307  USA.
 *
 */

#ifndef _DELAEP7x8_H
#define _DELAEP7x8_H

#include <stdio.h>

#include "types.h"

extern BYTE REGPARM1 delaep7x8_io1_read(WORD addr);
extern void REGPARM2 delaep7x8_io1_store(WORD addr, BYTE value);

extern void delaep7x8_config_init(void);
extern void delaep7x8_config_setup(BYTE *rawcart);
extern int delaep7x8_crt_attach(FILE *fd, BYTE *rawcart);
extern void delaep7x8_detach(void);

#endif
