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
    // EndOfBuffer = Buffer->Memory + Buffer->Pitch * Buffer->Height;

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

static void DrawBitmap(game_graphics_buffer* Buffer, loaded_bitmap* Bitmap, float FloatX, float FloatY)
{
    int32 MinX = Clamp32(RoundFloatToInt32(FloatX), 0, Buffer->Width);
    int32 MinY = Clamp32(RoundFloatToInt32(FloatY), 0, Buffer->Height);
    int32 MaxX = Clamp32(RoundFloatToInt32(FloatX + Bitmap->Width), 0, Buffer->Width);
    int32 MaxY = Clamp32(RoundFloatToInt32(FloatY + Bitmap->Height), 0, Buffer->Height);

    int32 BlitWidth = Bitmap->Width;
    int32 BlitHeight = Bitmap->Height;

    uint32* SourceRow = Bitmap->Pixels + BlitWidth * (BlitHeight - 1);
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
            float R = DesR + A * (SrcR - DesR);   // Or R = (1-A)*SR + (A*DR)
            float G = DesG + A * (SrcG - DesG);
            float B = DesB + A * (SrcB - DesB);

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

static void InitialzeArena(memory_arena* Arena, size_t ArenaSize, uint8* Storage)
{
    Arena->Size = ArenaSize;
    Arena->Base = Storage;
    Arena->Used = 0;
}

#pragma pack(push, 1)
struct bitmap_header
{
    uint16 FileType;        /* File type, always 4D42h ("BM") */
    uint32 FileSize;        /* Size of the file in bytes */
    uint16 Reserved1;       /* Always 0 */
    uint16 Reserved2;       /* Always 0 */
    uint32 BitmapOffset;    /* Starting position of image data in bytes */
    uint32 Size;            /* Size of this header in bytes */
    int32 Width;            /* Image width in pixels */
    int32 Height;           /* Image height in pixels */
    uint16  Planes;         /* Number of color planes */
    uint16  BitsPerPixel;   /* Number of bits per pixel */
    uint32 Compression;     /* Compression methods used */
    uint32 SizeOfBitmap;    /* Size of bitmap in bytes */
    int32  HorzResolution;  /* Horizontal resolution in pixels per meter */
    int32  VertResolution;  /* Vertical resolution in pixels per meter */
    uint32 ColorsUsed;      /* Number of colors in the image */
    uint32 ColorsImportant; /* Minimum number of important colors */

    uint32 RedMask;         /* Mask identifying bits of red component */
    uint32 GreenMask;       /* Mask identifying bits of green component */
    uint32 BlueMask;        /* Mask identifying bits of blue component */
};
#pragma pack(pop)

static loaded_bitmap DEBUGLoadBMP(thread_context* Thread, debug_platform_read_entire_file* ReadEntireFile, char* FileName)
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

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender)
{
    Assert((&Input->Controllers[0].Terminator - &Input->Controllers[0].Buttons[0]) ==
        (ArrayCount(Input->Controllers[0].Buttons)));
    Assert(sizeof(game_state) <= GameMemory->PermanentStorageSize);

    game_state* GameState = (game_state*)GameMemory->PermanentStorage;

    if (!GameMemory->IsInitialized)
    {
        GameState->PlayerBMP = DEBUGLoadBMP(Thread, GameMemory->DEBUGPlatformReadEntireFile, "sprites/folayfila_64_0.bmp");

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

    tile_map* TileMap = GameState->World->TileMap;

    int32 TileSideInPixels = 80;
    float MetersToPixels = (float)TileSideInPixels / TileMap->TileSideInMeters;

    float PlayerWidth = (float)GameState->PlayerBMP.Width;
    float PlayerHeight = (float)GameState->PlayerBMP.Height;

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
        PlayerBottomRight.TileRelX += PlayerWidth/ TileSideInPixels;
        PlayerBottomRight = RecanonicalizePosition(TileMap, PlayerBottomRight);

        if (IsTileMapPointEmpty(TileMap, PlayerBottomRight) &&
            IsTileMapPointEmpty(TileMap, NewPlayerP))
        {
            if (!AreOnSameTiles(&GameState->PlayerP, &NewPlayerP))
            {
                uint32 TileValue = GetTileValue(TileMap, &NewPlayerP);
                if (TileValue == 'o')
                {
                    ++NewPlayerP.AbsTileZ;
                }
                else if (TileValue == 'i')
                {
                    --NewPlayerP.AbsTileZ;
                }
            }
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
                TileColor = color(0.5f, 0.5f, 0.5f);
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
                //TileColor = color(1.0f, 0.0f, 0.0f);
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

    float Left = ScreenCenterX;
    float Bottom = ScreenCenterY - PlayerHeight;

    DrawBitmap(GraphicsBuffer, &GameState->PlayerBMP, Left, Bottom);
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