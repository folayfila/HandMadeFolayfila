// (C) Copyright 2026 by Abdallah Maaliki / folayfila.

#include "folayfila.h"

internal void GameOutputSound(game_state* GameState, game_output_sound_buffer* SoundBuffer)
{
    int16 ToneVolume = 3000;
    int16 ToneHz = 256;
    local_presist float tSine = 0.0f;
    int WavePeriod = SoundBuffer->SamplesPerSecond / ToneHz;

    int16* SampleOut = SoundBuffer->Samples;
    for (int SampleIndex = 0; SampleIndex < SoundBuffer->SampleCount; ++SampleIndex)
    {
#if 0
        float SineValue = sinf(GameState->tSine);
        int16 SampleValue = (int16)(SineValue * ToneVolume);
#else
        int16 SampleValue = 0;     
#endif        
        *SampleOut++ = SampleValue;
        *SampleOut++ = SampleValue;

        tSine += 2.0f * Pi32 * 1.0f / (float)WavePeriod;
    }
}

internal void DrawRectangle(game_graphics_buffer* Buffer, vec2 Min, vec2 Max)
{
    uint8* EndOfBuffer = (uint8*)Buffer->Memory + Buffer->Pitch * Buffer->Height;

    int32 MinX = Clamp(RoundFloatToInt32(Min.X), 0, RoundFloatToInt32(Min.X));
    int32 MaxX = Clamp(RoundFloatToInt32(Max.X), 0, Buffer->Width);
    int32 MinY = Clamp(RoundFloatToInt32(Min.Y), 0, RoundFloatToInt32(Min.Y));
    int32 MaxY = Clamp(RoundFloatToInt32(Max.Y), 0, Buffer->Height);

    static uint32_t Color = 0x12345678;
    //Color ^= Color << 13;
    //Color ^= Color >> 17;
    //Color ^= Color << 5;

    for (int X = (int)Min.X; X < (int)Max.X; ++X)
    {
        uint8* Pixel = ((uint8*)Buffer->Memory + X * Buffer->BytesPerPixel + (int)Min.Y * Buffer->Pitch);
        
        for (int Y = (int)Min.Y; Y < (int)Max.Y; ++Y)
        {
            if ((Pixel >= Buffer->Memory) && ((Pixel + 4) <= EndOfBuffer))
            {
                *(uint32*)Pixel = Color;
            }
            Pixel += Buffer->Pitch;
        }
    }
}

extern "C" GAME_UPDATE_AND_RENDER (GameUpdateAndRender)
{
    Assert((&Input->Controllers[0].Terminator - &Input->Controllers[0].Buttons[0]) == 
           (ArrayCount(Input->Controllers[0].Buttons)));
    Assert(sizeof(game_state) <= GameMemory->PermanentStorageSize);

    if (!GameMemory->IsInitialized)
    {
        GameMemory->IsInitialized = true;
    }

    game_state* GameState = (game_state*)GameMemory->PermanentStorage;

    for (int ControllerIndex = 0; ControllerIndex < ArrayCount(Input->Controllers); ++ControllerIndex)
    {
        game_controller_input* ControllerInput = GetController(Input, ControllerIndex);

        if (!ControllerInput->IsConnected)
        {
            continue;
        }

        if (ControllerInput->MoveDown.EndedDown)
        {
        }
        if (ControllerInput->MoveUp.EndedDown)
        {
        }
        if (ControllerInput->MoveRight.EndedDown)
        {
        }
        if (ControllerInput->MoveLeft.EndedDown)
        {
        }

        if (ControllerInput->RightShoulder.EndedDown)
        {
        }
        if (ControllerInput->LeftShoulder.EndedDown)
        {
        }

        // Quit game
        if (ControllerInput->Back.EndedDown)
        {
            GlobalRunning = false;
        }
    }
    vec2 Min;
    Min.X = 400.0f;
    Min.Y = 250.0f;
    vec2 Max;
    Max.X = Min.X + 100.0f;
    Max.Y = Min.Y + 100.0f;
    DrawRectangle(GraphicsBuffer, Min, Max);
}


/****************************************************************************/
// Old code
/*
internal void DisplayAwesomeGradient(game_graphics_buffer* Buffer, float XOffset, float YOffset)
{
    local_presist uint8 Red = 0;
    local_presist bool32 MaxRed = false;
    if (!MaxRed)
    {
        ++Red;
        if (Red == 255)
        {
            MaxRed = true;
        }
    }
    else
    {
        --Red;
        if (Red == 0)
        {
            MaxRed = false;
        }
    }

    uint8* Row = (uint8*)Buffer->Memory;
    for (int Y = 0; Y < Buffer->Height; ++Y)
    {
        uint32* Pixel = (uint32*)Row;
        for (int X = 0; X < Buffer->Width; ++X)
        {

            uint8 Blue = (uint8)(X + XOffset);
            uint8 Green = (uint8)(Y + YOffset);
            Red;

            // Memory: BB GG RR xx
            *Pixel++ = (Blue | Green << 8 | Red << 16);
        }
        Row += Buffer->Pitch;
    }
}
*/