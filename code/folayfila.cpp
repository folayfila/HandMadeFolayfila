// (C) Copyright 2026 by Abdallah Maaliki / folayfila.

#include "folayfila_types.h"
#include "folayfila.h"
#include "folayfila_intrinsics.h"
#include "folayfila_tile.cpp"

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

internal void DrawRectangle(game_graphics_buffer* Buffer, vec2 vMin, vec2 vMax, color Color)
{
    // EndOfBuffer = Buffer->Memory + Buffer->Pitch * Buffer->Height;

    int32 MinX = Clamp32(RoundFloatToInt32(vMin.X), 0, Buffer->Width);
    int32 MaxX = Clamp32(RoundFloatToInt32(vMax.X), 0, Buffer->Width);
    int32 MinY = Clamp32(RoundFloatToInt32(vMin.Y), 0, Buffer->Height);
    int32 MaxY = Clamp32(RoundFloatToInt32(vMax.Y), 0, Buffer->Height);

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

internal void DrawBitmap(game_graphics_buffer* Buffer, loaded_bitmap* Bitmap, float FloatX, float FloatY)
{
    int32 SourceOffsetX = 0;
    int32 MinX = RoundFloatToInt32(FloatX);
    if (MinX < 0)
    {
        SourceOffsetX = -MinX;
        MinX = 0;
    }
    
    int32 SourceOffsetY = 0;
    int32 MinY = RoundFloatToInt32(FloatY);
    if (MinY < 0)
    {
        SourceOffsetY = -MinY;
        MinY = 0;
    }

    int32 MaxX = Clamp32(RoundFloatToInt32(FloatX + Bitmap->Width), MinX, Buffer->Width);
    int32 MaxY = Clamp32(RoundFloatToInt32(FloatY + Bitmap->Height), MinY, Buffer->Height);

    int32 BlitWidth = Bitmap->Width;
    int32 BlitHeight = Bitmap->Height;

    uint32* SourceRow = Bitmap->Pixels + BlitWidth * (BlitHeight - 1);
    SourceRow += -SourceOffsetY*Bitmap->Width + SourceOffsetX; // Fixes clipping on edges.
    uint8* DestRow = ((uint8*)Buffer->Memory + MinX * Buffer->BytesPerPixel + MinY * Buffer->Pitch);

    for (int32 Y = MinY; Y < MaxY; ++Y)
    {
        // Start at the last row and move up.
        uint32* Source = SourceRow;
        uint32* Dest = (uint32*)DestRow;

        for (int32 X = MinX; X < MaxX; ++X)
        {
            float A = (float)((*Source >> 24) & 0xFF) / 255.0f;
            float SrcR = (float)((*Source >> 16) & 0xFF);
            float SrcG = (float)((*Source >> 8) & 0xFF);
            float SrcB = (float)((*Source >> 0) & 0xFF);
            
            float DesR = (float)((*Dest >> 16) & 0xFF);
            float DesG = (float)((*Dest >> 8) & 0xFF);
            float DesB = (float)((*Dest >> 0) & 0xFF);

            // Linear Blend: C = A + t(B - A)
            float R = LinearBlend(DesR, SrcR, A);
            float G = LinearBlend(DesG, SrcG, A);
            float B = LinearBlend(DesB, SrcB, A);

            *Dest = ((RoundFloatToUInt32(R) << 16) |
                     (RoundFloatToUInt32(G) << 8)|
                     (RoundFloatToUInt32(B) << 0));

            ++Dest;
            ++Source;
        }
        DestRow += Buffer->Pitch;
        SourceRow -= BlitWidth;
    }
}

internal void InitialzeArena(memory_arena* Arena, size_t ArenaSize, uint8* Storage)
{
    Arena->Size = ArenaSize;
    Arena->Base = Storage;
    Arena->Used = 0;
}

internal loaded_bitmap DEBUGLoadBMP(thread_context* Thread, debug_platform_read_entire_file* ReadEntireFile, char* FileName)
{
    loaded_bitmap Result = {};
    debug_read_file_result ReadResult = ReadEntireFile(Thread, FileName);
    if (ReadResult.ContentSize != 0)
    {
        bitmap_header* Header = (bitmap_header*)ReadResult.Contents;
        uint32* Pixels = (uint32*)((uint8*)ReadResult.Contents + Header->BitmapOffset);
        Result.Pixels = Pixels;
        Result.Width = Header->Width;
        Result.Height = Header->Height;

        // The only compression mode we load here is 3.
        Assert(Header->Compression == 3)

        uint32 RedMask = Header->RedMask;
        uint32 GreenMask = Header->GreenMask;
        uint32 BlueMask = Header->BlueMask;
        uint32 AlphaMask = ~(RedMask | GreenMask | BlueMask);

        bit_scan_result RedShift = FindLeastSignificantSetBit(RedMask);
        bit_scan_result GreenShift = FindLeastSignificantSetBit(GreenMask);
        bit_scan_result BlueShift = FindLeastSignificantSetBit(BlueMask);
        bit_scan_result AlphaShift = FindLeastSignificantSetBit(AlphaMask);

        Assert(RedShift.Found);
        Assert(GreenShift.Found);
        Assert(BlueShift.Found);
        Assert(AlphaShift.Found);

        uint32* SourceDest = Pixels;
        for (int32 Y = 0; Y < Header->Height; ++Y)
        {
            for (int X = 0; X < Header->Width; ++X)
            {
                uint32 C = *SourceDest;
                *SourceDest++ = ((((C >> AlphaShift.Index) & 0xFF) << 24) |
                    (((C >> RedShift.Index) & 0xFF) << 16) |
                    (((C >> GreenShift.Index) & 0xFF) << 8) |
                    (((C >> BlueShift.Index) & 0xFF) << 0));
            }
        }
    }
    return Result;
}

internal GAME_UPDATE_AND_RENDER(InitializeGame)
{
    game_state* GameState = (game_state*)GameMemory->PermanentStorage;

    if (!GameMemory->IsInitialized)
    {
        GameState->PlayerBMP = DEBUGLoadBMP(Thread, GameMemory->DEBUGPlatformReadEntireFile, "sprites/folayfila_64_0.bmp");

        // >> MAKING FOLAYFILA REEEEEDDDDD!!
        uint32* SourceDest = GameState->PlayerBMP.Pixels;
        for (int32 Y = 0; Y < GameState->PlayerBMP.Height; ++Y)
        {
            for (int X = 0; X < GameState->PlayerBMP.Width; ++X)
            {
                uint32 C = *SourceDest;
                uint32 Red = (C & (0xFF) << 16);
                uint32 Green = (C & (0xFF) << 8);
                uint32 Blue = (C & (0xFF) << 0);

                Red = Green << 8 | Red;
                Green = Blue << 8;

                uint32 NewColorMap = (C & (0xFF << 24)) | Red | Green | Blue;

                *SourceDest++ = NewColorMap;
            }
        }

        GameState->PlayerP.AbsTileX = 2;
        GameState->PlayerP.AbsTileY = 5;
        GameState->PlayerP.Offset = 0.5f;

        InitialzeArena(&GameState->WorldArena, (size_t)(GameMemory->PermanentStorageSize - sizeof(game_state)),
            (uint8*)GameMemory->PermanentStorage + sizeof(game_state));

        GameState->World = PushSize(&GameState->WorldArena, world);
        world* World = GameState->World;

        World->TileMap = PushSize(&GameState->WorldArena, tile_map);
        tile_map* TileMap = World->TileMap;

        TileMap->ChunkShift = 4;
        TileMap->ChunkMask = (1 << TileMap->ChunkShift) - 1; // 256x256
        TileMap->ChunkDim = (1 << TileMap->ChunkShift);

        TileMap->TileSideInMeters = 1.0f;
        TileMap->TileSideInPixels = 60;

        TileMap->TileChunkCountX = 128;
        TileMap->TileChunkCountY = 128;
        TileMap->TileChunkCountZ = 2;

        TileMap->TileChunks = PushArray(&GameState->WorldArena,
            TileMap->TileChunkCountX * TileMap->TileChunkCountY * TileMap->TileChunkCountZ,
            tile_chunk);

        TileMap->TilesPerWidth = 17;
        TileMap->TilesPerHeight = 9;

        GameState->CameraP.AbsTileX = TileMap->TilesPerWidth / 2;
        GameState->CameraP.AbsTileY = TileMap->TilesPerHeight / 2;

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

            bool32 CreatedZDoor = false;
            if (RandomChoice == 2)
            {
                CreatedZDoor = true;
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

            uint32 TilesPerWidth = TileMap->TilesPerWidth;
            uint32 TilesPerHeight = TileMap->TilesPerHeight;
            for (uint32 TileY = 0; TileY < TilesPerHeight; ++TileY)
            {
                for (uint32 TileX = 0; TileX < TilesPerWidth; ++TileX)
                {
                    uint32 AbsTileX = ScreenX * TilesPerWidth + TileX;
                    uint32 AbsTileY = ScreenY * TilesPerHeight + TileY;

                    // Note: Glorious code below! 
                    uint32 TileValue = tile_type::Dirt;
                    if (((TileX == 0) && !(TileY == (TilesPerHeight / 2) && DoorLeft)) ||
                        ((TileX == TilesPerWidth - 1) && !(TileY == (TilesPerHeight / 2) && DoorRight)))
                    {
                        TileValue = tile_type::Grass;
                    }

                    if (((TileY == 0) && !(TileX == (TilesPerWidth / 2) && DoorBottom)) ||
                        ((TileY == TilesPerHeight - 1) && !(TileX == (TilesPerWidth / 2) && DoorTop)))
                    {
                        TileValue = tile_type::Grass;
                    }

                    if ((TileX == 6) && (TileY == 6))
                    {
                        if (DoorUp)
                        {
                            TileValue = tile_type::CaveEntrance;
                        }
                        if (DoorDown)
                        {
                            TileValue = tile_type::CaveExit;
                        }
                    }

                    SetTileValue(&GameState->WorldArena, TileMap, AbsTileX, AbsTileY, AbsTileZ, TileValue);
                }
            }

            DoorLeft = DoorRight;
            DoorBottom = DoorTop;

            if (CreatedZDoor)
            {
                DoorDown = !DoorDown;
                DoorUp = !DoorUp;
            }
            else
            {
                DoorUp = false;
                DoorDown = false;
            }

            DoorRight = false;
            DoorTop = false;

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
}

internal void HandleGameInput(game_state *GameState, game_input *Input)
{
    tile_map* TileMap = GameState->World->TileMap;

    for (int ControllerIndex = 0; ControllerIndex < ArrayCount(Input->Controllers); ++ControllerIndex)
    {
        game_controller_input* ControllerInput = GetController(Input, ControllerIndex);

        if (!ControllerInput->IsConnected)
        {
            continue;
        }

        local_presist float Speed = 5.0f;
        vec2 ddPlayer = {};
        if (ControllerInput->MoveDown.EndedDown)
        {
            ddPlayer.Y -= 1.0f;
        }
        if (ControllerInput->MoveUp.EndedDown)
        {
            ddPlayer.Y += 1.0f;
        }
        if (ControllerInput->MoveRight.EndedDown)
        {
            ddPlayer.X += 1.0f;
        }
        if (ControllerInput->MoveLeft.EndedDown)
        {
            ddPlayer.X -= 1.0f;
        }

        if (ControllerInput->RightShoulder.EndedDown)
        {
            ddPlayer.X += 10.0f;
        }
        if (ControllerInput->LeftShoulder.EndedDown)
        {
            ddPlayer.X -= 10.0f;
        }
        if (ControllerInput->ActionUp.EndedDown)
        {
            Speed = 20.0f;
        }
        else if (ddPlayer.X != 0 && ddPlayer.Y != 0)
        {
            // Using Pythagorean theorem to cancel the extended movement when the player
            // is moving diagonally:
            // A^2 + B^2 = C^2
            // Diagonal means x=y; 2A^2 = C^2
            // A^2 = (C^2)/2 -> A = Sqrt(C^2/2) = Sqrt(1/2)C
            // Sqrt(1/2) = 0.7071067811865475f
            ddPlayer *= 0.7071067811865475f;
        }
        if (!ControllerInput->ActionUp.EndedDown)
        {
            Speed = 10.0f;   // m/s^2
        }
        ddPlayer *= Speed;

        ddPlayer += -1.5f*GameState->dPlayer;

        // Integrating Position and velocity:
        // Position: P' = 1/2AdT^2 + dT*V + P
        tile_map_position NewPlayerP = GameState->PlayerP;
        NewPlayerP.Offset = (0.5f*ddPlayer*Square(Input->DeltaTime) +
                            Input->DeltaTime*GameState->dPlayer +
                            NewPlayerP.Offset);
        // Velocity: V' = AdT + V
        GameState->dPlayer = ddPlayer*Input->DeltaTime + GameState->dPlayer;

        NewPlayerP = RecanonicalizePosition(TileMap, NewPlayerP);

        tile_map_position PlayerBottomRight = NewPlayerP;
        PlayerBottomRight.Offset.X += GameState->PlayerBMP.Width/ (TileMap->TileSideInPixels/TileMap->TileSideInMeters);
        PlayerBottomRight = RecanonicalizePosition(TileMap, PlayerBottomRight);

        if (IsTileMapPointEmpty(TileMap, NewPlayerP) && IsTileMapPointEmpty(TileMap, PlayerBottomRight))
        {
            if (!AreOnSameTiles(&GameState->PlayerP, &NewPlayerP))
            {
                uint32 TileValue = GetTileValue(TileMap, &NewPlayerP);
                if (TileValue == tile_type::CaveExit)
                {
                    ++NewPlayerP.AbsTileZ;
                }
                else if (TileValue == tile_type::CaveEntrance)
                {
                    --NewPlayerP.AbsTileZ;
                }
            }
            GameState->PlayerP = NewPlayerP;
        }
        GameState->CameraP.AbsTileZ = GameState->PlayerP.AbsTileZ;

        tile_map_difference Diff = Subtract(TileMap, &GameState->PlayerP, &GameState->CameraP);
        if (Diff.dXY.X > (((float)TileMap->TilesPerWidth / 2.0f) * TileMap->TileSideInMeters))
        {
            GameState->CameraP.AbsTileX += TileMap->TilesPerWidth;
        }
        else if (Diff.dXY.X < -(((float)TileMap->TilesPerWidth / 2.0f) * TileMap->TileSideInMeters))
        {
            --GameState->CameraP.AbsTileX -= TileMap->TilesPerWidth;
        }

        if (Diff.dXY.Y > (((float)TileMap->TilesPerHeight / 2.0f) * TileMap->TileSideInMeters))
        {
            GameState->CameraP.AbsTileY += TileMap->TilesPerHeight;
        }
        else if (Diff.dXY.Y < -(((float)TileMap->TilesPerHeight / 2.0f) * TileMap->TileSideInMeters))
        {
            GameState->CameraP.AbsTileY -= TileMap->TilesPerHeight;
        }
    }
}

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    Assert((&Input->Controllers[0].Terminator - &Input->Controllers[0].Buttons[0]) ==
        (ArrayCount(Input->Controllers[0].Buttons)));
    Assert(sizeof(game_state) <= GameMemory->PermanentStorageSize);

    if (!GameMemory->IsInitialized)
    {
        InitializeGame(Thread, GameMemory, Input, GraphicsBuffer, SoundBuffer);
    }

    game_state* GameState = (game_state*)GameMemory->PermanentStorage;
    HandleGameInput(GameState, Input);

    /// Drawing code:
    //b 
    tile_map* TileMap = GameState->World->TileMap;

    uint32 TilesPerWidth = TileMap->TilesPerWidth;
    uint32 TilesPerHeight = TileMap->TilesPerHeight;

    int32 TileSideInPixels = TileMap->TileSideInPixels;
    float MetersToPixels = (float)TileSideInPixels / TileMap->TileSideInMeters;

    float ScreenCenterX = 0.5f * (float)GraphicsBuffer->Width - TileSideInPixels/2;
    float ScreenCenterY = 0.5f * (float)GraphicsBuffer->Height + TileSideInPixels/2;

    // Draw Tilemap
    for (int32 RelRow = -100; RelRow < 100; ++RelRow)
    {
        for (int32 RelColumn = -200; RelColumn < 200; ++RelColumn)
        {
            color TileColor = {};
            uint32 Column = GameState->CameraP.AbsTileX + RelColumn;
            uint32 Row = GameState->CameraP.AbsTileY + RelRow;
            switch (GetTileValue(TileMap, Column, Row, GameState->CameraP.AbsTileZ))
            {
            case (tile_type::Dirt):
            {
                TileColor.Set(0.34f, 0.15f, 0.14f);
            } break;
            case (tile_type::Grass):
            {
                TileColor.G = 0.9f;
            } break;
            case (tile_type::CaveEntrance):
            {
                TileColor.Set(0.5f, 0.5f, 0.5f);
            } break;
            case (tile_type::CaveExit):
            {
                TileColor.Set(0.2f, 0.2f, 0.2f);
            } break;
            default:
            {
                // Water
                TileColor.Set(0.2f, 0.75f, 0.95f);
            }break;
            }

            if (Column == GameState->CameraP.AbsTileX && Row == GameState->CameraP.AbsTileY)
            {
                //TileColor.Set(1.0f, 0.0f, 0.0f);
            }

            vec2 CamOffset = MetersToPixels * GameState->CameraP.Offset;

            vec2 Min, Max;
            Min.X = ScreenCenterX + (float)RelColumn * TileSideInPixels - CamOffset.X;
            Min.Y = ScreenCenterY - (float)RelRow * TileSideInPixels + CamOffset.Y - TileSideInPixels;

            Max.X = Min.X + TileSideInPixels;
            Max.Y = ScreenCenterY - (float)RelRow * TileSideInPixels + CamOffset.Y;

            DrawRectangle(GraphicsBuffer, Min, Max, TileColor);
        }
    }

    tile_map_difference Diff = Subtract(TileMap, &GameState->PlayerP, &GameState->CameraP);

    float PlayerGroundPointX = (ScreenCenterX + MetersToPixels*Diff.dXY.X);
    float PlayerGroundPointY = (ScreenCenterY - MetersToPixels * Diff.dXY.Y) - ((float)GameState->PlayerBMP.Height*0.9f);

    // Draw Player
    DrawBitmap(GraphicsBuffer, &GameState->PlayerBMP, PlayerGroundPointX, PlayerGroundPointY);
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