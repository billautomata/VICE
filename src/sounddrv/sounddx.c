/*
 * sounddx.c - Implementation of the DirectSound sound device.
 *
 * Written by
 *  Tibor Biczo <crown@mail.matav.hu>
 *  Ettore Perazzoli <ettore@comm2000.it>
 *  Andreas Matthies <andreas.matthies@gmx.net>
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

#if __GNUC__>2 || (__GNUC__==2 && __GNUC_MINOR__>=95)
#include <windows.h>
#include <mmsystem.h>
#endif

#define DIRECTSOUND_VERSION 0x0500
#include <dsound.h>

#include "lib.h"
#include "sound.h"
#include "types.h"
#include "ui.h"

#ifndef HAVE_GUIDLIB
/* FIXME: It would be better to convert the dxguid.lib from DX5 into the
   Mingw32 port of DX5 */
const GUID IID_IDirectSoundNotify = {0xb0210783, 0x89cd, 0x11d0,
                                    {0xaf,0x08,0x00,0xa0,0xc9,0x25,0xcd,0x16}};
#endif

/* ------------------------------------------------------------------------ */

/* #define DEBUG_SOUND */

/* Debugging stuff.  */
#ifdef DEBUG_SOUND
static void sound_debug(const char *format, ...)
{
    char tmp[1024];
    va_list args;

    va_start(args, format);
    vsprintf(tmp, format, args);
    va_end(args);
    log_debug(tmp);
}
#define DEBUG(x) sound_debug x
#else
#define DEBUG(x)
#endif

static char *ds_error(HRESULT result)
{
    switch (result) {
      case DSERR_ALLOCATED:
        return "Already allocated resource";
      case DSERR_CONTROLUNAVAIL:
        return "Control not available";
      case DSERR_INVALIDPARAM:
        return "Parameter not valid";
      case DSERR_INVALIDCALL:
        return "Call not valid";
      case DSERR_GENERIC:
        return "Generic error";
      case DSERR_PRIOLEVELNEEDED:
        return "Priority level needed";
      case DSERR_OUTOFMEMORY:
        return "Out of memory";
      case DSERR_BADFORMAT:
        return "Specified WAVE format not supported";
      case DSERR_UNSUPPORTED:
        return "Not supported";
      case DSERR_NODRIVER:
        return "No sound driver is available for use";
      case DSERR_ALREADYINITIALIZED:
        return "Object already initialized";
      case DSERR_NOAGGREGATION:
        return "Object does not support aggregation";
      case DSERR_BUFFERLOST:
        return "Buffer lost";
      case DSERR_OTHERAPPHASPRIO:
        return "Another app has a higher priority level";
      case DSERR_UNINITIALIZED:
        return "Object not initialized";
      case DSERR_NOINTERFACE:
        return "Requested COM interface is not available";
      default:
        return "Whadda hell?!";
    }
}

/* ------------------------------------------------------------------------ */

/* DirectSound object.  */
static LPDIRECTSOUND ds=NULL;

/* Audio buffer.  */
static LPDIRECTSOUNDBUFFER buffer=NULL;
static LPDIRECTSOUNDBUFFER pbuffer=NULL;

/* Buffer offset.  */
static DWORD buffer_offset;

/* Buffer size.  */
static DWORD buffer_size;

/* Fragment size.  */
static int fragment_size;

/* Channels */
static int num_of_channels;

/* Flag: are we in exclusive mode?  */
/* static int is_exclusive; */

#if 0
/*  DirectSoundNotify Interface, if present */
static LPDIRECTSOUNDNOTIFY      notify;

typedef enum {
        STREAM_NOTIFY,
        STREAM_TIMER
} streammode_t;

/*  Flag: streaming mode */
static streammode_t         streammode=STREAM_TIMER;

/*  Notify Position Array */
static DSBPOSITIONNOTIFY    *notifypositions;

/*  Notify Event */
static HANDLE               notifyevent;

/*  End Event */
static HANDLE               endevent;

/*  Event Table */
static HANDLE               events[2];

/*  ID of Notify Thread */
static DWORD                notifyThreadID;

/*  Handle of Notify Thread */
static HANDLE               notifyThreadHandle;

/*  Pointer for waiting fragment */
static LPVOID               fragment_pointer;
#endif

/*  Last played sample. This will be played in underflow condition */
static SWORD                last_buffered_sample[2];

/*  Flag: is soundcard a 16bit or 8bit card? */
static int                  is16bit;

#if 0
/*  Streaming buffer */
static SWORD                *stream_buffer;

/*  Offset of first buffered sample */
static volatile int         stream_buffer_first;

/*  Offset of last buffered sample */
static volatile int         stream_buffer_last;

/*  Offset of first buffered sample in shadow counter */
static volatile DWORD       stream_buffer_shadow_first;

/*  Offset of last buffered sample in shadow counter */
static volatile DWORD       stream_buffer_shadow_last;
#endif

/*  Size of streaming buffer */
static int                  stream_buffer_size;

#if 0
/*  Timer callback interval */
static int                  timer_interval;

/*  ID of timer event */
static UINT                 timer_id;
#endif

/* ------------------------------------------------------------------------ */

DSBUFFERDESC desc;
PCMWAVEFORMAT pcmwf;
DSCAPS  capabilities;
WAVEFORMATEX    wfex;

static void dx_clear(void)
{
LPVOID  lpvPtr1;
DWORD   dwBytes1;
LPVOID  lpvPtr2;
DWORD   dwBytes2;
HRESULT result;

    result = IDirectSoundBuffer_Lock(buffer, 0, buffer_size,
                                     &lpvPtr1, &dwBytes1, &lpvPtr2,
                                     &dwBytes2, 0);
    if (result == DSERR_BUFFERLOST) {
        IDirectSoundBuffer_Restore(buffer);
    } else {
        if (is16bit) {
            memset(lpvPtr1, 0, dwBytes1);
            if (lpvPtr2) memset(lpvPtr2, 0, dwBytes2);
        } else {
            memset(lpvPtr1, 0x80, dwBytes1);
            if (lpvPtr2) memset(lpvPtr2, 0x80, dwBytes2);
        }
        result = IDirectSoundBuffer_Unlock(buffer, lpvPtr1, dwBytes1,
                                           lpvPtr2, dwBytes2);
    }
}

static int dx_init(const char *param, int *speed, int *fragsize, int *fragnr,
                   int *channels)
{
HRESULT result;

    DEBUG(("DirectSound driver initialization: speed = %d, fragsize = %d, fragnr = %d, channels = %d\n",
           *speed, *fragsize, *fragnr, *channels));

    if (ds == NULL) {
        result = DirectSoundCreate(NULL, &ds, NULL);
        if (result != DS_OK) {
            ui_error("Cannot initialize DirectSound:\n%s", ds_error(result));
            return -1;
        }

        result = IDirectSound_SetCooperativeLevel(ds, ui_get_main_hwnd(),
                                                  DSSCL_EXCLUSIVE);
        if (result != DS_OK) {
            ui_error("Cannot set cooperative level:\n%s",
                     ds_error(result));
            return -1;
        }
    }

    memset(&capabilities, 0, sizeof(DSCAPS));
    capabilities.dwSize = sizeof(DSCAPS);

    IDirectSound_GetCaps(ds, &capabilities);
    if ((capabilities.dwFlags & DSCAPS_PRIMARY16BIT)
        || (capabilities.dwFlags & DSCAPS_SECONDARY16BIT)) {
        is16bit = 1;
    } else {
        is16bit = 0;
    }
    if (!((capabilities.dwFlags & DSCAPS_PRIMARYSTEREO)
        || (capabilities.dwFlags & DSCAPS_SECONDARYSTEREO))) {
        *channels = 1;
    }
    num_of_channels = *channels;

    DEBUG(("16bit flag: %d",is16bit));
    DEBUG(("Channels: %d",*channels));
    DEBUG(("Capabilities %08x",capabilities.dwFlags));
    DEBUG(("Secondary min Hz: %d",capabilities.dwMinSecondarySampleRate));
    DEBUG(("Secondary max Hz: %d",capabilities.dwMaxSecondarySampleRate));

    memset(&pcmwf, 0, sizeof(PCMWAVEFORMAT));
    pcmwf.wf.wFormatTag = WAVE_FORMAT_PCM;
    pcmwf.wf.nChannels = *channels;
    pcmwf.wf.nSamplesPerSec = *speed;
    pcmwf.wBitsPerSample = is16bit ? 16 : 8;
/* Hack to fix if mmsystem header is bad
    ((WORD*)&pcmwf)[7] = 16;
*/
    pcmwf.wf.nBlockAlign = (is16bit ? 2 : 1) * *channels;
    pcmwf.wf.nAvgBytesPerSec = pcmwf.wf.nSamplesPerSec * pcmwf.wf.nBlockAlign;

    memset(&desc, 0, sizeof(DSBUFFERDESC));
    desc.dwSize = sizeof(DSBUFFERDESC);
    desc.dwFlags = DSBCAPS_PRIMARYBUFFER;

    fragment_size = *fragsize;

    buffer_size = *fragsize * *fragnr * (is16bit ? sizeof(SWORD) : 1)
                  * *channels;

    result = IDirectSound_CreateSoundBuffer(ds, &desc, &pbuffer, NULL);

    if (result != DS_OK) {
        ui_error("Cannot create Primary DirectSound bufer: %s",
                 ds_error(result));
        return -1;
    }

    memset(&desc, 0, sizeof(DSBUFFERDESC));
    desc.dwSize = sizeof(DSBUFFERDESC);
    desc.dwFlags = DSBCAPS_CTRLPOSITIONNOTIFY | DSBCAPS_GETCURRENTPOSITION2
                   | DSBCAPS_CTRLFREQUENCY | DSBCAPS_CTRLPAN
                   | DSBCAPS_CTRLVOLUME | DSBCAPS_GLOBALFOCUS ;

    desc.dwBufferBytes = buffer_size;
    desc.lpwfxFormat = (LPWAVEFORMATEX)&pcmwf;

    stream_buffer_size = fragment_size * *fragnr * *channels;

    result = IDirectSound_CreateSoundBuffer(ds, &desc, &buffer, NULL);
    if (result != DS_OK) {
        ui_error("Cannot create DirectSound buffer:\n%s", ds_error(result));
        return -1;
    }

    memset(&wfex, 0, sizeof(WAVEFORMATEX));
    wfex.wFormatTag = WAVE_FORMAT_PCM;
    wfex.nChannels = *channels;
    wfex.nSamplesPerSec = *speed;
    wfex.wBitsPerSample = is16bit ? 16 : 8;
    wfex.nBlockAlign = (is16bit ? 2 : 1) * *channels;
    wfex.nAvgBytesPerSec = wfex.nSamplesPerSec * wfex.nBlockAlign;

    result=IDirectSoundBuffer_SetFormat(pbuffer, &wfex);
    if (result != DS_OK) {
        ui_error("Cannot set Output format for primary sound buffer:\n%s",
                 ds_error(result));
        return -1;
    }

    buffer_offset= *fragsize * (is16bit ? sizeof(SWORD) : 1) * (*fragnr - 1)
                   * *channels;
    dx_clear();
    /* Let's go...  */
    result = IDirectSoundBuffer_Play(buffer, 0, 0, DSBPLAY_LOOPING);
    if (result == DSERR_BUFFERLOST) {
        ui_error("Restoring DirectSound buffer.");
        if ((result = IDirectSoundBuffer_Restore(buffer)) != DS_OK)
            ui_error("Cannot restore buffer:\n%s", ds_error(result));
        result = IDirectSoundBuffer_Play(buffer, 0, 0, DSBPLAY_LOOPING);
    }
    if (result != DS_OK) {
        ui_error("Cannot play DirectSound buffer:\n%s", ds_error(result));
        return -1;
    }

    DEBUG(("DirectSound initialization done succesfully.\n"));

    return 0;
}

static void dx_close(void)
{
    /*  Stop buffer play */
    if (ds == NULL)
        return;

    IDirectSoundBuffer_Stop(buffer);

    /*  Release buffer */
    IDirectSoundBuffer_Release(buffer);
    /*  Release DirectSoundObject */
    IDirectSound_Release(ds);
    buffer = NULL;
    ds = NULL;
}

static int dx_bufferspace(void)
{
    DWORD play_cursor, write_cursor;
    int free_samples;

    IDirectSoundBuffer_GetCurrentPosition(buffer, &play_cursor, &write_cursor);
    if (play_cursor <= buffer_offset)
        free_samples = stream_buffer_size - (buffer_offset - play_cursor)
                       / (is16bit ? 2 : 1);
    else
        free_samples = (play_cursor - buffer_offset) / (is16bit ? 2 : 1);

    return free_samples / num_of_channels;
}

static int dx_write(SWORD *pbuf, size_t nr)
{
    LPVOID lpvPtr1;
    DWORD dwBytes1;
    LPVOID lpvPtr2;
    DWORD dwBytes2;
    HRESULT result;
    DWORD buffer_lock_size; /* buffer_lock_end; */
    unsigned int i, count;

    /* XXX: Assumes `nr' is multiple of `fragment_size'.  */
    count = nr / fragment_size;
    buffer_lock_size = fragment_size * (is16bit ? sizeof(SWORD) : 1);

    /* Write one fragment at a time.  FIXME: This could be faster.  */
    for (i = 0; i < count; i++) {
        do {
            result = IDirectSoundBuffer_Lock(buffer, buffer_offset,
                                             buffer_lock_size,
                                             &lpvPtr1, &dwBytes1, &lpvPtr2,
                                             &dwBytes2, 0);

            if (result == DSERR_BUFFERLOST) {
                IDirectSoundBuffer_Restore(buffer);
                result = IDirectSoundBuffer_Lock(buffer, buffer_offset,
                                                 buffer_lock_size, &lpvPtr1,
                                                 &dwBytes1,
                                                 &lpvPtr2, &dwBytes2, 0);
            }
        } while ((dwBytes1 + dwBytes2) < buffer_lock_size);
        if (is16bit) {
            memcpy(lpvPtr1,pbuf,dwBytes1);
            if (lpvPtr2)
                memcpy(lpvPtr2,(BYTE *)pbuf + dwBytes1, dwBytes2);
        } else {
            SWORD *copyptr = pbuf;
            for (i = 0; i < dwBytes1; i++) {
                ((BYTE *)lpvPtr1)[i]=(*(copyptr++) >> 8) + 0x80;
            }
            if (lpvPtr2 != NULL) {
                for (i = 0; i < dwBytes2; i++) {
                    ((BYTE *)lpvPtr2)[i]=(*(copyptr++) >> 8) + 0x80;
                }
            }
        }

        result = IDirectSoundBuffer_Unlock(buffer, lpvPtr1, dwBytes1,
                                           lpvPtr2, dwBytes2);
        buffer_offset += buffer_lock_size;

        if (buffer_offset >= buffer_size)
            buffer_offset = 0;

        pbuf += fragment_size;
    }

    pbuf -= num_of_channels;

    for (i = 0; i < (unsigned int)num_of_channels; i++) {
        last_buffered_sample[i] = *pbuf++;
    }

    return 0;
}

static int dx_suspend(void)
{
    int c, i;
    SWORD *p = (SWORD *)lib_malloc(stream_buffer_size * sizeof(SWORD));

    if (!p)
        return 0;

    for (c = 0; c < num_of_channels; c++) {
        for (i = 0; i < stream_buffer_size/num_of_channels; i++) 
            p[i * num_of_channels + c] = last_buffered_sample[c];
    }

    i = dx_write(p, stream_buffer_size);
    lib_free(p);

    return 0;
}

static int dx_resume(void)
{
    buffer_offset = buffer_size - fragment_size;
    IDirectSoundBuffer_Play(buffer, 0, 0, DSBPLAY_LOOPING);

    return 0;
}



static sound_device_t dx_device =
{
    "dx",
    dx_init,
    dx_write,
    NULL,
    NULL,
    dx_bufferspace,
    dx_close,
    dx_suspend,
    dx_resume,
    0
};

int sound_init_dx_device(void)
{
    return sound_register_device(&dx_device);
}

