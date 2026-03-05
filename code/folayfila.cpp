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

internal void DrawRectangle(game_graphics_buffer* Buffer, vec2 Min, vec2 Max, color Color)
{
    uint8* EndOfBuffer = (uint8*)Buffer->Memory + Buffer->Pitch * Buffer->Height;

    int32 MinX = Clamp32(RoundFloatToInt32(Min.X), 0, RoundFloatToInt32(Min.X));
    int32 MaxX = Clamp32(RoundFloatToInt32(Max.X), 0, Buffer->Width);
    int32 MinY = Clamp32(RoundFloatToInt32(Min.Y), 0, RoundFloatToInt32(Min.Y));
    int32 MaxY = Clamp32(RoundFloatToInt32(Max.Y), 0, Buffer->Height);

    uint8* Row = ((uint8*)Buffer->Memory + MinX * Buffer->BytesPerPixel + MinY * Buffer->Pitch);
    uint32 Color32 = GetColorU32(Color);

    for (int Y = MinY; Y < MaxY; ++Y)
    {
        uint32* Pixel = (uint32*)Row;
        
        for (int X = MinX; X < MaxX; ++X)
        {
            *Pixel++ = Color32;
        }
        Row += Buffer->Pitch;
    }
}

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
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

    float UpperLeftX = -20.0f;
    float UpperLeftY = 0.0f;
    float TileWidth = 59.0f;
    float TileHeight = 59.0f;

    DrawRectangle(GraphicsBuffer, vec2(0.0f, 00.0f),
        vec2(GraphicsBuffer->Width, GraphicsBuffer->Height), color(1.0f, 1.0f, 0.0f));

    uint32 TileMap[9][17] =
    {
        {2, 2, 2, 2,   2, 2, 2, 2,   2, 2, 2, 2,   2, 2, 2, 2, 2},
        {2, 1, 0, 0,   0, 0, 0, 0,   0, 0, 1, 1,   0, 0, 0, 0, 2},
        {2, 1, 0, 0,   0, 0, 0, 0,   0, 0, 1, 1,   0, 0, 0, 0, 2},
        {2, 1, 1, 0,   0, 0, 0, 0,   0, 1, 0, 0,   0, 1, 1, 0, 2},
        {1, 1, 1, 0,   0, 1, 1, 0,   1, 0, 0, 0,   1, 1, 1, 1, 1},
        {2, 1, 0, 1,   1, 1, 1, 1,   0, 0, 0, 0,   1, 1, 1, 0, 2},
        {2, 1, 2, 0,   0, 0, 0, 0,   0, 0, 1, 0,   0, 0, 0, 0, 2},
        {2, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0, 2},
        {2, 2, 2, 2,   2, 2, 2, 2,   2, 2, 2, 2,   2, 2, 2, 2, 2},
    };

    for (int Row = 0; Row < 9; ++Row)
    {
        for (int Column = 0; Column < 17; ++Column)
        {
            color TileColor;
            switch (TileMap[Row][Column])
            {
            case 0:
            {
                // Dirt
                TileColor = color(87.0f, 40.0f, 36.0f, true);
            } break;
            case 1:
            {
                // Grass
                TileColor = color(0.0f, 0.9f, 0.0f);
            } break;
            case 2:
            {
                // Water
                TileColor = color(0.0f, 0.1f, 0.8f);
            } break;
            }

            vec2 Min, Max;
            Min.X = UpperLeftX + ((float)(Column * TileWidth));
            Min.Y = UpperLeftY + ((float)(Row * TileHeight));
            Max.X = Min.X + TileWidth;
            Max.Y = Min.Y + TileHeight;

            DrawRectangle(GraphicsBuffer, Min, Max, TileColor);
        }
    }

    //color PlayerColor;
    //DrawRectangle(GraphicsBuffer, vec2(), vec2(), PlayerColor);
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