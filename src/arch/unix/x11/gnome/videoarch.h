/*
 * videoarch.h - GTK+ graphics routines.
 *
 * based on the X11 version written by
 *  Ettore Perazzoli
 *  Teemu Rantanen <tvr@cs.hut.fi>
 *  Andreas Boose <viceteam@t-online.de>
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

#ifndef _VIDEOARCH_H
#define _VIDEOARCH_H

#include "vice.h"

#include "uiarch.h"
#include "viewport.h"
#include "video.h"

#ifdef HAVE_HWSCALE
#include <GL/gl.h>
#endif

typedef void (*video_refresh_func_t)(struct video_canvas_s *,
              int, int, int, int, unsigned int, unsigned int);


struct video_canvas_s {
    unsigned int initialized;
    unsigned int created;
    GtkWidget* emuwindow;
    guchar *gdk_image;
    rectangle_t gdk_image_size;
    struct video_render_config_s *videoconfig;
    struct draw_buffer_s *draw_buffer;
    struct viewport_s *viewport;
    struct geometry_s *geometry;
    struct palette_s *palette;

    struct video_draw_buffer_callback_s *video_draw_buffer_callback;
#ifdef HAVE_FULLSCREEN
    struct fullscreenconfig_s *fullscreenconfig;
    video_refresh_func_t video_fullscreen_refresh_func;
#endif
#ifdef HAVE_HWSCALE
    GLint screen_texture;
#endif
};
typedef struct video_canvas_s video_canvas_t;

#endif
