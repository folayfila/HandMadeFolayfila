// (C) Copyright 2026 by Abdallah Maaliki / folayfila.

#if !defined(WIN32_FOLAYFILA)
#define WIN32_FOLAYFILA

#include <windows.h>
#include "folayfila.h"

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
    int LatencySampleCount;
};

struct win32_debug_time_marker
{
    DWORD PlayCursor;
    DWORD WriteCursor;
};

struct win32_game_code
{
    HMODULE GameCodeDLL;
    FILETIME DLLLastWriteTime;
    
    // Callback must be valid. Check for null before usage.
    game_update_and_render* UpdateAndRender;
    bool32 IsValid;
};

#define WIN32_STATE_FILE_NAME_COUNT MAX_PATH
struct win32_state
{
    uint64 TotalSize;
    void *GameMemoryBlock;

    HANDLE RecordingHandle;
    int InputRecordingIndex;

    HANDLE PlaybackHandle;
    int InputPlayingIndex;

    char EXEFileName[WIN32_STATE_FILE_NAME_COUNT];
    char *OnePastLastEXEFileNameSlash;
};

#endif  // WIN32_FOLAYFILA