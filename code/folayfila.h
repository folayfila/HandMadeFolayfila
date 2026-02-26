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

#define Pi32 3.141459265359f

#define Kilobytes(Value) ((Value) * 1024)
#define Megabytes(Value) (Kilobytes(Value) * 1024)
#define Gigabytes(Value) (Megabytes(Value) * 1024)

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))
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
    bool32 IsAnalog;

    vec2 Start;
    vec2 Min;
    vec2 Max;
    vec2 End;

    union
    {
        struct
        {
            game_button_state DPadUp;
            game_button_state DPadDown;
            game_button_state DPaDLeft;
            game_button_state DPadRight;
            game_button_state GamePadUp;
            game_button_state GamePadDown;
            game_button_state GamePadLeft;
            game_button_state GamePadRight;
            game_button_state LeftShoulder;
            game_button_state RightShoulder;
        };
    };
};

struct game_input
{
    game_controller_input Controllers[4];
};

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
    float DeltaTime
};

/**************************************/

internal void GameUpdateAndRender(game_memory* Memory, game_input* Input,
    game_graphics_buffer* GraphicsBuffer,
    game_output_sound_buffer* SoundBuffer);

///
///
///
/// 
struct game_state
{
    int ColorXoffset;
    int ColorYoffset;
    int ToneHz;
};


#endif  // FOLAYFILA_H