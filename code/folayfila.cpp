// (C) Copyright 2026 by Abdallah Maaliki / folayfila.

#include "folayfila.h"

internal void GameOutputSound(game_state* GameState, game_output_sound_buffer* SoundBuffer)
{
    int16 ToneVolume = 3000;
    int WavePeriod = SoundBuffer->SamplesPerSecond / GameState->ToneHz;

    int16* SampleOut = SoundBuffer->Samples;
    for (int SampleIndex = 0; SampleIndex < SoundBuffer->SampleCount; ++SampleIndex)
    {
        float SineValue = sinf(GameState->tSine);
        int16 SampleValue = (int16)(SineValue * ToneVolume);
        *SampleOut++ = SampleValue;
        *SampleOut++ = SampleValue;

        GameState->tSine += 2.0f * Pi32 * 1.0f / (float)WavePeriod;
    }
}

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

extern "C" GAME_UPDATE_AND_RENDER (GameUpdateAndRender)
{
    Assert((&Input->Controllers[0].Terminator - &Input->Controllers[0].Buttons[0]) == 
           (ArrayCount(Input->Controllers[0].Buttons)));
    Assert(sizeof(game_state) <= GameMemory->PermanentStorageSize);

    game_state* GameState = (game_state*)GameMemory->PermanentStorage;

    if (!GameMemory->IsInitialized)
    {
        // File I/O test code.
        char* FileName = __FILE__;
        debug_read_file_result File = GameMemory->DEBUGPlatformReadEntireFile(FileName);
        if (File.Contents)
        {
           GameMemory->DEBUGPlatformWriteEntireFile("Skibidi.out", File.ContentSize, File.Contents);
           GameMemory->DEBUGPlatformFreeFileMemory(File.Contents);
        }

        GameState->ToneHz = 256;
        GameState->tSine = 0.0f;
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

        GameState->ColorXoffset -= ControllerInput->StickAverage.X;
        GameState->ColorYoffset -= ControllerInput->StickAverage.Y;

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
            GameState->ColorXoffset -= 1.0f;
        }
        if (ControllerInput->MoveLeft.EndedDown)
        {
            GameState->ColorXoffset += 1.0f;
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
    GameOutputSound(GameState, SoundBuffer);
    DisplayAwesomeGradient(GraphicsBuffer, GameState->ColorXoffset, GameState->ColorYoffset);
}