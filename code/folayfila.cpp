// (C) Copyright 2026 by Abdallah Maaliki / folayfila.

#include "folayfila.h"

internal void GameOutputSound(game_output_sound_buffer* SoundBuffer, int ToneHz)
{
    local_presist float tSine;
    int16_t ToneVolume = 3000;
    int WavePeriod = SoundBuffer->SamplesPerSecond / ToneHz;

    int16_t* SampleOut = SoundBuffer->Samples;
    for (int SampleIndex = 0; SampleIndex < SoundBuffer->SampleCount; ++SampleIndex)
    {
        float SineValue = sinf(tSine);
        int16_t SampleValue = (int16_t)(SineValue * ToneVolume);
        *SampleOut++ = SampleValue;
        *SampleOut++ = SampleValue;

        tSine += 2.0f * Pi32 * 1.0f / (float)WavePeriod;
    }
}

internal void DisplayAwesomeGradient(game_graphics_buffer* Buffer, int XOffset, int YOffset)
{
    uint8_t* Row = (uint8_t*)Buffer->Memory;
    for (int Y = 0; Y < Buffer->Height; ++Y)
    {
        uint32_t* Pixel = (uint32_t*)Row;
        for (int X = 0; X < Buffer->Width; ++X)
        {

            uint8_t Blue = (uint8_t)(X + XOffset);
            uint8_t Green = (uint8_t)(Y + YOffset);
            uint8_t Red = 100;

            // Memory: BB GG RR xx
            *Pixel++ = (Blue | Green << 8 | Red << 16);
        }
        Row += Buffer->Pitch;
    }
}

internal void GameUpdateAndRender(game_input* Input,
                                  game_graphics_buffer* GraphicsBuffer,
                                  game_output_sound_buffer* SoundBuffer)
{
    local_presist int ColorXoffset = 0;
    local_presist int ColorYoffset = 0;
    local_presist int ToneHz = 256;

    game_controller_input* Input0 = &Input->Controllers[0];
    if (Input0->IsAnalog)
    {
        ToneHz = 256 + (int)(128.0f * (Input0->End.X));
        ColorYoffset += (int)(4.0f * (Input0->End.Y));
    }
    else
    {

    }

    if(Input0->GamePadDown.EndedDown)
    {
        ColorXoffset += 1;
    }
    if (Input0->GamePadUp.EndedDown)
    {
        ColorXoffset -= 1;
    }

    // Quit game
    if (Input0->GamePadRight.EndedDown)
    {
        GlobalRunning = false;
    }


    GameOutputSound(SoundBuffer, ToneHz);
    DisplayAwesomeGradient(GraphicsBuffer, ColorXoffset, ColorYoffset);
}