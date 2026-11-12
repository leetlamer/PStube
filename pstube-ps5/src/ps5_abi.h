/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2024 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/
// Altered adaptation for PStube's platform probe, from SDL PS5 ee4c47dc0d617b3bc8f35108f9956baf228a1322.
#pragma once
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/event.h>
extern "C" {
typedef struct PS5_VideoBuf {
    void *data;
    uint64_t junk0[3];
} PS5_VideoBuf;

typedef struct PS5_VideoAttr {
    uint8_t junk0[80];
} PS5_VideoAttr;

int sceSystemServiceHideSplashScreen(void);

int sceKernelAllocateMainDirectMemory(size_t, size_t, int, intptr_t*);
int sceKernelMapDirectMemory(void**, size_t, int, int, intptr_t, size_t);
int sceKernelReleaseDirectMemory(intptr_t, size_t);

int sceKernelCreateEqueue(struct kevent **, const char *);
int sceKernelWaitEqueue(struct kevent*, struct kevent*, int, int*, unsigned int*);
int sceKernelDeleteEqueue(struct kevent*);

int sceVideoOutOpen(int, int, int, const void*);
void sceVideoOutClose(int);

int sceVideoOutAddFlipEvent(struct kevent*, int, void*);
int sceVideoOutSetFlipRate(int, int);
int sceVideoOutSubmitFlip(int, int, uint32_t, int64_t);
int sceVideoOutDeleteFlipEvent(struct kevent*, int);

void sceVideoOutSetBufferAttribute2(PS5_VideoAttr*, uint64_t, uint32_t, uint32_t,
                                    uint32_t, uint64_t, uint32_t, uint64_t);
int sceVideoOutRegisterBuffers2(int, int, int, PS5_VideoBuf*, int, PS5_VideoAttr*,
                                int, void*);

int32_t sceAudioOutInit(void);
int32_t sceAudioOutOpen(int32_t userId, int32_t type, int32_t index,
                        uint32_t len, uint32_t freq, uint32_t param);
int32_t sceAudioOutOutput(int32_t handle, const void *p);
int32_t sceAudioOutClose(int32_t handle);






#define PS5_PAD_PORT_TYPE_STANDARD       0
#define PS5_PAD_PORT_TYPE_REMOTE_CONTROL 16

#define PS5_USER_ID_SYSTEM 0xff

#define PS5_PAD_ERROR_ALREADY_OPENED 0x80920004u

#define PS5_PAD_BUTTON_L3        0x0002
#define PS5_PAD_BUTTON_R3        0x0004
#define PS5_PAD_BUTTON_OPTIONS   0x0008
#define PS5_PAD_BUTTON_UP        0x0010
#define PS5_PAD_BUTTON_RIGHT     0x0020
#define PS5_PAD_BUTTON_DOWN      0x0040
#define PS5_PAD_BUTTON_LEFT      0x0080
#define PS5_PAD_BUTTON_L2        0x0100
#define PS5_PAD_BUTTON_R2        0x0200
#define PS5_PAD_BUTTON_L1        0x0400
#define PS5_PAD_BUTTON_R1        0x0800
#define PS5_PAD_BUTTON_TRIANGLE  0x1000
#define PS5_PAD_BUTTON_CIRCLE    0x2000
#define PS5_PAD_BUTTON_CROSS     0x4000
#define PS5_PAD_BUTTON_SQUARE    0x8000
#define PS5_PAD_BUTTON_TOUCH_PAD 0x100000

typedef struct PS5_PadTouch
{
    uint16_t x;
    uint16_t y;
    uint8_t finger;
    uint8_t pad[3];
} PS5_PadTouch;

typedef struct PS5_PadTouchData
{
    uint8_t fingers;
    uint8_t pad1[3];
    uint32_t pad2;
    PS5_PadTouch touch[2];
} PS5_PadTouchData;

typedef struct PS5_PadColor
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
} PS5_PadColor;

typedef struct PS5_PadVibration {
    uint8_t large_motor;
    uint8_t small_motor;
} PS5_PadVibration;

typedef struct PS5_PadData
{
    uint32_t buttons;
    struct
    {
        uint8_t x;
        uint8_t y;
    } leftStick;
    struct
    {
        uint8_t x;
        uint8_t y;
    } rightStick;
    struct
    {
        uint8_t l2;
        uint8_t r2;
    } analogButtons;
    uint16_t padding;
    struct
    {
        float x;
        float y;
        float z;
        float w;
    } quat;
    struct
    {
        float x;
        float y;
        float z;
    } vel;
    struct
    {
        float x;
        float y;
        float z;
    } acell;
    PS5_PadTouchData touch;
    uint8_t connected;
    uint64_t timestamp;
    uint8_t ext[16];
    uint8_t count;
    uint8_t unknown[15];
} PS5_PadData;

int scePadInit(void);
int scePadOpen(int user_id, int type, int index, void *param);
int scePadGetHandle(int user_id, int type, int index);
int scePadReadState(int handle, PS5_PadData *data);
int scePadSetLightBar(int handle, const PS5_PadColor* color);
int scePadSetVibrationMode(int handle, int mode);
int scePadSetVibration(int handle, const PS5_PadVibration* vib);
int scePadClose(int handle);

int sceUserServiceInitialize(void *);
int sceUserServiceGetLoginUserIdList(int userId[4]);
int sceUserServiceGetUserName(int userId, char* name, size_t size);

}

static_assert(sizeof(PS5_VideoBuf)==32 && sizeof(PS5_VideoAttr)==80,"SDL PS5 video ABI");
static_assert(sizeof(PS5_PadData)==120 && offsetof(PS5_PadData,timestamp)==80,"SDL PS5 pad ABI");
