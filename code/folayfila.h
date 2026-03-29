// (C) Copyright 2026 by Abdallah Maaliki / folayfila.

#if !defined(FOLAYFILA_H)
#define FOLAYFILA_H

#include "folayfila_types.h"
#include "folayfila_tile.h"

inline int StringLength(char* String)
{
    int Count = 0;
    while (*String++)
    {
        ++Count;
    }
    return Count;
}

inline void CatStrings(size_t SourceACount, char* SourceA,
    size_t SourceBCount, char* SourceB,
    size_t DestCount, char* Dest)
{
    for (size_t Index = 0; Index < SourceACount; ++Index)
    {
        *Dest++ = *SourceA++;
    }

    for (size_t Index = 0; Index < SourceBCount; ++Index)
    {
        *Dest++ = *SourceB++;
    }

    *Dest++ = 0;
}
/**************************************/

/************** structs ***************/
struct vec2
{
    vec2() {}
    vec2(int x, int y)
    {
        X = (float)x;
        Y = (float)y;
    }
    vec2(float vec)
    {
        X = vec;
        Y = vec;
    }
    vec2(float x, float y)
    {
        X = x;
        Y = y;
    }

    vec2 operator*(float Scalar) const
    {
        vec2 Result;
        Result.X = X * Scalar;
        Result.Y = Y * Scalar;
        return Result;
    }

    vec2& operator=(const vec2& Other)
    {
        if (this != &Other)
        {
            X = Other.X;
            Y = Other.Y;
        }
        return *this;
    }
    vec2& operator+=(const vec2& Other)
    {
        if (this != &Other)
        {
            X += Other.X;
            Y += Other.Y;
        }
        return *this;
    }
    float X;
    float Y;
};

struct color
{
    color() {}
    color(float r, float g, float b, bool32 Is255 = false)
    {
        if (Is255)
        {
            R = r / 255.0f;
            G = g / 255.0f;
            B = b / 255.0f;
        }
        else
        {
            R = r;
            G = g;
            B = b;
        }
    }
    float R;
    float G;
    float B;
};

struct game_graphics_buffer
{
	void* Memory;
    int Width;
    int Height;
    int Pitch;
    int BytesPerPixel;
};

struct game_output_sound_buffer
{
    int SamplesPerSecond;
    int SampleCount;
    int16 *Samples;
};


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

struct game_button_state
{
    int HalfTransitionCount;
    bool32 EndedDown;
};

struct game_controller_input
{
    bool32 IsConnected;
    bool32 IsAnalog;
    vec2 StickAverage;

    union
    {
        game_button_state Buttons[12];
        struct
        {
            game_button_state MoveUp;
            game_button_state MoveDown;
            game_button_state MoveLeft;
            game_button_state MoveRight;

            game_button_state ActionUp;
            game_button_state ActionDown;
            game_button_state ActionLeft;
            game_button_state ActionRight;
            
            game_button_state LeftShoulder;
            game_button_state RightShoulder;

            game_button_state Start;
            game_button_state Back;

            //? All buttons must be added above this one.
            game_button_state Terminator;
        };
    };
};

struct game_input
{
    game_button_state MouseButtons[5];
    int32 MouseX, MouseY, MouseZ;

    float DeltaTime;
    game_controller_input Controllers[5];
};

inline game_controller_input* GetController(game_input *Input, int ControllerIndex)
{ 
    Assert(ControllerIndex < ArrayCount(Input->Controllers));
    return &Input->Controllers[ControllerIndex];
}

struct thread_context
{
    int PlaceHolder;
};


#if FOLAYFILA_INTERNAL
//! IMPORTANT: These are not for shipping!
struct debug_read_file_result
{
    uint32 ContentSize;
    void* Contents;
};

#define DEBUG_PLATFORM_FREE_FILE_MEMORY(name) void name(thread_context *Thread, void *Memeory)
typedef DEBUG_PLATFORM_FREE_FILE_MEMORY(debug_platform_free_file_memory);

#define DEBUG_PLATFORM_READ_ENTIRE_FILE(name) debug_read_file_result name(thread_context *Thread, char *FileName)
typedef DEBUG_PLATFORM_READ_ENTIRE_FILE(debug_platform_read_entire_file);

#define DEBUG_PLATFORM_WRITE_ENTIRE_FILE(name) bool32 name(thread_context *Thread, char *FileName, uint32 MemorySize, void *Memory)
typedef DEBUG_PLATFORM_WRITE_ENTIRE_FILE(debug_platform_write_entire_file);

DEBUG_PLATFORM_FREE_FILE_MEMORY(DEBUGPlatformFreeFileMemory);
DEBUG_PLATFORM_READ_ENTIRE_FILE(DEBUGPlatformReadEntireFile);
DEBUG_PLATFORM_WRITE_ENTIRE_FILE(DEBUGPlatformWriteEntireFile);
#endif  // FOLAYFILA_INTERNAL

struct game_memory
{
    bool32 IsInitialized;

    uint64 PermanentStorageSize;
    void* PermanentStorage; // REQUIRED to be cleared to zero at startup.

    uint64 TransientStorageSize;
    void* TransientStorage; // REQUIRED to be cleared to zero at startup.

    debug_platform_free_file_memory *DEBUGPlatformFreeFileMemory;
    debug_platform_read_entire_file *DEBUGPlatformReadEntireFile;
    debug_platform_write_entire_file *DEBUGPlatformWriteEntireFile;
};
/**************************************/
#define GAME_UPDATE_AND_RENDER(name) void name(thread_context *Thread, game_memory* GameMemory, game_input* Input, game_graphics_buffer* GraphicsBuffer, game_output_sound_buffer* SoundBuffer)
typedef GAME_UPDATE_AND_RENDER(game_update_and_render);
///
///
///
/// 

struct memory_arena
{
    size_t Size;
    uint8* Base;
    size_t Used;
};

struct world
{
    tile_map* TileMap;
};

struct loaded_bitmap
{
    int32 Width;
    int32 Height;
    uint32* Pixels;
};

struct game_state
{
    memory_arena WorldArena;
    world* World;
    tile_map_position CameraP;
    tile_map_position PlayerP;

    loaded_bitmap PlayerBMP;
};


#define PushSize(Arena, type) (type *)PushSize_(Arena, sizeof(type))
#define PushArray(Arena, Count, type) (type *)PushSize_(Arena, (Count)*sizeof(type))
void* PushSize_(memory_arena* Arena, size_t Size)
{
    Assert((Arena->Used + Size) <= Arena->Size);
    void* Result = Arena->Base + Arena->Used;
    Arena->Used += Size;

    return Result;
}


#endif  // FOLAYFILA_H