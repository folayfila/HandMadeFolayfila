// (C) Copyright 2026 by Abdallah Maaliki / folayfila.

#include "folayfila.h"
#include "folayfila_intrinsics.h"

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

inline tile_chunk* GetTileChunk(world* World, int32 TileChunkX, int32 TileChunkY)
{
    tile_chunk* TileChunk = 0;

    if ((TileChunkX >= 0) && (TileChunkX < World->TileChunkCountX) &&
        (TileChunkY >= 0) && (TileChunkY < World->TileChunkCountY))
    {
        TileChunk = &World->TileChunks[TileChunkY * World->TileChunkCountX + TileChunkX];
    }
    return TileChunk;
}

inline uint32 GetTileValueUnchecked(world* World, tile_chunk* TileChunk, uint32 TileX, uint32 TileY)
{
    Assert(TileChunk);
    Assert(TileX < World->ChunkDim);
    Assert(TileY < World->ChunkDim);

    uint32 TileChunkValue = TileChunk->Tiles[TileY * World->ChunkDim + TileX];
    return TileChunkValue;
}

inline uint32 GetTileValue(world *World, tile_chunk *TileChunk, uint32 TestTileX, uint32 TestTileY)
{
    uint32 TileChunkValue = 0;
    if (TileChunk)
    {
        TileChunkValue = GetTileValueUnchecked(World, TileChunk, TestTileX, TestTileY);
    }
    return TileChunkValue;
}

inline void CannonicalizeCoord(world* World, uint32 *Tile, float *TileRel)
{
    int32 Offset = FloorFloatToInt32(*TileRel / World->TileSideInMeters);
    *Tile += Offset;
    *TileRel -= Offset * World->TileSideInMeters;

    Assert(*TileRel >= 0);
    Assert(*TileRel <= World->TileSideInMeters);
}

inline world_position RecanonicalizePosition(world* World, world_position Pos)
{
    world_position Result = Pos;

    CannonicalizeCoord(World, &Result.AbsTileX, &Result.TileRelX);
    CannonicalizeCoord(World, &Result.AbsTileY, &Result.TileRelY);

    return Result;
}

inline tile_chunk_position GetChunckPositionFor(world *World, uint32 AbsTileX, uint32 AbsTileY)
{
    tile_chunk_position Result;

    Result.TilChunkX = AbsTileX >> World->ChunkShift;
    Result.TilChunkY = AbsTileY >> World->ChunkShift;
    Result.RelTileX = AbsTileX & World->ChunkMask;
    Result.RelTileY = AbsTileY & World->ChunkMask;

    return Result;
}

internal uint32 GetTileValue(world* World, uint32 AbsTileX, uint32 AbsTileY)
{
    tile_chunk_position ChunkPos = GetChunckPositionFor(World, AbsTileX, AbsTileY);
    tile_chunk* TileChunk = GetTileChunk(World, ChunkPos.TilChunkX, ChunkPos.TilChunkY);
    uint32 TileChunkValue = GetTileValue(World, TileChunk, ChunkPos.RelTileX, ChunkPos.RelTileY);
    return TileChunkValue;
}

internal bool32 IsWorldPointEmpty(world* World, world_position CanPos)
{
    uint32 TileChunkValue = GetTileValue(World, CanPos.AbsTileX, CanPos.AbsTileY);
    bool32 Empty = (TileChunkValue == 'd');
    return Empty;
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
        GameState->PlayerP.AbsTileY = 4;
        GameState->PlayerP.TileRelX = 0.5f;
        GameState->PlayerP.TileRelY = 0.5f;

        GameMemory->IsInitialized = true;
    }

#define TILEMAP_COUNT_X (256)
#define TILEMAP_COUNT_Y (256)

    uint32 Tiles[TILEMAP_COUNT_X][TILEMAP_COUNT_Y] =
    {
        {'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w', 'w'},

        {'w', 'g', 'd', 'd',   'd', 'd', 'd', 'd',   'd', 'd', 'g', 'g',   'd', 'd', 'd', 'd', 'w', 'w', 'd', 'd', 'd',   'd', 'd', 'd', 'd',   'd', 'd', 'd', 'd',   'w', 'w', 'g', 'g', 'w', 'w', 'g', 'd', 'd',   'd', 'd', 'd', 'g',   'd', 'd', 'g', 'g',   'g', 'g', 'g', 'g', 'w', 'w', 'd', 'd', 'd',   'd', 'd', 'd', 'd',   'd', 'd', 'd', 'd',   'w', 'w', 'w', 'w', 'w'},
        {'w', 'g', 'd', 'd',   'd', 'w', 'g', 'd',   'd', 'd', 'd', 'd',   'd', 'd', 'd', 'd', 'w', 'w', 'd', 'd', 'd',   'd', 'w', 'g', 'd',   'd', 'd', 'd', 'd',   'w', 'w', 'g', 'g', 'w', 'w', 'w', 'd', 'd',   'd', 'w', 'g', 'd',   'd', 'd', 'd', 'g',   'g', 'g', 'g', 'g', 'w', 'w', 'd', 'd', 'd',   'd', 'w', 'g', 'd',   'd', 'd', 'd', 'd',   'w', 'w', 'w', 'w', 'w'},
        {'w', 'g', 'd', 'd',   'd', 'g', 'g', 'd',   'd', 'g', 'g', 'g',   'g', 'g', 'd', 'd', 'w', 'w', 'd', 'd', 'd',   'd', 'g', 'g', 'd',   'd', 'd', 'd', 'd',   'd', 'd', 'd', 'd', 'w', 'w', 'w', 'd', 'd',   'd', 'g', 'g', 'd',   'd', 'd', 'g', 'g',   'g', 'w', 'w', 'g', 'w', 'w', 'd', 'd', 'd',   'd', 'g', 'g', 'd',   'd', 'd', 'd', 'd',   'w', 'w', 'w', 'w', 'w'},
        {'w', 'd', 'd', 'd',   'd', 'd', 'd', 'd',   'd', 'd', 'd', 'd',   'd', 'd', 'd', 'd', 'd', 'd', 'd', 'd', 'd',   'd', 'd', 'd', 'd',   'd', 'd', 'd', 'd',   'd', 'd', 'd', 'd', 'd', 'd', 'd', 'd', 'd',   'd', 'd', 'd', 'd',   'd', 'd', 'd', 'd',   'd', 'd', 'd', 'd', 'd', 'd', 'd', 'd', 'd',   'g', 'g', 'g', 'd',   'd', 'd', 'd', 'd',   'w', 'w', 'w', 'w', 'w'},
        {'w', 'g', 'd', 'g',   'w', 'w', 'g', 'd',   'g', 'g', 'g', 'd',   'g', 'g', 'g', 'd', 'w', 'w', 'd', 'd', 'd',   'w', 'w', 'g', 'd',   'd', 'd', 'd', 'd',   'w', 'w', 'w', 'g', 'w', 'w', 'g', 'd', 'g',   'w', 'w', 'g', 'd',   'g', 'g', 'g', 'g',   'g', 'w', 'w', 'g', 'w', 'w', 'd', 'd', 'd',   'w', 'w', 'g', 'd',   'd', 'd', 'd', 'd',   'w', 'w', 'w', 'w', 'w'},
        {'w', 'g', 'd', 'g',   'w', 'w', 'g', 'd',   'd', 'd', 'g', 'd',   'd', 'd', 'd', 'd', 'w', 'w', 'd', 'd', 'd',   'w', 'w', 'g', 'd',   'd', 'd', 'd', 'd',   'w', 'w', 'w', 'g', 'w', 'w', 'g', 'd', 'g',   'w', 'w', 'g', 'd',   'd', 'd', 'g', 'g',   'g', 'w', 'w', 'g', 'w', 'w', 'd', 'd', 'd',   'w', 'w', 'g', 'd',   'd', 'd', 'd', 'd',   'w', 'w', 'w', 'w', 'w'},
        {'w', 'g', 'd', 'd',   'd', 'd', 'd', 'd',   'd', 'd', 'd', 'd',   'd', 'd', 'd', 'd', 'w', 'w', 'd', 'd', 'd',   'd', 'd', 'd', 'd',   'd', 'd', 'd', 'd',   'w', 'w', 'g', 'g', 'w', 'w', 'g', 'd', 'd',   'd', 'd', 'd', 'd',   'd', 'd', 'd', 'g',   'g', 'g', 'g', 'g', 'w', 'w', 'd', 'd', 'd',   'd', 'd', 'd', 'd',   'd', 'd', 'd', 'd',   'w', 'w', 'w', 'w', 'w'},

        {'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w', 'w'},
        {'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w', 'w'},
        {'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w', 'w'},
        {'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w', 'w'},
        {'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w', 'w'},
        {'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w', 'w'},
        {'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w', 'w'}
    };

    tile_chunk TileChunk;
    TileChunk.Tiles = (uint32*)Tiles;

    world World;
    World.ChunkShift = 8;
    World.ChunkMask = (1 << World.ChunkShift) - 1; // 256x256
    World.ChunkDim = 256;

    World.TileSideInMeters = 1.4f;
    World.TileSideInPixels = 60;
    World.MetersToPixels = (float)World.TileSideInPixels/World.TileSideInMeters;

    World.TileChunkCountX = 1;
    World.TileChunkCountY = 1;

    World.TileChunks = &TileChunk;

    float CenterX = 0.5f * (float)GraphicsBuffer->Width;
    float CenterY = 0.5f * (float)GraphicsBuffer->Height;

    float PlayerWidth = 0.25f * World.TileSideInMeters;
    float PlayerHeight = 0.25f * World.TileSideInMeters;

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
        world_position NewPlayerP = GameState->PlayerP;
        NewPlayerP.TileRelX += dPlayer.X;
        NewPlayerP.TileRelY += dPlayer.Y;
        NewPlayerP = RecanonicalizePosition(&World, NewPlayerP);

        world_position PlayerBottomRight = NewPlayerP;
        PlayerBottomRight.TileRelX += PlayerWidth;
        PlayerBottomRight = RecanonicalizePosition(&World, PlayerBottomRight);

        world_position PlayerBootmLeft = NewPlayerP;
        PlayerBootmLeft = RecanonicalizePosition(&World, PlayerBootmLeft);

        if (IsWorldPointEmpty(&World, PlayerBottomRight) &&
            IsWorldPointEmpty(&World, PlayerBootmLeft))
        {
            GameState->PlayerP = NewPlayerP;
        }
    }

    DrawRectangle(GraphicsBuffer, vec2(0.0f), vec2(1000.0f), color(0.0f, 0.0f, 0.0f));

    for (int32 RelRow = -10; RelRow < 10; ++RelRow)
    {
        for (int32 RelColumn = -20; RelColumn < 20; ++RelColumn)
        {
            color TileColor;
            uint32 Column = (uint32)Clamp32(GameState->PlayerP.AbsTileX + RelColumn, 0, World.ChunkDim-1);
            uint32 Row = (uint32)Clamp32(GameState->PlayerP.AbsTileY + RelRow, 0, World.ChunkDim-1);
            switch (GetTileValue(&World, &TileChunk, Column, Row))
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

            float PlayerOffsetX = World.MetersToPixels * GameState->PlayerP.TileRelX;
            float PlayerOffsetY = World.MetersToPixels * GameState->PlayerP.TileRelY;

            vec2 Min, Max;
            Min.X = CenterX + (float)RelColumn * World.TileSideInPixels - PlayerOffsetX;
            Min.Y = CenterY - (float)RelRow * World.TileSideInPixels + PlayerOffsetY - World.TileSideInPixels;

            Max.X = Min.X + World.TileSideInPixels;
            Max.Y = CenterY - (float)RelRow * World.TileSideInPixels + PlayerOffsetY;

            DrawRectangle(GraphicsBuffer, Min, Max, TileColor);
        }
    }

    color PlayerColor(1.0f, 0.5f, 0.5f);
    float Left = CenterX;
    float Bottom = CenterY - World.MetersToPixels*PlayerHeight;
    
    DrawRectangle(GraphicsBuffer, vec2(Left, Bottom),
        vec2(Left + World.MetersToPixels*PlayerWidth, Bottom + World.MetersToPixels*PlayerHeight),
        PlayerColor);
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