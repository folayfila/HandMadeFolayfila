// (C) Copyright 2026 by Abdallah Maaliki / folayfila.

#include "folayfila_types.h"
#include "folayfila.h"
#include "folayfila_intrinsics.h"
#include "folayfila_tile.cpp"

static void GameOutputSound(game_state* GameState, game_output_sound_buffer* SoundBuffer)
{
    int16 ToneVolume = 3000;
    int16 ToneHz = 256;
    static float tSine = 0.0f;
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

static void DrawRectangle(game_graphics_buffer* Buffer, vec2 Min, vec2 Max, color Color)
{
    uint8* EndOfBuffer = (uint8*)Buffer->Memory + Buffer->Pitch * Buffer->Height;

    int32 MinX = Clamp32(RoundFloatToInt32(Min.X), 0, Buffer->Width);
    int32 MaxX = Clamp32(RoundFloatToInt32(Max.X), 0, Buffer->Width);
    int32 MinY = Clamp32(RoundFloatToInt32(Min.Y), 0, Buffer->Height);
    int32 MaxY = Clamp32(RoundFloatToInt32(Max.Y), 0, Buffer->Height);

    uint8* Row = ((uint8*)Buffer->Memory + MinX * Buffer->BytesPerPixel + MinY * Buffer->Pitch);
    
    uint32 Color32 = ((RoundFloatToUInt32(Color.R * 255) << 16) |
        (RoundFloatToUInt32(Color.G * 255) << 8) |
        (RoundFloatToUInt32(Color.B * 255) << 0));

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

static void InitialzeArena(memory_arena* Arena, size_t ArenaSize, uint8* Storage)
{
    Arena->Size = ArenaSize;
    Arena->Base = Storage;
    Arena->Used = 0;
}

#define PushSize(Arena, type) (type *)PushSize_(Arena, sizeof(type))
#define PushArray(Arena, Count, type) (type *)PushSize_(Arena, (Count)*sizeof(type))
void* PushSize_(memory_arena* Arena, size_t Size)
{
    Assert((Arena->Used + Size) <= Arena->Size);
    void* Result = Arena->Base + Arena->Used;
    Arena->Used += Size;

    return Result;
}

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    Assert((&Input->Controllers[0].Terminator - &Input->Controllers[0].Buttons[0]) ==
        (ArrayCount(Input->Controllers[0].Buttons)));
    Assert(sizeof(game_state) <= GameMemory->PermanentStorageSize);

    game_state* GameState = (game_state*)GameMemory->PermanentStorage;

    if (!GameMemory->IsInitialized)
    {
        GameState->PlayerP.AbsTileX = 2;
        GameState->PlayerP.AbsTileY = 5;
        GameState->PlayerP.TileRelX = 0.5f;
        GameState->PlayerP.TileRelY = 0.5f;

        InitialzeArena(&GameState->WorldArena, (size_t)(GameMemory->PermanentStorageSize - sizeof(game_state)), 
            (uint8 *)GameMemory->PermanentStorage + sizeof(game_state));

        GameState->World = (world*)PushSize(&GameState->WorldArena, world);
        world* World = GameState->World;

        World->TileMap = (tile_map*)PushSize(&GameState->WorldArena, tile_map);
        tile_map* TileMap = World->TileMap;

        TileMap->ChunkShift = 8;
        TileMap->ChunkMask = (1 << TileMap->ChunkShift) - 1; // 256x256
        TileMap->ChunkDim = 256;

        TileMap->TileSideInMeters = 1.0f;
        TileMap->TileSideInPixels = 60;
        TileMap->MetersToPixels = (float)TileMap->TileSideInPixels / TileMap->TileSideInMeters;

        TileMap->TileChunkCountX = 4;
        TileMap->TileChunkCountY = 4;

        TileMap->TileChunks = PushArray(&GameState->WorldArena, 
            TileMap->TileChunkCountX*TileMap->TileChunkCountY, tile_chunk);

        for (uint32 Y = 0; Y < TileMap->TileChunkCountY; ++Y)
        {
            for (uint32 X = 0; X < TileMap->TileChunkCountX; ++X)
            {
                TileMap->TileChunks[Y * TileMap->TileChunkCountX + X].Tiles =
                    PushArray(&GameState->WorldArena, TileMap->ChunkDim * TileMap->ChunkDim, uint32);
            }
        }

        uint32 TilesPerWidth = 17;
        uint32 TilesPerHeight = 9;
        for (uint32 ScreenY = 0; ScreenY < 32; ++ScreenY)
        {
            for (uint32 ScreenX = 0; ScreenX < 32; ++ScreenX)
            {
                for (uint32 TileY = 0; TileY < TilesPerHeight; ++TileY)
                {
                    for (uint32 TileX = 0; TileX < TilesPerWidth; ++TileX)
                    {
                        uint32 AbsTileX = ScreenX * TilesPerWidth + TileX;
                        uint32 AbsTileY = ScreenY * TilesPerHeight + TileY;

                        SetTileValue(TileMap, AbsTileX, AbsTileY,
                            (TileX == TileY) && (TileY % 2) ? 'g' : 'd');
                    }

                }
            }
        }

        GameMemory->IsInitialized = true;
    }

    tile_map* TileMap = GameState->World->TileMap;
    float PlayerWidth = 0.25f * TileMap->TileSideInMeters;
    float PlayerHeight = 0.25f * TileMap->TileSideInMeters;

    for (int ControllerIndex = 0; ControllerIndex < ArrayCount(Input->Controllers); ++ControllerIndex)
    {
        game_controller_input* ControllerInput = GetController(Input, ControllerIndex);

        if (!ControllerInput->IsConnected)
        {
            continue;
        }

        // Quit game
        if (ControllerInput->Back.EndedDown)
        {
            GlobalRunning = false;
            break;
        }

        vec2 dPlayer(0.0f);
        if (ControllerInput->MoveDown.EndedDown)
        {
            dPlayer.Y -= 1.0f;
        }
        if (ControllerInput->MoveUp.EndedDown)
        {
            dPlayer.Y += 1.0f;
        }
        if (ControllerInput->MoveRight.EndedDown)
        {
            dPlayer.X += 1.0f;
        }
        if (ControllerInput->MoveLeft.EndedDown)
        {
            dPlayer.X -= 1.0f;
        }

        if (ControllerInput->RightShoulder.EndedDown)
        {
            dPlayer.X += 10.0f;
        }
        if (ControllerInput->LeftShoulder.EndedDown)
        {
            dPlayer.X -= 10.0f;
        }
        dPlayer = (dPlayer * 5.0f) * Input->DeltaTime;

        if (dPlayer.X == 0 && dPlayer.Y == 0)
        {
            continue;
        }
        tile_map_position NewPlayerP = GameState->PlayerP;
        NewPlayerP.TileRelX += dPlayer.X;
        NewPlayerP.TileRelY += dPlayer.Y;
        NewPlayerP = RecanonicalizePosition(TileMap, NewPlayerP);

        tile_map_position PlayerBottomRight = NewPlayerP;
        PlayerBottomRight.TileRelX += PlayerWidth;
        PlayerBottomRight = RecanonicalizePosition(TileMap, PlayerBottomRight);

        tile_map_position PlayerBootmLeft = NewPlayerP;
        PlayerBootmLeft = RecanonicalizePosition(TileMap, PlayerBootmLeft);

        if (IsTileMapPointEmpty(TileMap, PlayerBottomRight) &&
            IsTileMapPointEmpty(TileMap, PlayerBootmLeft))
        {
            GameState->PlayerP = NewPlayerP;
        }
    }

    DrawRectangle(GraphicsBuffer, vec2(0.0f), vec2(1000.0f), color(0.0f, 0.0f, 0.0f));

    float ScreenCenterX = 0.5f * (float)GraphicsBuffer->Width;
    float ScreenCenterY = 0.5f * (float)GraphicsBuffer->Height;

    for (int32 RelRow = -10; RelRow < 10; ++RelRow)
    {
        for (int32 RelColumn = -20; RelColumn < 20; ++RelColumn)
        {
            color TileColor;
            uint32 Column = (uint32)Clamp32(GameState->PlayerP.AbsTileX + RelColumn, 0, TileMap->ChunkDim-1);
            uint32 Row = (uint32)Clamp32(GameState->PlayerP.AbsTileY + RelRow, 0, TileMap->ChunkDim-1);
            switch (GetTileValue(TileMap, Column, Row))
            {
            case 'd':
            {
                // Dirt
                TileColor = color(87.0f, 40.0f, 36.0f, true);
            } break;
            case 'g':
            {
                // Grass
                TileColor = color(0.0f, 0.9f, 0.0f);
            } break;
            case 'w':
            {
                // Water
                TileColor = color(0.2f, 0.75f, 0.95f);
            } break;
            default:
            {
                TileColor = color(0.7f, 0.7f, 0.7f);
            }break;
            }

            if (Column == GameState->PlayerP.AbsTileX && Row == GameState->PlayerP.AbsTileY)
            {
                TileColor = color(1.0f, 0.0f, 0.0f);
            }

            float PlayerOffsetX = TileMap->MetersToPixels * GameState->PlayerP.TileRelX;
            float PlayerOffsetY = TileMap->MetersToPixels * GameState->PlayerP.TileRelY;

            vec2 Min, Max;
            Min.X = ScreenCenterX + (float)RelColumn * TileMap->TileSideInPixels - PlayerOffsetX;
            Min.Y = ScreenCenterY - (float)RelRow * TileMap->TileSideInPixels + PlayerOffsetY - TileMap->TileSideInPixels;

            Max.X = Min.X + TileMap->TileSideInPixels;
            Max.Y = ScreenCenterY - (float)RelRow * TileMap->TileSideInPixels + PlayerOffsetY;

            DrawRectangle(GraphicsBuffer, Min, Max, TileColor);
        }
    }

    color PlayerColor(1.0f, 0.5f, 0.5f);
    float Left = ScreenCenterX;
    float Bottom = ScreenCenterY - TileMap->MetersToPixels*PlayerHeight;
    
    DrawRectangle(GraphicsBuffer, vec2(Left, Bottom),
        vec2(Left + TileMap->MetersToPixels*PlayerWidth, Bottom + TileMap->MetersToPixels*PlayerHeight),
        PlayerColor);
}
/****************************************************************************/
// Old code
/*
static void DisplayAwesomeGradient(game_graphics_buffer* Buffer, float XOffset, float YOffset)
{
    static uint8 Red = 0;
    static bool32 MaxRed = false;
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