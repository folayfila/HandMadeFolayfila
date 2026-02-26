// (C) Copyright 2026 by Abdallah Maaliki / folayfila.

#if !defined(FOLAYFILA_H)
#define FOLAYFILA_H

#include <stdint.h>
#include <math.h>

#define internal static
#define local_presist static
#define global_variable static

#define Pi32 3.141459265359f

typedef int32_t bool32;

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

global_variable bool32 GlobalRunning;

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
    int16_t *Samples;
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

internal void GameUpdateAndRender(game_input* Input,
    game_graphics_buffer* GraphicsBuffer,
    game_output_sound_buffer* SoundBuffer);

#endif  // FOLAYFILA_H