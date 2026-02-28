// (C) Copyright 2026 by Abdallah Maaliki / folayfila.

#include "folayfila.h"

internal void GameOutputSound(game_output_sound_buffer* SoundBuffer, int ToneHz)
{
    local_presist float tSine;
    int16 ToneVolume = 3000;
    int WavePeriod = SoundBuffer->SamplesPerSecond / ToneHz;

    int16* SampleOut = SoundBuffer->Samples;
    for (int SampleIndex = 0; SampleIndex < SoundBuffer->SampleCount; ++SampleIndex)
    {
        float SineValue = sinf(tSine);
        int16 SampleValue = (int16)(SineValue * ToneVolume);
        *SampleOut++ = SampleValue;
        *SampleOut++ = SampleValue;

        tSine += 2.0f * Pi32 * 1.0f / (float)WavePeriod;
    }
}

internal void DisplayAwesomeGradient(game_graphics_buffer* Buffer, float XOffset, float YOffset)
{
    uint8* Row = (uint8*)Buffer->Memory;
    for (int Y = 0; Y < Buffer->Height; ++Y)
    {
        uint32* Pixel = (uint32*)Row;
        for (int X = 0; X < Buffer->Width; ++X)
        {

            uint8 Blue = (uint8)(X + XOffset);
            uint8 Green = (uint8)(Y + YOffset);
            uint8 Red = 100;

            // Memory: BB GG RR xx
            *Pixel++ = (Blue | Green << 8 | Red << 16);
        }
        Row += Buffer->Pitch;
    }
}

internal void GameUpdateAndRender(game_memory* GameMemory, game_input* Input,
                                  game_graphics_buffer* GraphicsBuffer,
                                  game_output_sound_buffer* SoundBuffer)
{
    Assert((&Input->Controllers[0].Terminator - &Input->Controllers[0].Buttons[0]) == 
           (ArrayCount(Input->Controllers[0].Buttons)));
    Assert(sizeof(game_state) <= GameMemory->PermanentStorageSize);

    game_state* GameState = (game_state*)GameMemory->PermanentStorage;

    if (!GameMemory->IsInitialized)
    {
        // File I/O test code.
        char* FileName = __FILE__;
        debug_read_file_result File = DEBUGPlatformReadEntireFile(FileName);
        if (File.Contents)
        {
           DEBUGPlatformWriteEntireFile("Skibidi.out", File.ContentSize, File.Contents);
           DEBUGPlatformFreeFileMemory(File.Contents);
        }

        GameState->ToneHz = 256;
        GameMemory->IsInitialized = true;
    }

    for (int ControllerIndex = 0; ControllerIndex < ArrayCount(Input->Controllers); ++ControllerIndex)
    {
        game_controller_input* ControllerInput = GetController(Input, ControllerIndex);

        if (!ControllerInput->IsConnected)
        {
            continue;
        }

        if (ControllerInput->IsAnalog)
        {
            GameState->ToneHz = 256 + (int)(128.0f * (ControllerInput->StickAverage.X));
        }

        GameState->ColorXoffset += ControllerInput->StickAverage.X;
        GameState->ColorYoffset += ControllerInput->StickAverage.Y;

        if (ControllerInput->MoveDown.EndedDown)
        {
            GameState->ColorYoffset += 1.0f;
        }
        if (ControllerInput->MoveUp.EndedDown)
        {
            GameState->ColorYoffset -= 1.0f;
        }
        if (ControllerInput->MoveRight.EndedDown)
        {
            GameState->ColorXoffset += 1.0f;
        }
        if (ControllerInput->MoveLeft.EndedDown)
        {
            GameState->ColorXoffset -= 1.0f;
        }

        if (ControllerInput->RightShoulder.EndedDown)
        {
            GameState->ColorXoffset += 1000.0f;
        }
        if (ControllerInput->LeftShoulder.EndedDown)
        {
            GameState->ColorXoffset -= 1000.0f;
        }

        // Quit game
        if (ControllerInput->Back.EndedDown)
        {
            GlobalRunning = false;
        }
    }
    GameOutputSound(SoundBuffer, GameState->ToneHz);
    DisplayAwesomeGradient(GraphicsBuffer, GameState->ColorXoffset, GameState->ColorYoffset);
}