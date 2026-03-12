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

    int32 MinX = Clamp32(RoundFloatToInt32(Min.X), 0, RoundFloatToInt32(Min.X));
    int32 MaxX = Clamp32(RoundFloatToInt32(Max.X), 0, Buffer->Width);
    int32 MinY = Clamp32(RoundFloatToInt32(Min.Y), 0, RoundFloatToInt32(Min.Y));
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

inline tile_map* GetTileMap(world* World, int32 TileMapX, int32 TileMapY)
{
    tile_map* TileMap = 0;

    if ((TileMapX >= 0) && (TileMapX < World->TileMapCountX) &&
        (TileMapY >= 0) && (TileMapY < World->TileMapCountY))
    {
        TileMap = &World->TileMaps[TileMapY * World->TileMapCountX + TileMapX];
    }
    return TileMap;
}

inline uint32 GetTileValueUnchecked(world* World, tile_map* TileMap, int32 TileX, int32 TileY)
{
    uint32 TileMapValue = TileMap->Tiles[TileY * World->CountX + TileX];
    return TileMapValue;
}

inline bool32 IsTileMapPointEmpty(world *World, tile_map *TileMap, int32 TestTileX, int32 TestTileY)
{
    bool32 Empty = false;
    if (TileMap)
    {
        if ((TestTileX >= 0) && (TestTileX < World->CountX) &&
            (TestTileY >= 0) && (TestTileY < World->CountY))
        {
            uint32 Tile = GetTileValueUnchecked(World, TileMap, TestTileX, TestTileY);
            Empty = (Tile == 'd');
        }
    }
    return Empty;
}

inline void CannonicalizeCoord(world* World, int32 TileCount, int32 *TileMap, int32 *Tile, float *TileRel)
{
    int32 Offset = FloorFloatToInt32(*TileRel / World->TileSideInMeters);
    *Tile += Offset;
    *TileRel -= Offset * World->TileSideInMeters;

    Assert(*TileRel >= 0);
    Assert(*TileRel < World->TileSideInMeters);

    if (*Tile < 0)
    {
        *Tile += TileCount;
        --*TileMap;
    }
    if (*Tile >= TileCount)
    {
        *Tile -= TileCount;
        ++*TileMap;
    }
}

inline canonical_position RecanonicalizePosition(world* World, canonical_position Pos)
{
    canonical_position Result = Pos;

    CannonicalizeCoord(World, World->CountX, &Result.TileMapX, &Result.TileX, &Result.TileRelX);
    CannonicalizeCoord(World, World->CountY,  &Result.TileMapY, &Result.TileY, &Result.TileRelY);

    return Result;
}

internal bool32 IsWorldPointEmpty(world* World, canonical_position CanPos)
{
    bool32 Empty = false;

    tile_map* TileMap = GetTileMap(World, CanPos.TileMapX, CanPos.TileMapY);
    Empty = IsTileMapPointEmpty(World, TileMap, CanPos.TileX, CanPos.TileY) && 
        (CanPos.TileMapX >= 0 || CanPos.TileMapX < World->CountX) &&
        (CanPos.TileMapY >= 0 || CanPos.TileMapY < World->CountY);

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
        GameState->PlayerP.TileMapX = 0;
        GameState->PlayerP.TileMapY = 0;
        GameState->PlayerP.TileX = 3;
        GameState->PlayerP.TileY = 3;
        GameState->PlayerP.TileRelX = 0.5f;
        GameState->PlayerP.TileRelY = 0.5f;

        GameMemory->IsInitialized = true;
    }

#define TILEMAP_COUNT_X (17)
#define TILEMAP_COUNT_Y (9)

    uint32 Tiles00[TILEMAP_COUNT_Y][TILEMAP_COUNT_X] =
    {
        {'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w', 'w'},
        {'w', 'g', 'd', 'd',   'd', 'd', 'd', 'd',   'd', 'd', 'g', 'g',   'd', 'd', 'd', 'd', 'w'},
        {'w', 'g', 'd', 'd',   'd', 'w', 'g', 'd',   'd', 'd', 'd', 'd',   'd', 'd', 'd', 'd', 'w'},
        {'w', 'g', 'd', 'd',   'd', 'g', 'g', 'd',   'd', 'g', 'g', 'g',   'g', 'g', 'd', 'd', 'w'},
        {'w', 'g', 'd', 'd',   'g', 'g', 'g', 'd',   'd', 'g', 'g', 'g',   'g', 'g', 'd', 'd', 'd'},
        {'w', 'g', 'd', 'g',   'w', 'w', 'g', 'd',   'g', 'g', 'g', 'd',   'g', 'g', 'g', 'd', 'w'},
        {'w', 'g', 'd', 'g',   'w', 'w', 'g', 'd',   'd', 'd', 'g', 'd',   'd', 'd', 'd', 'd', 'w'},
        {'w', 'g', 'd', 'd',   'd', 'd', 'd', 'd',   'd', 'd', 'd', 'd',   'd', 'd', 'd', 'd', 'w'},
        {'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w',   'd', 'w', 'w', 'w',   'w', 'w', 'w', 'w', 'w'},
    };

    uint32 Tiles01[TILEMAP_COUNT_Y][TILEMAP_COUNT_X] =
    {
        {'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w', 'w'},
        {'w', 'd', 'd', 'd',   'd', 'd', 'd', 'd',   'd', 'd', 'd', 'd',   'w', 'w', 'g', 'g', 'w'},
        {'w', 'd', 'd', 'd',   'd', 'w', 'g', 'd',   'd', 'd', 'd', 'd',   'w', 'w', 'g', 'g', 'w'},
        {'w', 'd', 'd', 'd',   'd', 'g', 'g', 'd',   'd', 'd', 'd', 'd',   'd', 'd', 'd', 'd', 'w'},
        {'d', 'd', 'd', 'd',   'g', 'g', 'g', 'd',   'd', 'd', 'd', 'd',   'd', 'd', 'd', 'd', 'w'},
        {'w', 'd', 'd', 'd',   'w', 'w', 'g', 'd',   'd', 'd', 'd', 'd',   'w', 'w', 'w', 'g', 'w'},
        {'w', 'd', 'd', 'd',   'w', 'w', 'g', 'd',   'd', 'd', 'd', 'd',   'w', 'w', 'w', 'g', 'w'},
        {'w', 'd', 'd', 'd',   'd', 'd', 'd', 'd',   'd', 'd', 'd', 'd',   'w', 'w', 'g', 'g', 'w'},
        {'W', 'W', 'W', 'W',   'w', 'w', 'w', 'w',   'd', 'w', 'w', 'w',   'w', 'w', 'w', 'w', 'w'},

    };

    uint32 Tiles10[TILEMAP_COUNT_Y][TILEMAP_COUNT_X] =
    {
        {'w', 'w', 'w', 'w',   'w', 'w', 'w', 'g',   'd', 'w', 'w', 'w',   'w', 'w', 'w', 'w', 'w'},
        {'w', 'g', 'd', 'd',   'd', 'd', 'd', 'g',   'd', 'd', 'g', 'g',   'g', 'g', 'g', 'g', 'w'},
        {'w', 'w', 'd', 'd',   'd', 'w', 'g', 'd',   'd', 'd', 'd', 'g',   'g', 'g', 'g', 'g', 'w'},
        {'w', 'w', 'd', 'd',   'd', 'g', 'g', 'd',   'd', 'd', 'g', 'g',   'g', 'w', 'w', 'g', 'w'},
        {'w', 'w', 'd', 'd',   'g', 'g', 'g', 'd',   'd', 'd', 'd', 'd',   'd', 'd', 'd', 'd', 'd'},
        {'w', 'g', 'd', 'g',   'w', 'w', 'g', 'd',   'g', 'g', 'g', 'g',   'g', 'w', 'w', 'g', 'w'},
        {'w', 'g', 'd', 'g',   'w', 'w', 'g', 'd',   'd', 'd', 'g', 'g',   'g', 'w', 'w', 'g', 'w'},
        {'w', 'g', 'd', 'd',   'd', 'd', 'd', 'd',   'd', 'd', 'd', 'g',   'g', 'g', 'g', 'g', 'w'},
        {'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w', 'w'},
    };

    uint32 Tiles11[TILEMAP_COUNT_Y][TILEMAP_COUNT_X] =
    {
        {'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w',   'd', 'w', 'w', 'w',   'w', 'w', 'w', 'w', 'w'},
        {'w', 'd', 'd', 'd',   'd', 'd', 'd', 'd',   'd', 'd', 'd', 'd',   'd', 'd', 'd', 'd', 'w'},
        {'w', 'd', 'd', 'd',   'd', 'w', 'g', 'd',   'd', 'd', 'd', 'd',   'w', 'w', 'g', 'g', 'w'},
        {'w', 'd', 'd', 'd',   'd', 'g', 'g', 'd',   'd', 'd', 'd', 'd',   'w', 'w', 'w', 'g', 'w'},
        {'d', 'd', 'd', 'd',   'g', 'g', 'g', 'd',   'd', 'd', 'd', 'd',   'w', 'w', 'w', 'g', 'w'},
        {'w', 'd', 'd', 'd',   'w', 'w', 'g', 'd',   'd', 'd', 'd', 'd',   'w', 'w', 'w', 'g', 'w'},
        {'w', 'd', 'd', 'd',   'w', 'w', 'g', 'd',   'd', 'd', 'd', 'd',   'w', 'w', 'w', 'g', 'w'},
        {'w', 'd', 'd', 'd',   'd', 'd', 'd', 'd',   'd', 'd', 'd', 'd',   'w', 'w', 'g', 'g', 'w'},
        {'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w', 'w'},
    };
                   // Y   X
    tile_map TileMaps[2][2];
    TileMaps[0][0].Tiles = (uint32*)Tiles00;
    TileMaps[0][1].Tiles = (uint32*)Tiles01;
    TileMaps[1][0].Tiles = (uint32*)Tiles10;
    TileMaps[1][1].Tiles = (uint32*)Tiles11;

    world World;
    World.TileSideInMeters = 1.4f;
    World.TileSideInPixels = 60;
    World.MetersToPixels = (float)World.TileSideInPixels/World.TileSideInMeters;
    World.CountX = TILEMAP_COUNT_X;
    World.CountY = TILEMAP_COUNT_Y;

    World.UpperLeftX = -(float)World.TileSideInMeters*30.0f;
    World.UpperLeftY = -(float)World.TileSideInMeters*30.0f;

    World.TileMapCountX = 2;
    World.TileMapCountY = 2;
    World.TileMaps = (tile_map*)TileMaps;

    tile_map* CurrentTileMap = GetTileMap(&World, GameState->PlayerP.TileMapX, GameState->PlayerP.TileMapY);

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
            dPlayer.Y += 1.0f;
        }
        if (ControllerInput->MoveUp.EndedDown)
        {
            dPlayer.Y -= 1.0f;
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
        dPlayer = (dPlayer * 10.0f) * Input->DeltaTime;

        if (dPlayer.X == 0 && dPlayer.Y == 0)
        {
            continue;
        }
        canonical_position NewPlayerP = GameState->PlayerP;
        NewPlayerP.TileRelX += dPlayer.X;
        NewPlayerP.TileRelY += dPlayer.Y;
        NewPlayerP = RecanonicalizePosition(&World, NewPlayerP);

        float PlayerCollisionWidth = PlayerWidth * 0.25f;

        canonical_position PlayerRight = NewPlayerP;
        PlayerRight.TileRelX += PlayerCollisionWidth;
        RecanonicalizePosition(&World, PlayerRight);

        canonical_position PlayerLeft = NewPlayerP;
        PlayerLeft.TileRelX -= PlayerCollisionWidth;
        RecanonicalizePosition(&World, PlayerLeft);

        if (IsWorldPointEmpty(&World, PlayerRight) &&
            IsWorldPointEmpty(&World, PlayerLeft))
        {
            GameState->PlayerP = NewPlayerP;
        }
    }

    DrawRectangle(GraphicsBuffer, vec2(0.0f), vec2(1000.0f), color(0.0f, 0.0f, 0.0f));

    for (int Row = 0; Row < TILEMAP_COUNT_Y; ++Row)
    {
        for (int Column = 0; Column < TILEMAP_COUNT_X; ++Column)
        {
            color TileColor;
            switch (GetTileValueUnchecked(&World, CurrentTileMap, Column, Row))
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
                TileColor = color(0.0f, 0.1f, 0.8f);
            } break;
            }

            if ((Column == GameState->PlayerP.TileX) && (Row == GameState->PlayerP.TileY))
            {
                TileColor = color(1.0f, 0.0f, 0.0f);
            }

            vec2 Min, Max;
            Min.X = World.UpperLeftX + ((float)(Column * World.TileSideInPixels));
            Min.Y = World.UpperLeftY + ((float)(Row * World.TileSideInPixels));
            Max.X = Min.X + World.TileSideInPixels;
            Max.Y = Min.Y + World.TileSideInPixels;

            DrawRectangle(GraphicsBuffer, Min, Max, TileColor);
        }
    }

    color PlayerColor(1.0f, 0.5f, 0.5f);
    float Left = World.UpperLeftX + World.MetersToPixels*World.TileSideInMeters*GameState->PlayerP.TileX +
        World.MetersToPixels*GameState->PlayerP.TileRelX - World.MetersToPixels*(0.25f*PlayerWidth);

    float Top = World.UpperLeftY + World.MetersToPixels*World.TileSideInMeters*GameState->PlayerP.TileY +
        World.MetersToPixels*GameState->PlayerP.TileRelY - World.MetersToPixels*PlayerHeight;
    
    DrawRectangle(GraphicsBuffer, vec2(Left, Top),
        vec2(Left + World.MetersToPixels*PlayerWidth, Top + World.MetersToPixels*PlayerHeight),
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