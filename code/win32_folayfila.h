// (C) Copyright 2026 by Abdallah Maaliki / folayfila.

#if !defined(WIN32_FOLAYFILA)
#define WIN32_FOLAYFILA

#include "folayfila.cpp"
#include <windows.h>

struct win32_window_deminsion
{
    int Width;
    int Height;
};

struct win32_offscreen_buffer
{
    BITMAPINFO Info;
    void* Memory;
    int Width;
    int Height;
    int Pitch;
    int BytesPerPixel;
};

struct win32_sound_output
{
    int SamplesPerSecond;
    uint32 RunningSampleIndex;
    int BytesPerSample;
    int SecondaryBufferSize;
    float tSine;
    int LatencySampleCount;
};

struct win32_debug_time_marker
{
    DWORD PlayCursor;
    DWORD WriteCursor;
};

#endif  // WIN32_FOLAYFILA