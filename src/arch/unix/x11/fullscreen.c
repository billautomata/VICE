/*
 * fullscreen.c
 *
 * Written by
 *  Andreas Boose <viceteam@t-online.de>
 *  Martin Pottendorfer <pottendo@utanet.at>
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

#include "vice.h"

#include <stdio.h>
#include <string.h>

#include "fullscreen.h"
#include "fullscreenarch.h"
#include "lib.h"
#include "video.h"
#include "videoarch.h"


#if defined  (USE_XF86_EXTENSIONS) && defined (HAVE_FULLSCREEN)

#define STR_VIDMODE "Vidmode"
#define STR_XRANDR  "XRANDR"

#ifdef USE_XF86_VIDMODE_EXT
#include "vidmode.h"
#endif
#ifdef HAVE_XRANDR
#include "xrandr.h"
#endif

int fullscreen_is_enabled;

int fullscreen_available(void) 
{
#ifdef USE_XF86_VIDMODE_EXT
    if (vidmode_available())
        return 1;
#endif
    return 0;
}

void fullscreen_shutdown(void)
{
#ifdef USE_XF86_VIDMODE_EXT
    vidmode_shutdown();
#endif
#ifdef HAVE_XRANDR
    xrandr_shutdown();
#endif
}

void fullscreen_suspend(int level)
{
#ifdef USE_XF86_VIDMODE_EXT
    vidmode_suspend(level);
#endif
}

void fullscreen_resume(void)
{
#ifdef USE_XF86_VIDMODE_EXT
    vidmode_resume();
#endif
}

void fullscreen_set_mouse_timeout(void)
{
#ifdef USE_XF86_VIDMODE_EXT
    vidmode_set_mouse_timeout();
#endif
}

void fullscreen_mode_callback(const char *device, void *callback)
{
#ifdef USE_XF86_VIDMODE_EXT
    if (strcmp(STR_VIDMODE, device) == 0)
        vidmode_mode_callback(callback);
#endif
#ifdef HAVE_XRANDR
    if (strcmp(STR_XRANDR, device) == 0)
        xrandr_mode_callback(callback);
#endif
}

void fullscreen_menu_create(struct ui_menu_entry_s *menu)
{
#ifdef USE_XF86_VIDMODE_EXT
    vidmode_menu_create(menu);
#endif
#ifdef HAVE_XRANDR
    xrandr_menu_create(menu);
#endif
}

void fullscreen_menu_shutdown(struct ui_menu_entry_s *menu)
{
#ifdef USE_XF86_VIDMODE_EXT
    vidmode_menu_shutdown(menu);
#endif
#ifdef HAVE_XRANDR
    xrandr_menu_shutdown(menu);
#endif
}

int fullscreen_init(void)
{
#ifdef USE_XF86_VIDMODE_EXT
    if (vidmode_init() < 0)
        return -1;
#endif
#ifdef HAVE_XRANDR
    if (xrandr_init() < 0)
	return -1;
#endif
    return 0;
}

int fullscreen_init_alloc_hooks(struct video_canvas_s *canvas)
{
    return 0;
}

void fullscreen_shutdown_alloc_hooks(struct video_canvas_s *canvas)
{
}

static int fullscreen_statusbar(struct video_canvas_s *canvas, int enable)
{
    if (!fullscreen_is_enabled)
	return 0;
    return ui_fullscreen_statusbar(canvas, enable);
}

static int fullscreen_enable(struct video_canvas_s *canvas, int enable)
{
    if (canvas->fullscreenconfig->device == NULL)
	return 0;
    
#ifdef USE_XF86_VIDMODE_EXT
    if (strcmp(STR_VIDMODE, canvas->fullscreenconfig->device) == 0)
        if (vidmode_enable(canvas, enable) < 0)
            return -1;
#endif
#ifdef HAVE_XRANDR
    if (strcmp(STR_XRANDR, canvas->fullscreenconfig->device) == 0)
        if (xrandr_enable(canvas, enable) < 0)
            return -1;
#endif
    fullscreen_is_enabled = canvas->fullscreenconfig->enable = enable;
    return 0;
}

static int fullscreen_double_size(struct video_canvas_s *canvas,
                                  int double_size)
{
    canvas->fullscreenconfig->double_size = double_size;
    return 0;
}

static int fullscreen_double_scan(struct video_canvas_s *canvas,
                                  int double_scan)
{
    canvas->fullscreenconfig->double_scan = double_scan;
    return 0;
}

static int fullscreen_device(struct video_canvas_s *canvas, const char *device)
{
    while (1) {
#ifdef USE_XF86_VIDMODE_EXT
    if (strcmp(STR_VIDMODE, device) == 0)
        break;
#endif
#ifdef HAVE_XRANDR
    if (strcmp(STR_XRANDR, device) == 0)
        break;
#endif
        return -1;
    }

    if (canvas->fullscreenconfig->device)
	lib_free(canvas->fullscreenconfig->device);
    
    canvas->fullscreenconfig->device = lib_stralloc(device);

    return 0;
}

#ifdef USE_XF86_VIDMODE_EXT
static int fullscreen_mode_vidmode(struct video_canvas_s *canvas, int mode)
{
    vidmode_mode(canvas, mode);
    return 0;
}
#endif

#ifdef HAVE_XRANDR
static int fullscreen_mode_xrandr(struct video_canvas_s *canvas, int mode)
{
    xrandr_mode(canvas, mode);
    return 0;
}
#endif

#endif	/* USE_XF86_EXTENSIONS && HAVE_FULLSCREEN */

void fullscreen_capability(cap_fullscreen_t *cap_fullscreen)
{
    cap_fullscreen->device_num = 0;

#ifdef USE_XF86_VIDMODE_EXT
    cap_fullscreen->device_name[cap_fullscreen->device_num] = STR_VIDMODE;
    cap_fullscreen->enable = fullscreen_enable;
    cap_fullscreen->statusbar = fullscreen_statusbar;
    cap_fullscreen->double_size = fullscreen_double_size;
    cap_fullscreen->double_scan = fullscreen_double_scan;
    cap_fullscreen->device = fullscreen_device;
    cap_fullscreen->mode[cap_fullscreen->device_num] = fullscreen_mode_vidmode;
    cap_fullscreen->device_num += 1;
#endif
#ifdef HAVE_XRANDR
    cap_fullscreen->device_name[cap_fullscreen->device_num] = STR_XRANDR;
    cap_fullscreen->enable = fullscreen_enable;
    cap_fullscreen->statusbar = fullscreen_statusbar;
    cap_fullscreen->double_size = fullscreen_double_size;
    cap_fullscreen->double_scan = fullscreen_double_scan;
    cap_fullscreen->device = fullscreen_device;
    cap_fullscreen->mode[cap_fullscreen->device_num] = fullscreen_mode_xrandr;
    cap_fullscreen->device_num += 1;
#endif
}
