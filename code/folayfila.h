// (C) Copyright 2026 by Abdallah Maaliki / folayfila.

#if !defined(FOLAYFILA_H)
#define FOLAYFILA_H

#include <stdint.h>
#include <math.h>

/************** typedef ***************/
typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int32 bool32;
/**************************************/

/************** #define ***************/
#define internal static
#define local_presist static
#define global_variable static

/*
* Build Options:
** FOLAYFILA_INTERNAL
*   0 - Build for public release.
*   1 - Build for developer only.

* ** FOLAYFILA_SLOW
*   0 - No slow code allowed.
*   1 - Slow code welcome.
*/

#if FOLAYFILA_SLOW
#define Assert(Expression) if(!(Expression)) {*(int *)0 = 0;}
#else
#define Assert(Expression)
#endif  // FOLAYFILA_SLOW

#if FOLAYFILA_INTERNAL
//! IMPORTANT: These are not for shipping!
struct debug_read_file_result
{
    uint32 ContentSize;
    void* Contents;
};
internal debug_read_file_result DEBUGPlatformReadEntireFile(char *FileName);
internal void DEBUGPlatformFreeFileMemory(void *Memeory);
internal bool32 DEBUGPlatformWriteEntireFile(char *FileName, uint32 MemorySize, void *Memory);
#endif  // FOLAYFILA_INTERNAL

#define Pi32 3.141459265359f

#define Kilobytes(Value) ((Value) * 1024)
#define Megabytes(Value) (Kilobytes(Value) * 1024)
#define Gigabytes(Value) (Megabytes(Value) * 1024)

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

inline uint32 SafeTruncateUInt64(uint64 Value)
{
    Assert(Value <= 0xffffffff);
    return ((uint32)Value);
}

/**************************************/

/************** globals ***************/
global_variable bool32 GlobalRunning;
/**************************************/

/************** structs ***************/
struct vec2
{
    float X;
    float Y;
};

struct game_graphics_buffer
{
	void* Memory;
    int Width;
    int Height;
    int Pitch;
};

struct game_output_sound_buffer
{
    int SamplesPerSecond;
    int SampleCount;
    int16 *Samples;
};

struct game_button_state
{
    int HalfTransitionCount;
    bool32 EndedDown;
};

struct game_controller_input
{
    bool32 IsConnected;
    bool32 IsAnalog;
    vec2 StickAverage;

    union
    {
        game_button_state Buttons[12];
        struct
        {
            game_button_state MoveUp;
            game_button_state MoveDown;
            game_button_state MoveLeft;
            game_button_state MoveRight;

            game_button_state ActionUp;
            game_button_state ActionDown;
            game_button_state ActionLeft;
            game_button_state ActionRight;
            
            game_button_state LeftShoulder;
            game_button_state RightShoulder;

            game_button_state Start;
            game_button_state Back;

            //? All buttons must be added above this one.
            game_button_state Terminator;
        };
    };
};

struct game_input
{
    game_controller_input Controllers[5];
};
inline game_controller_input* GetController(game_input *Input, int ControllerIndex)
{ 
    Assert(ControllerIndex < ArrayCount(Input->Controllers));
    return &Input->Controllers[ControllerIndex];
}

struct game_memory
{
    bool32 IsInitialized;

    uint64 PermanentStorageSize;
    void* PermanentStorage; // REQUIRED to be cleared to zero at startup.

    uint64 TransientStorageSize;
    void* TransientStorage; // REQUIRED to be cleared to zero at startup.
};

struct game_clocks
{
    float DeltaTime;
};

/**************************************/

internal void GameUpdateAndRender(game_memory* GameMemory, game_input* Input,
    game_graphics_buffer* GraphicsBuffer,
    game_output_sound_buffer* SoundBuffer);

///
///
///
/// 
struct game_state
{
    float ColorXoffset;
    float ColorYoffset;
    int ToneHz;
};


#endif  // FOLAYFILA_H