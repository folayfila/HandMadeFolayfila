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

inline bool IsTileMapPointEmpty(world *World, tile_map *TileMap, int32 TestTileX, int32 TestTileY)
{
    bool Empty = false;
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

inline canonical_position GetCanonicalPosition(world* World, raw_position Pos)
{
    canonical_position Result;

    Result.TileMapX = Pos.TileMapX;
    Result.TileMapY = Pos.TileMapY;

    Result.X = Pos.X;
    Result.Y = Pos.Y;

    float X = Pos.X - World->UpperLeftX;
    float Y = Pos.Y - World->UpperLeftY;
    Result.TileX = FloorFloatToInt32(X / World->TileWidth);
    Result.TileY = FloorFloatToInt32(Y / World->TileHeight);

    //Assert(Result.X >= 0);
    //Assert(Result.Y >= 0);
    //Assert(Result.X < World->TileWidth);
    //Assert(Result.Y < World->TileHeight);

    if (Result.TileX < 0)
    {
        Result.TileX += World->CountX;
        --Result.TileMapX;
        Result.X += World->CountX * World->TileWidth;
    }
    if (Result.TileY < 0)
    {
        Result.TileY += World->CountY;
        --Result.TileMapY;
        Result.Y += World->CountY * World->TileHeight;
    }
    if (Result.TileX >= World->CountX)
    {
        Result.TileX -= World->CountX;
        ++Result.TileMapX;
        Result.X -= World->CountX * World->TileWidth;
    }
    if (Result.TileY >= World->CountY)
    {
        Result.TileY -= World->CountY;
        ++Result.TileMapY;
        Result.Y -= World->CountY * World->TileHeight;
    }

    return Result;
}

internal bool IsWorldPointEmpty(world* World, raw_position Pos)
{
    bool Empty = false;

    canonical_position CanPos = GetCanonicalPosition(World, Pos);
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
        GameState->Player = vec2(125.0f, 250.0f);
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
        {'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w',   'd', 'w', 'w', 'w',   'w', 'w', 'w', 'w', 'w'},
        {'w', 'd', 'd', 'd',   'd', 'd', 'd', 'd',   'd', 'd', 'd', 'd',   'w', 'w', 'g', 'g', 'w'},
        {'w', 'd', 'd', 'd',   'd', 'w', 'g', 'd',   'd', 'd', 'd', 'd',   'w', 'w', 'g', 'g', 'w'},
        {'w', 'd', 'd', 'd',   'd', 'g', 'g', 'd',   'd', 'd', 'd', 'd',   'd', 'd', 'd', 'd', 'w'},
        {'d', 'd', 'd', 'd',   'g', 'g', 'g', 'd',   'd', 'd', 'd', 'd',   'd', 'd', 'd', 'd', 'w'},
        {'w', 'd', 'd', 'd',   'w', 'w', 'g', 'd',   'd', 'd', 'd', 'd',   'w', 'w', 'w', 'g', 'w'},
        {'w', 'd', 'd', 'd',   'w', 'w', 'g', 'd',   'd', 'd', 'd', 'd',   'w', 'w', 'w', 'g', 'w'},
        {'w', 'd', 'd', 'd',   'd', 'd', 'd', 'd',   'd', 'd', 'd', 'd',   'w', 'w', 'g', 'g', 'w'},
        {'W', 'W', 'W', 'W',   'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w', 'w'},

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
    World.CountX = TILEMAP_COUNT_X;
    World.CountY = TILEMAP_COUNT_Y;
    World.UpperLeftX = -30;
    World.UpperLeftY = -30;
    World.TileWidth = 60;
    World.TileHeight = 60;
    World.TileMapCountX = 2;
    World.TileMapCountY = 2;
    World.TileMaps = (tile_map*)TileMaps;

    tile_map* CurrentTileMap = GetTileMap(&World, GameState->PlayerTileMapX, GameState->PlayerTileMapY);

    float PlayerWidth = 0.25f * World.TileWidth;
    float PlayerHeight = 0.25f * World.TileHeight;

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
            dPlayer.Y += 10.0f;
        }
        if (ControllerInput->MoveUp.EndedDown)
        {
            dPlayer.Y -= 10.0f;
        }
        if (ControllerInput->MoveRight.EndedDown)
        {
            dPlayer.X += 10.0f;
        }
        if (ControllerInput->MoveLeft.EndedDown)
        {
            dPlayer.X -= 10.0f;
        }

        if (ControllerInput->RightShoulder.EndedDown)
        {
            dPlayer.X += 100.0f;
        }
        if (ControllerInput->LeftShoulder.EndedDown)
        {
            dPlayer.X -= 100.0f;
        }
        dPlayer = dPlayer * 10.0f;

        if (dPlayer.X == 0 && dPlayer.Y == 0)
        {
            continue;
        }
        vec2 NewPlayerPos;
        NewPlayerPos.X = GameState->Player.X + dPlayer.X * Input->DeltaTime;
        NewPlayerPos.Y = GameState->Player.Y + dPlayer.Y * Input->DeltaTime;

        float PlayerCollisionWidth = PlayerWidth * 0.25f;

        raw_position BottomRight(GameState->PlayerTileMapX, GameState->PlayerTileMapY,
            NewPlayerPos.X + PlayerCollisionWidth, NewPlayerPos.Y - PlayerHeight * 0.1f);

        raw_position BottomLeft(GameState->PlayerTileMapX, GameState->PlayerTileMapY,
            NewPlayerPos.X - PlayerCollisionWidth, NewPlayerPos.Y - PlayerHeight * 0.1f);

        raw_position TopRight(GameState->PlayerTileMapX, GameState->PlayerTileMapY,
            NewPlayerPos.X + PlayerCollisionWidth, NewPlayerPos.Y - PlayerHeight * 0.5f);

        raw_position TopLeft(GameState->PlayerTileMapX, GameState->PlayerTileMapY,
            NewPlayerPos.X - PlayerCollisionWidth, NewPlayerPos.Y - PlayerHeight * 0.5f);

        if (IsWorldPointEmpty(&World, BottomRight) &&
            IsWorldPointEmpty(&World, BottomLeft) &&
            IsWorldPointEmpty(&World, TopRight) &&
            IsWorldPointEmpty(&World, TopLeft))
        {
            raw_position PlayerPos(GameState->PlayerTileMapX, GameState->PlayerTileMapY,
                NewPlayerPos.X, NewPlayerPos.Y);
            canonical_position CanPos = GetCanonicalPosition(&World, PlayerPos);
            GameState->PlayerTileMapX = CanPos.TileMapX;
            GameState->PlayerTileMapY = CanPos.TileMapY;

            GameState->Player.X = CanPos.X;
            GameState->Player.Y = CanPos.Y;
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

            vec2 Min, Max;
            Min.X = World.UpperLeftX + ((float)(Column * World.TileWidth));
            Min.Y = World.UpperLeftY + ((float)(Row * World.TileHeight));
            Max.X = Min.X + World.TileWidth;
            Max.Y = Min.Y + World.TileHeight;

            DrawRectangle(GraphicsBuffer, Min, Max, TileColor);
        }
    }

    color PlayerColor(1.0f, 0.5f, 0.5f);
    vec2 PlayerTopLeft = vec2(GameState->Player.X - 0.5f*PlayerWidth, GameState->Player.Y - PlayerHeight);
    vec2 PlayerBottomRight = vec2(PlayerTopLeft.X + PlayerWidth, PlayerTopLeft.Y + PlayerHeight);
    DrawRectangle(GraphicsBuffer, PlayerTopLeft, PlayerBottomRight, PlayerColor);
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