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

inline uint32 GetTileValueUnchecked(tile_map* TileMap, int32 TileX, int32 TileY)
{
    uint32 TileMapValue = TileMap->Tiles[TileY * TileMap->ColumnCount + TileX];
    return TileMapValue;
}

internal bool IsTileMapPointEmpty(tile_map *TileMap, vec2 Test)
{
    bool Empty = false;

    int32 PlayerTileX = TruncateFloatToInt32((Test.X - TileMap->UpperLeftX) / TileMap->TileWidth);
    int32 PlayerTileY = TruncateFloatToInt32((Test.Y - TileMap->UpperLeftY) / TileMap->TileHeight);

    if ((PlayerTileX >= 0) && (PlayerTileX < TileMap->ColumnCount) &&
        (PlayerTileY >= 0) && (PlayerTileY < TileMap->RowCount))
    {
        uint32 Tile = GetTileValueUnchecked(TileMap, PlayerTileX, PlayerTileY);
        if (Tile == 'd')
        {
            Empty = true;
        }
    }
    return Empty;
}

internal bool IsWorldPointEmpty(tile_map* TileMap, vec2 Test)
{
    bool Empty = false;

    int32 PlayerTileX = TruncateFloatToInt32((Test.X - TileMap->UpperLeftX) / TileMap->TileWidth);
    int32 PlayerTileY = TruncateFloatToInt32((Test.Y - TileMap->UpperLeftY) / TileMap->TileHeight);

    if ((PlayerTileX >= 0) && (PlayerTileX < TileMap->ColumnCount) &&
        (PlayerTileY >= 0) && (PlayerTileY < TileMap->RowCount))
    {
        uint32 Tile = GetTileValueUnchecked(TileMap, PlayerTileX, PlayerTileY);
        if (Tile == 'd')
        {
            Empty = true;
        }
    }
    return Empty;
}

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    Assert((&Input->Controllers[0].Terminator - &Input->Controllers[0].Buttons[0]) ==
        (ArrayCount(Input->Controllers[0].Buttons)));
    Assert(sizeof(game_state) <= GameMemory->PermanentStorageSize);


#define TILEMAP_ROW_COUNT (9)
#define TILEMAP_COLUMN_COUNT (17)

    uint32 Tiles0[TILEMAP_ROW_COUNT][TILEMAP_COLUMN_COUNT] =
    {
        {'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w', 'w'},
        {'w', 'g', 'd', 'd',   'd', 'd', 'd', 'd',   'd', 'd', 'g', 'g',   'd', 'd', 'd', 'd', 'w'},
        {'w', 'g', 'd', 'd',   'd', 'w', 'g', 'd',   'd', 'd', 'd', 'd',   'd', 'd', 'd', 'd', 'w'},
        {'w', 'g', 'd', 'd',   'd', 'g', 'g', 'd',   'd', 'g', 'g', 'g',   'g', 'g', 'd', 'd', 'w'},
        {'d', 'd', 'd', 'd',   'g', 'g', 'g', 'd',   'd', 'g', 'g', 'g',   'g', 'g', 'd', 'd', 'd'},
        {'w', 'g', 'd', 'g',   'w', 'w', 'g', 'd',   'g', 'g', 'g', 'd',   'g', 'g', 'g', 'd', 'w'},
        {'w', 'g', 'd', 'g',   'w', 'w', 'g', 'd',   'd', 'd', 'g', 'd',   'd', 'd', 'd', 'd', 'w'},
        {'w', 'g', 'd', 'd',   'd', 'd', 'd', 'd',   'g', 'd', 'd', 'd',   'd', 'd', 'd', 'd', 'w'},
        {'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w', 'w'},
    };

    uint32 Tiles1[TILEMAP_ROW_COUNT][TILEMAP_COLUMN_COUNT] =
    {
        {'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w', 'w'},
        {'w', 'g', 'd', 'd',   'd', 'd', 'd', 'd',   'd', 'd', 'g', 'g',   'g', 'g', 'g', 'g', 'w'},
        {'w', 'g', 'd', 'd',   'd', 'w', 'g', 'd',   'd', 'd', 'd', 'g',   'g', 'g', 'g', 'g', 'w'},
        {'w', 'g', 'd', 'd',   'd', 'g', 'g', 'd',   'd', 'g', 'g', 'g',   'g', 'w', 'w', 'g', 'w'},
        {'d', 'd', 'd', 'd',   'g', 'g', 'g', 'd',   'd', 'g', 'g', 'g',   'g', 'w', 'w', 'g', 'w'},
        {'w', 'g', 'd', 'g',   'w', 'w', 'g', 'd',   'g', 'g', 'g', 'g',   'g', 'w', 'w', 'g', 'w'},
        {'w', 'g', 'd', 'g',   'w', 'w', 'g', 'd',   'd', 'd', 'g', 'g',   'g', 'w', 'w', 'g', 'w'},
        {'w', 'g', 'd', 'd',   'd', 'd', 'd', 'd',   'g', 'd', 'd', 'g',   'g', 'g', 'g', 'g', 'w'},
        {'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w',   'w', 'w', 'w', 'w', 'w'},
    };

    tile_map TileMap[2];
    TileMap[0].ColumnCount = TILEMAP_COLUMN_COUNT;
    TileMap[0].RowCount = TILEMAP_ROW_COUNT;
    TileMap[0].UpperLeftX = -30;
    TileMap[0].UpperLeftY = -30;
    TileMap[0].TileWidth = 60;
    TileMap[0].TileHeight = 60;
    TileMap[0].Tiles = (uint32*)Tiles0;

    TileMap[1] = TileMap[0];
    TileMap[1].Tiles = (uint32*)Tiles1;

    tile_map *CurrentTileMap = &TileMap[1];

    float PlayerWidth = 0.25f * CurrentTileMap->TileWidth;
    float PlayerHeight = 0.25f * CurrentTileMap->TileHeight;

    game_state* GameState = (game_state*)GameMemory->PermanentStorage;

    if (!GameMemory->IsInitialized)
    {
        GameState->Player = vec2(30.0f, 250.0f);
        GameMemory->IsInitialized = true;
    }

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

        vec2 BottomRight(NewPlayerPos.X + PlayerCollisionWidth, NewPlayerPos.Y - PlayerHeight*0.1f);
        vec2 BottomLeft(NewPlayerPos.X - PlayerCollisionWidth, NewPlayerPos.Y - PlayerHeight * 0.1f);
        vec2 TopRight(NewPlayerPos.X + PlayerCollisionWidth, NewPlayerPos.Y - PlayerHeight*0.5f);
        vec2 TopLeft(NewPlayerPos.X - PlayerCollisionWidth, NewPlayerPos.Y - PlayerHeight * 0.5f);

        if (IsTileMapPointEmpty(CurrentTileMap, BottomRight) &&
            IsTileMapPointEmpty(CurrentTileMap, BottomLeft) &&
            IsTileMapPointEmpty(CurrentTileMap, TopRight) &&
            IsTileMapPointEmpty(CurrentTileMap, TopLeft))
        {
            GameState->Player = NewPlayerPos;
        }
    }

    DrawRectangle(GraphicsBuffer, vec2(0.0f), vec2(1000.0f), color(0.0f, 0.0f, 0.0f));

    for (int Row = 0; Row < TILEMAP_ROW_COUNT; ++Row)
    {
        for (int Column = 0; Column < TILEMAP_COLUMN_COUNT; ++Column)
        {
            color TileColor;
            switch (GetTileValueUnchecked(CurrentTileMap, Column, Row))
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
            Min.X = CurrentTileMap->UpperLeftX + ((float)(Column * CurrentTileMap->TileWidth));
            Min.Y = CurrentTileMap->UpperLeftY + ((float)(Row * CurrentTileMap->TileHeight));
            Max.X = Min.X + CurrentTileMap->TileWidth;
            Max.Y = Min.Y + CurrentTileMap->TileHeight;

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