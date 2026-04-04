// (C) Copyright 2026 by Abdallah Maaliki / folayfila.

#include "folayfila_types.h"
#include "folayfila.h"
#include "folayfila_intrinsics.h"
#include "folayfila_tile.cpp"

internal void GameOutputSound(game_state* GameState, game_output_sound_buffer* SoundBuffer)
{
    int16 toneVolume = 3000;
    int16 toneHz = 256;
    local_presist float tSine = 0.0f;
    int wavePeriod = SoundBuffer->SamplesPerSecond / toneHz;

    int16* sampleOut = SoundBuffer->Samples;
    for (int sampleIndex = 0; sampleIndex < SoundBuffer->SampleCount; ++sampleIndex)
    {
#if 0
        float sineValue = sinf(GameState->tSine);
        int16 sampleValue = (int16)(SineValue * ToneVolume);
#else
        int16 sampleValue = 0;     
#endif        
        *sampleOut++ = sampleValue;
        *sampleOut++ = sampleValue;

        tSine += 2.0f * Pi32 * 1.0f / (float)wavePeriod;
    }
}

internal void DrawRectangle(game_graphics_buffer* Buffer, vec2 vMin, vec2 vMax, color Color)
{
    // EndOfBuffer = Buffer->Memory + Buffer->Pitch * Buffer->Height;

    int32 minX = Clamp32(RoundFloatToInt32(vMin.X), 0, Buffer->Width);
    int32 maxX = Clamp32(RoundFloatToInt32(vMax.X), 0, Buffer->Width);
    int32 minY = Clamp32(RoundFloatToInt32(vMin.Y), 0, Buffer->Height);
    int32 maxY = Clamp32(RoundFloatToInt32(vMax.Y), 0, Buffer->Height);

    uint8* row = ((uint8*)Buffer->Memory + minX * Buffer->BytesPerPixel + minY * Buffer->Pitch);
    
    uint32 color32 = ((RoundFloatToUInt32(Color.R * 255) << 16) |
        (RoundFloatToUInt32(Color.G * 255) << 8) |
        (RoundFloatToUInt32(Color.B * 255) << 0));

    for (int y = minY; y < maxY; ++y)
    {
        uint32* pixel = (uint32*)row;
        
        for (int x = minX; x < maxX; ++x)
        {
            *pixel++ = color32;
        }
        row += Buffer->Pitch;
    }
}

internal void DrawBitmap(game_graphics_buffer* Buffer, loaded_bitmap* Bitmap, float FloatX, float FloatY)
{
    int32 sourceOffsetX = 0;
    int32 minX = RoundFloatToInt32(FloatX);
    if (minX < 0)
    {
        sourceOffsetX = -minX;
        minX = 0;
    }
    
    int32 sourceOffsetY = 0;
    int32 minY = RoundFloatToInt32(FloatY);
    if (minY < 0)
    {
        sourceOffsetY = -minY;
        minY = 0;
    }

    int32 maxX = Clamp32(RoundFloatToInt32(FloatX + Bitmap->Width), minX, Buffer->Width);
    int32 maxY = Clamp32(RoundFloatToInt32(FloatY + Bitmap->Height), minY, Buffer->Height);

    int32 blitWidth = Bitmap->Width;
    int32 blitHeight = Bitmap->Height;

    uint32* sourceRow = Bitmap->Pixels + blitWidth * (blitHeight - 1);
    sourceRow += -sourceOffsetY*Bitmap->Width + sourceOffsetX; // Fixes clipping on edges.
    uint8* destRow = ((uint8*)Buffer->Memory + minX * Buffer->BytesPerPixel + minY * Buffer->Pitch);

    for (int32 y = minY; y < maxY; ++y)
    {
        // Start at the last row and move up.
        uint32* source = sourceRow;
        uint32* dest = (uint32*)destRow;

        for (int32 X = minX; X < maxX; ++X)
        {
            float alpha = (float)((*source >> 24) & 0xFF) / 255.0f;
            float srcR = (float)((*source >> 16) & 0xFF);
            float srcG = (float)((*source >> 8) & 0xFF);
            float srcB = (float)((*source >> 0) & 0xFF);
            
            float desR = (float)((*dest >> 16) & 0xFF);
            float desG = (float)((*dest >> 8) & 0xFF);
            float desB = (float)((*dest >> 0) & 0xFF);

            // Linear Blend: C = A + t(B - A)
            float red = LinearBlend(desR, srcR, alpha);
            float green = LinearBlend(desG, srcG, alpha);
            float blue = LinearBlend(desB, srcB, alpha);

            *dest = ((RoundFloatToUInt32(red)   << 16) |
                     (RoundFloatToUInt32(green) << 8)  |
                     (RoundFloatToUInt32(blue)  << 0));

            ++dest;
            ++source;
        }
        destRow += Buffer->Pitch;
        sourceRow -= blitWidth;
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
    loaded_bitmap result = {};
    debug_read_file_result readResult = ReadEntireFile(Thread, FileName);
    if (readResult.ContentSize != 0)
    {
        bitmap_header* header = (bitmap_header*)readResult.Contents;
        uint32* pixels = (uint32*)((uint8*)readResult.Contents + header->BitmapOffset);
        result.Pixels = pixels;
        result.Width = header->Width;
        result.Height = header->Height;

        // The only compression mode we load here is 3.
        Assert(header->Compression == 3)

        uint32 redMask = header->RedMask;
        uint32 greenMask = header->GreenMask;
        uint32 blueMask = header->BlueMask;
        uint32 alphaMask = ~(redMask | greenMask | blueMask);

        bit_scan_result redShift = FindLeastSignificantSetBit(redMask);
        bit_scan_result greenShift = FindLeastSignificantSetBit(greenMask);
        bit_scan_result blueShift = FindLeastSignificantSetBit(blueMask);
        bit_scan_result alphaShift = FindLeastSignificantSetBit(alphaMask);

        Assert(redShift.Found);
        Assert(greenShift.Found);
        Assert(blueShift.Found);
        Assert(alphaShift.Found);

        uint32* sourceDest = pixels;
        for (int32 Y = 0; Y < header->Height; ++Y)
        {
            for (int X = 0; X < header->Width; ++X)
            {
                uint32 C = *sourceDest;
                *sourceDest++ = ((((C >> alphaShift.Index) & 0xFF) << 24) |
                    (((C >> redShift.Index) & 0xFF) << 16) |
                    (((C >> greenShift.Index) & 0xFF) << 8) |
                    (((C >> blueShift.Index) & 0xFF) << 0));
            }
        }
    }
    return result;
}

internal rect CreateRectFromPoint(tile_map *TileMap, tile_map_position BottomLeft, float Width, float Height)
{
    rect result;

    result.Width = Width;
    result.Height = Height;

    tile_map_position topLeft = BottomLeft;
    topLeft.Offset.Y += Height;
    tile_map_position topRight = topLeft;
    topRight.Offset.X += Width;
    tile_map_position bottomRight = topRight;
    bottomRight.Offset.Y -= Height;

    result.Points[0] = RecanonicalizePosition(TileMap, BottomLeft);
    result.Points[1] = RecanonicalizePosition(TileMap, topLeft);
    result.Points[2] = RecanonicalizePosition(TileMap, topRight);
    result.Points[3] = RecanonicalizePosition(TileMap, bottomRight);

    return result;
}

internal entity* GetEntity(game_state* GameState, uint32 Index)
{
    entity* Entity = 0;

    if ((Index >= 0) && (Index < ArrayCount(GameState->Entities)))
    {
        Entity = &GameState->Entities[Index];
    }

    return Entity;
}

internal void AddPlayer(game_state* GameState, entity* Entity)
{
    *Entity = {};

    Entity->Exists = true;
    Entity->Pos.AbsTileX = 2;
    Entity->Pos.AbsTileY = 5;
    Entity->Pos.Offset = 0.5f;

    GameState->CurrentNumberOfPlayers++;
}

internal void RemovePlayer(game_state* GameState, entity* Entity)
{
    Entity->Exists = true;
    *Entity = {};

    GameState->CurrentNumberOfPlayers--;
}

internal GAME_UPDATE_AND_RENDER(InitializeGame)
{
    game_state* gameState = (game_state*)GameMemory->PermanentStorage;

    if (!GameMemory->IsInitialized)
    {
        gameState->CameraFollowingEntityIndex = 0;
        gameState->CurrentNumberOfPlayers = 1;

        entity* player = GetEntity(gameState, 0);
        if (player)
        {
            AddPlayer(gameState, player);
        }

        // > TODO: Remove this code!
        // Temp code just to change the color of ma bois
        for (uint32 i = 0; i < 4; ++i)
        {
            gameState->PlayerBMP[i] = DEBUGLoadBMP(Thread, GameMemory->DEBUGPlatformReadEntireFile, "sprites/folayfila_64_0.bmp");

            // >> MAKING FOLAYFILA REEEEEDDDDD!!
            // > very poor code, just doing it for the hell of it
            uint32* SourceDest = gameState->PlayerBMP[i].Pixels;
            for (int32 Y = 0; Y < gameState->PlayerBMP[i].Height; ++Y)
            {
                for (int X = 0; X < gameState->PlayerBMP[i].Width; ++X)
                {
                    uint32 C = *SourceDest;
                    uint32 Red = (C & (0xFF) << 16);
                    uint32 Green = (C & (0xFF) << 8);
                    uint32 Blue = (C & (0xFF) << 0);

                    if (i == 1)
                    {
                        // red folayfila
                        Red = Green << 8 | Red;
                        Green = Blue << 8;
                    }
                    if (i == 2)
                    {
                        // black folayfila
                        Green = Blue << 8;
                    }
                    if (i == 3)
                    {
                        // blue folayfila
                        Blue += 200;
                    }


                    uint32 NewColorMap = (C & (0xFF << 24)) | Red | Green | Blue;

                    *SourceDest++ = NewColorMap;
                }
            }
        }

        InitialzeArena(&gameState->WorldArena, (size_t)(GameMemory->PermanentStorageSize - sizeof(game_state)),
            (uint8*)GameMemory->PermanentStorage + sizeof(game_state));

        gameState->World = PushSize(&gameState->WorldArena, world);
        world* world = gameState->World;

        world->TileMap = PushSize(&gameState->WorldArena, tile_map);
        tile_map* tileMap = world->TileMap;

        tileMap->ChunkShift = 4;
        tileMap->ChunkMask = (1 << tileMap->ChunkShift) - 1; // 256x256
        tileMap->ChunkDim = (1 << tileMap->ChunkShift);

        tileMap->TileSideInMeters = 1.0f;
        tileMap->TileSideInPixels = 60;

        tileMap->TileChunkCountX = 128;
        tileMap->TileChunkCountY = 128;
        tileMap->TileChunkCountZ = 2;

        tileMap->TileChunks = PushArray(&gameState->WorldArena,
            tileMap->TileChunkCountX * tileMap->TileChunkCountY * tileMap->TileChunkCountZ,
            tile_chunk);

        tileMap->TilesPerWidth = 17;
        tileMap->TilesPerHeight = 9;

        gameState->CameraP.AbsTileX = tileMap->TilesPerWidth / 2;
        gameState->CameraP.AbsTileY = tileMap->TilesPerHeight / 2;

        uint32 screenY = 0;
        uint32 screenX = 0;
        uint32 absTileZ = 0;

        bool32 doorRight = false;
        bool32 doorLeft = false;
        bool32 doorTop = false;
        bool32 doorBottom = false;
        bool32 doorUp = false;
        bool32 doorDown = false;
        for (uint32 screenIndex = 0; screenY < 100; ++screenIndex)
        {
            uint32 randomChoice;
            if (doorUp || doorDown)
            {
                randomChoice = RandomU32() % 2;
            }
            else
            {
                randomChoice = RandomU32() % 3;
            }

            bool32 createdZDoor = false;
            if (randomChoice == 2)
            {
                createdZDoor = true;
                if (absTileZ)
                {
                    doorUp = true;
                }
                else
                {
                    doorDown = true;
                }
            }
            else if (randomChoice == 1)
            {
                doorRight = true;
            }
            else
            {
                doorTop = true;
            }

            uint32 tilesPerWidth = tileMap->TilesPerWidth;
            uint32 tilesPerHeight = tileMap->TilesPerHeight;
            for (uint32 tileY = 0; tileY < tilesPerHeight; ++tileY)
            {
                for (uint32 tileX = 0; tileX < tilesPerWidth; ++tileX)
                {
                    uint32 absTileX = screenX * tilesPerWidth + tileX;
                    uint32 absTileY = screenY * tilesPerHeight + tileY;

                    // Note: Glorious code below! 
                    uint32 tileValue = tile_type::Dirt;
                    if (((tileX == 0) && !(tileY == (tilesPerHeight / 2) && doorLeft)) ||
                        ((tileX == tilesPerWidth - 1) && !(tileY == (tilesPerHeight / 2) && doorRight)))
                    {
                        tileValue = tile_type::Grass;
                    }

                    if (((tileY == 0) && !(tileX == (tilesPerWidth / 2) && doorBottom)) ||
                        ((tileY == tilesPerHeight - 1) && !(tileX == (tilesPerWidth / 2) && doorTop)))
                    {
                        tileValue = tile_type::Grass;
                    }

                    if ((tileX == 6) && (tileY == 6))
                    {
                        if (doorUp)
                        {
                            tileValue = tile_type::CaveEntrance;
                        }
                        if (doorDown)
                        {
                            tileValue = tile_type::CaveExit;
                        }
                    }

                    SetTileValue(&gameState->WorldArena, tileMap, absTileX, absTileY, absTileZ, tileValue);
                }
            }

            doorLeft = doorRight;
            doorBottom = doorTop;

            if (createdZDoor)
            {
                doorDown = !doorDown;
                doorUp = !doorUp;
            }
            else
            {
                doorUp = false;
                doorDown = false;
            }

            doorRight = false;
            doorTop = false;

            if (randomChoice == 2)
            {
                if (absTileZ == 0)
                {
                    absTileZ = 1;
                }
                else
                {
                    absTileZ = 0;
                }
            }
            else if (randomChoice == 1)
            {
                ++screenX;
            }
            else
            {
                ++screenY;
            }
        }

        GameMemory->IsInitialized = true;
    }
}

internal vec2 ClosestPointInRectangle(vec2 MinCorner, vec2 MaxCorner, vec2 Point)
{
    vec2 result = {};

    return result;
}

internal void HandleGameInput(game_state* GameState, game_input* Input)
{
    tile_map* tileMap = GameState->World->TileMap;

    for (int controllerIndex = 0; controllerIndex < ArrayCount(Input->Controllers); ++controllerIndex)
    {
        game_controller_input* controllerInput = GetController(Input, controllerIndex);

        if (!controllerInput->IsConnected)
        {
            continue;
        }

        entity* player = GetEntity(GameState, controllerIndex);
        if (!player)
        {
            continue;
        }

        if (player->Exists)
        {
            tile_map_position oldPlayerP = player->Pos;

            vec2 acceleration = {};
            if (controllerInput->MoveDown.EndedDown)
            {
                acceleration.Y -= 1.0f;
            }
            if (controllerInput->MoveUp.EndedDown)
            {
                acceleration.Y += 1.0f;
            }
            if (controllerInput->MoveRight.EndedDown)
            {
                acceleration.X += 1.0f;
            }
            if (controllerInput->MoveLeft.EndedDown)
            {
                acceleration.X -= 1.0f;
            }
            if (controllerInput->RightShoulder.EndedDown)
            {
                acceleration.X += 10.0f;
            }
            if (controllerInput->LeftShoulder.EndedDown)
            {
                acceleration.X -= 10.0f;
            }

            float speed = controllerInput->ActionDown.EndedDown ? 20.0f : 10.0f;
            if (acceleration.X != 0 && acceleration.Y != 0)
            {
                acceleration *= 0.7071067811865475f;
            }
            acceleration *= speed;

            acceleration += -0.5f * player->Pos.Offset;

            // Integrating Position and velocity:
            // Position: P' = 1/2AdT^2 + dT*V + P
            tile_map_position newPlayerP = player->Pos;

            vec2 playerDelta = (0.5f * acceleration * Square(Input->DeltaTime) +
                Input->DeltaTime * player->Velocity);
            newPlayerP.Offset += playerDelta;
            // Velocity: V' = AdT + V
            player->Velocity += acceleration * Input->DeltaTime;
            newPlayerP = RecanonicalizePosition(tileMap, newPlayerP);

#if 0

            float playerWidth = (GameState->PlayerBMP.Width / (tileMap->TileSideInPixels / tileMap->TileSideInMeters)) * 0.5f;
            float playerHeight = (GameState->PlayerBMP.Height / (tileMap->TileSideInPixels / tileMap->TileSideInMeters)) * 0.5f;
            rect playerBounds = CreateRectFromPoint(tileMap, newPlayerP, playerWidth, playerHeight);
            newPlayerP = playerBounds.Points[0];

            bool32 collided = false;
            tile_map_position ColP = {};
            for (int32 pointIndex = 0; pointIndex < 4; ++pointIndex)
            {
                if (!IsTileMapPointEmpty(tileMap, playerBounds.Points[pointIndex]))
                {
                    collided = true;
                    ColP = playerBounds.Points[pointIndex];
                }
            }
            if (collided)
            {
                vec2 reflectionVector = {};
                if (player->Pos.AbsTileX > ColP.AbsTileX)
                {
                    reflectionVector = { 1, 0 };
                }
                if (player->Pos.AbsTileX < ColP.AbsTileX)
                {
                    reflectionVector = { -1, 0 };
                }
                if (player->Pos.AbsTileY > ColP.AbsTileY)
                {
                    reflectionVector = { 0, 1 };
                }
                if (player->Pos.AbsTileY < ColP.AbsTileY)
                {
                    reflectionVector = { 0, -1 };
                }

                player->Velocity = player->Velocity - 2 * Dot(player->Velocity, reflectionVector) * reflectionVector;
            }
#else
            uint32 minTileX = 0;
            uint32 minTileY = 0;
            uint32 onePastMaxTileX = 0;
            uint32 onePastMaxTileY = 0;
            uint32 absTileZ = player->Pos.AbsTileZ;
            tile_map_position bestPlayerP = newPlayerP;
            float bestDistanceSq = LengthSq(playerDelta);
            for (uint32 absTileY = minTileY; absTileY != onePastMaxTileY; ++absTileY)
            {
                for (uint32 absTileX = minTileX; absTileX != onePastMaxTileX; ++absTileX)
                {
                    tile_map_position testTileP = CenteredTilePoint(absTileX, absTileY, absTileZ);
                    uint32 tileValue = GetTileValue(tileMap, absTileX, absTileY, absTileZ);
                    if (IsTileValueEmpty(tileValue))
                    {
                        vec2 minCorner = -0.5 * vec2{ tileMap->TileSideInMeters, tileMap->TileSideInMeters };
                        vec2 maxCorner = 0.5 * vec2{ tileMap->TileSideInMeters, tileMap->TileSideInMeters };

                        vec3 relNewPlayerP = Subtract(tileMap, &testTileP, &newPlayerP);
                        vec2 testP = ClosestPointInRectangle(minCorner, maxCorner, vec2{ relNewPlayerP.X, relNewPlayerP.Y });
                    }
                }
            }

            if (IsTileMapPointEmpty(tileMap, newPlayerP))
            {
                player->Pos = newPlayerP;
            }
#endif
            if (!AreOnSameTiles(&oldPlayerP, &player->Pos))
            {
                uint32 tileValue = GetTileValue(tileMap, &player->Pos);
                if (tileValue == tile_type::CaveExit)
                {
                    ++player->Pos.AbsTileZ;
                }
                else if (tileValue == tile_type::CaveEntrance)
                {
                    --player->Pos.AbsTileZ;
                }
            }
        }
        else
        {
            if (controllerInput->Start.EndedDown)
            {
                AddPlayer(GameState, player);
                player->Exists = true;
            }
        }

        entity* CameraFollowingEntity = GetEntity(GameState, GameState->CameraFollowingEntityIndex);
        if (CameraFollowingEntity)
        {
            GameState->CameraP.AbsTileZ = CameraFollowingEntity->Pos.AbsTileZ;

            vec3 diff = Subtract(tileMap, &CameraFollowingEntity->Pos, &GameState->CameraP);
            if (diff.X > (((float)tileMap->TilesPerWidth / 2.0f) * tileMap->TileSideInMeters))
            {
                GameState->CameraP.AbsTileX += tileMap->TilesPerWidth;
            }
            else if (diff.X < -(((float)tileMap->TilesPerWidth / 2.0f) * tileMap->TileSideInMeters))
            {
                GameState->CameraP.AbsTileX -= tileMap->TilesPerWidth;
            }

            if (diff.Y > (((float)tileMap->TilesPerHeight / 2.0f) * tileMap->TileSideInMeters))
            {
                GameState->CameraP.AbsTileY += tileMap->TilesPerHeight;
            }
            else if (diff.Y < -(((float)tileMap->TilesPerHeight / 2.0f) * tileMap->TileSideInMeters))
            {
                GameState->CameraP.AbsTileY -= tileMap->TilesPerHeight;
            }
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

    game_state* gameState = (game_state*)GameMemory->PermanentStorage;
    HandleGameInput(gameState, Input);

    /// Drawing code:
    //b 
    tile_map* tileMap = gameState->World->TileMap;

    uint32 tilesPerWidth = tileMap->TilesPerWidth;
    uint32 tilesPerHeight = tileMap->TilesPerHeight;

    int32 tileSideInPixels = tileMap->TileSideInPixels;
    float metersToPixels = (float)tileSideInPixels / tileMap->TileSideInMeters;

    float screenCenterX = 0.5f * (float)GraphicsBuffer->Width - tileSideInPixels / 2;
    float screenCenterY = 0.5f * (float)GraphicsBuffer->Height + tileSideInPixels / 2;

    // Draw Tilemap
    for (int32 relRow = -100; relRow < 100; ++relRow)
    {
        for (int32 relColumn = -200; relColumn < 200; ++relColumn)
        {
            color tileColor = {};
            uint32 column = gameState->CameraP.AbsTileX + relColumn;
            uint32 row = gameState->CameraP.AbsTileY + relRow;
            switch (GetTileValue(tileMap, column, row, gameState->CameraP.AbsTileZ))
            {
            case (tile_type::Dirt):
            {
                tileColor.Set(0.34f, 0.15f, 0.14f);
            } break;
            case (tile_type::Grass):
            {
                tileColor.G = 0.9f;
            } break;
            case (tile_type::CaveEntrance):
            {
                tileColor.Set(0.5f, 0.5f, 0.5f);
            } break;
            case (tile_type::CaveExit):
            {
                tileColor.Set(0.2f, 0.2f, 0.2f);
            } break;
            default:
            {
                // Water
                tileColor.Set(0.2f, 0.75f, 0.95f);
            }break;
            }

            if (column == gameState->CameraP.AbsTileX && row == gameState->CameraP.AbsTileY)
            {
                //TileColor.Set(1.0f, 0.0f, 0.0f);
            }

            vec2 camOffset = metersToPixels * gameState->CameraP.Offset;

            vec2 min, max;
            min.X = screenCenterX + (float)relColumn * tileSideInPixels - camOffset.X;
            min.Y = screenCenterY - (float)relRow * tileSideInPixels + camOffset.Y - tileSideInPixels;

            max.X = min.X + tileSideInPixels;
            max.Y = screenCenterY - (float)relRow * tileSideInPixels + camOffset.Y;

            DrawRectangle(GraphicsBuffer, min, max, tileColor);
        }
    }

    for (uint32 playerIndex = 0; playerIndex < gameState->CurrentNumberOfPlayers; ++playerIndex)
    {
        entity* player = GetEntity(gameState, playerIndex);
        if (!player || !player->Exists)
        {
            continue;
        }

        vec3 diff = Subtract(tileMap, &player->Pos, &gameState->CameraP);
        float playerGroundPointX = (screenCenterX + metersToPixels * diff.X);
        float playerGroundPointY = (screenCenterY - metersToPixels * diff.Y) - ((float)gameState->PlayerBMP[playerIndex].Height * 0.9f);

        // Draw Player
        DrawBitmap(GraphicsBuffer, &gameState->PlayerBMP[playerIndex], playerGroundPointX, playerGroundPointY);
    }
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