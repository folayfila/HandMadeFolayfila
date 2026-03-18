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

        GameState->World = PushSize(&GameState->WorldArena, world);
        world* World = GameState->World;

        World->TileMap = PushSize(&GameState->WorldArena, tile_map);
        tile_map* TileMap = World->TileMap;

        TileMap->ChunkShift = 4;
        TileMap->ChunkMask = (1 << TileMap->ChunkShift) - 1; // 256x256
        TileMap->ChunkDim = (1 << TileMap->ChunkShift);

        TileMap->TileSideInMeters = 1.0f;

        TileMap->TileChunkCountX = 128;
        TileMap->TileChunkCountY = 128;
        TileMap->TileChunkCountZ = 2;

        TileMap->TileChunks = PushArray(&GameState->WorldArena, 
            TileMap->TileChunkCountX*TileMap->TileChunkCountY*TileMap->TileChunkCountZ,
            tile_chunk);

        uint32 TilesPerWidth = 17;
        uint32 TilesPerHeight = 9;
        uint32 ScreenY = 0;
        uint32 ScreenX = 0;
        uint32 AbsTileZ = 0;

        bool32 DoorRight = false;
        bool32 DoorLeft = false;
        bool32 DoorTop = false;
        bool32 DoorBottom = false;
        bool32 DoorUp = false;
        bool32 DoorDown = false;
        for (uint32 ScreenIndex = 0; ScreenY < 100; ++ScreenIndex)
        {
            uint32 RandomChoice;
            if (DoorUp || DoorDown)
            {
                RandomChoice = RandomU32() % 2;
            }
            else
            {
                RandomChoice = RandomU32() % 3;
            }

            if (RandomChoice == 2)
            {
                if (AbsTileZ)
                {
                    DoorUp = true;
                }
                else
                {
                    DoorDown = true;
                }
            }
            else if (RandomChoice == 1)
            {
                DoorRight = true;
            }
            else
            {
                DoorTop = true;
            }

            for (uint32 TileY = 0; TileY < TilesPerHeight; ++TileY)
            {
                for (uint32 TileX = 0; TileX < TilesPerWidth; ++TileX)
                {
                    uint32 AbsTileX = ScreenX * TilesPerWidth + TileX;
                    uint32 AbsTileY = ScreenY * TilesPerHeight + TileY;

                    // Note: Glorious code below! 
                    uint32 TileValue = 'd';
                    if (((TileX == 0) && !(TileY == (TilesPerHeight / 2) && DoorLeft)) ||
                        ((TileX == TilesPerWidth - 1) && !(TileY == (TilesPerHeight / 2) && DoorRight)))
                    {
                        TileValue = 'g';
                    }

                    if (((TileY == 0) && !(TileX == (TilesPerWidth / 2) && DoorBottom)) ||
                        ((TileY == TilesPerHeight - 1) && !(TileX == (TilesPerWidth / 2) && DoorTop)))
                    {
                        TileValue = 'g';
                    }

                    if ((TileX == 6) && (TileY == 6))
                    {
                        if (DoorUp)
                        {
                            TileValue = 'i';
                        }
                        if (DoorDown)
                        {
                            TileValue = 'o';
                        }
                    }

                    SetTileValue(&GameState->WorldArena, TileMap, AbsTileX, AbsTileY, AbsTileZ, TileValue);
                }
            }

            DoorLeft = DoorRight;
            DoorBottom = DoorTop;
            DoorRight = false;
            DoorTop = false;
            if (DoorUp)
            {
                DoorDown = true;
                DoorUp = false;
            }
            else if (DoorDown)
            {
                DoorUp = true;
                DoorDown = false;
            }
            else
            {
                DoorUp = false;
                DoorDown = false;
            }

            if (RandomChoice == 2)
            {
                if (AbsTileZ == 0)
                {
                    AbsTileZ = 1;
                }
                else
                {
                    AbsTileZ = 0;
                }
            }
            else if (RandomChoice == 1)
            {
                ++ScreenX;
            }
            else
            {
                ++ScreenY;
            }
        }

        GameMemory->IsInitialized = true;
    }

    tile_map* TileMap = GameState->World->TileMap;

    int32 TileSideInPixels = 60;
    float MetersToPixels = (float)TileSideInPixels / TileMap->TileSideInMeters;

    float PlayerWidth = 0.5f * TileMap->TileSideInMeters;
    float PlayerHeight = 0.5f * TileMap->TileSideInMeters;

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

        static float Speed = 5.0f;
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
        if (ControllerInput->ActionUp.EndedDown)
        {
            Speed = 20.0f;
        }
        if (!ControllerInput->ActionUp.EndedDown)
        {
            Speed = 5.0f;
        }
        dPlayer = (dPlayer * Speed) * Input->DeltaTime;

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

    for (int32 RelRow = -100; RelRow < 100; ++RelRow)
    {
        for (int32 RelColumn = -200; RelColumn < 200; ++RelColumn)
        {
            color TileColor;
            uint32 Column = GameState->PlayerP.AbsTileX + RelColumn;
            uint32 Row = GameState->PlayerP.AbsTileY + RelRow;
            switch (GetTileValue(TileMap, Column, Row, GameState->PlayerP.AbsTileZ))
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
            case 'i':
            {
                // Up stair
                TileColor = color(0.0f, 0.0f, 0.0f);
            } break;
            case 'o':
            {
                // Down stair
                TileColor = color(0.0f, 0.0f, 0.0f);
            } break;
            default:
            {
                // Water
                TileColor = color(0.2f, 0.75f, 0.95f);
            }break;
            }

            if (Column == GameState->PlayerP.AbsTileX && Row == GameState->PlayerP.AbsTileY)
            {
                TileColor = color(1.0f, 0.0f, 0.0f);
            }

            float PlayerOffsetX = MetersToPixels * GameState->PlayerP.TileRelX;
            float PlayerOffsetY = MetersToPixels * GameState->PlayerP.TileRelY;

            vec2 Min, Max;
            Min.X = ScreenCenterX + (float)RelColumn * TileSideInPixels - PlayerOffsetX;
            Min.Y = ScreenCenterY - (float)RelRow * TileSideInPixels + PlayerOffsetY - TileSideInPixels;

            Max.X = Min.X + TileSideInPixels;
            Max.Y = ScreenCenterY - (float)RelRow * TileSideInPixels + PlayerOffsetY;

            DrawRectangle(GraphicsBuffer, Min, Max, TileColor);
        }
    }

    color PlayerColor(1.0f, 0.5f, 0.5f);
    float Left = ScreenCenterX;
    float Bottom = ScreenCenterY - MetersToPixels*PlayerHeight;
    
    DrawRectangle(GraphicsBuffer, vec2(Left, Bottom),
        vec2(Left + MetersToPixels*PlayerWidth, Bottom + MetersToPixels*PlayerHeight),
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