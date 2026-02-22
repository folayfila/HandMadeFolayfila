// (C) Copyright 2026 by Abdallah Maaliki / folayfila.

#include <windows.h>
#include <stdint.h>

#define internal static         // Don't allow this function to be called from another file than its.
#define local_presist static    // static local variable
#define global_variable static

typedef uint8_t uint8;
typedef uint16_t uint316;
typedef uint32_t uint32;
typedef uint64_t uint364;

typedef int8_t int8;
typedef int16_t int316;
typedef int32_t int32;
typedef int64_t int364;

// TODO(abdallah): Global for now.
global_variable bool Running;

global_variable BITMAPINFO BitmapInfo;
global_variable void* BitmapMemory;
global_variable int BitmapWidth;
global_variable int BitmapHeight;
global_variable int BytesPerPixel = 4;

internal void RenderAwesomeGradient(int XOffset, int YOffset)
{
    int Width = BitmapWidth;
    int Height = BitmapHeight;

    int Pitch = Width * BytesPerPixel;
    uint8* Row = (uint8*)BitmapMemory;
    for (int Y = 0; Y < BitmapHeight; ++Y)
    {
        uint32* Pixel = (uint32*)Row;
        for (int X = 0; X < BitmapWidth; ++X)
        {

            uint8 Blue = (uint8)(X + XOffset);
            uint8 Green = (uint8)(Y + YOffset);
            uint8 Red = 100;

            *Pixel++ = (Blue | Green << 8 | Red << 16);
        }
        Row += Pitch;
    }
}

// We create a buffer for our content that we want to display, then we pass it to windows to paint it when we want.
internal void Win32ResizeDIBSection(int Width, int Height)    // DIB: Devise Independent Bitmap
{
    if (BitmapMemory)
    {
        VirtualFree(BitmapMemory, 0, MEM_RELEASE);
    }

    BitmapWidth = Width;
    BitmapHeight = Height;

    BitmapInfo.bmiHeader.biSize = sizeof(BitmapInfo.bmiHeader);
    BitmapInfo.bmiHeader.biWidth = BitmapWidth;
    BitmapInfo.bmiHeader.biHeight = BitmapHeight;
    BitmapInfo.bmiHeader.biPlanes = 1;
    BitmapInfo.bmiHeader.biBitCount = 32;
    BitmapInfo.bmiHeader.biCompression = BI_RGB;

    // Allocate memory ourselves. As long as it's the right size, windows will handle it.
    // 4 bytes with an area of w*h
    int BitmapMemorySize = (BitmapWidth*BitmapHeight) * BytesPerPixel ;
    BitmapMemory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);
}

internal void Win32UpdateWindow(HDC DeviceContext, RECT* ClientRect, int X, int Y, int Width, int Height)
{
    int WindowWidth = ClientRect->right - ClientRect->left;
    int WindowHeight = ClientRect->bottom - ClientRect->top;
    StretchDIBits(
        DeviceContext,
        /*
        X, Y, Width, Height, // destination
        X, Y, Width, Height, // source
        */
        0, 0, BitmapWidth, BitmapHeight,
        0, 0, WindowWidth, WindowHeight,
        BitmapMemory,
        &BitmapInfo,
        DIB_RGB_COLORS, SRCCOPY);
}

LRESULT CALLBACK Win32MainWindowCallback(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam)
{
    LRESULT Result = 0;

    switch (Message)
    {
        case WM_SIZE:
        {
            RECT ClientRect;
            GetClientRect(Window, &ClientRect);
            int Width = ClientRect.right - ClientRect.left;
            int Height = ClientRect.bottom - ClientRect.top;
            Win32ResizeDIBSection(Width, Height);
        } break;

        case WM_ACTIVATEAPP:
        {
        } break;

        case WM_PAINT:
        {
            PAINTSTRUCT Paint;
            HDC DeviceContext = BeginPaint(Window, &Paint);

            int X = Paint.rcPaint.left;
            int Y = Paint.rcPaint.top;
            int Width = Paint.rcPaint.right - Paint.rcPaint.left;
            int Height = Paint.rcPaint.bottom - Paint.rcPaint.top;

            RECT ClientRect;
            GetClientRect(Window, &ClientRect);
            
            Win32UpdateWindow(DeviceContext, &ClientRect, X, Y, Width, Height);
            
            EndPaint(Window, &Paint);
        } break;

        case WM_DESTROY:
        {
            Running = false;
        } break;

        case WM_CLOSE:
        {
            Running = false;
        } break;

        default:
        {
            Result = DefWindowProc(Window, Message, WParam, LParam);
        } break;
    }

    return Result;
}


/****************** Main ************************/

int CALLBACK WinMain(
    HINSTANCE Instance,
    HINSTANCE PrevInstance,
    LPSTR     CommandLine,
    int       ShowCode)
{
    WNDCLASS WindowClass = {};
    WindowClass.style = CS_OWNDC|CS_HREDRAW|CS_VREDRAW;
    WindowClass.lpfnWndProc = Win32MainWindowCallback;
    WindowClass.hInstance = Instance;
    //WindowClass.hIcon = ;
    WindowClass.lpszClassName = "HandmadeFolayfilaWindowClass";

    if (RegisterClass(&WindowClass))
    {
        HWND Window =
            CreateWindowExA(
            0,
            WindowClass.lpszClassName,
            "Handmade Folayfila",
            WS_OVERLAPPEDWINDOW|WS_VISIBLE,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            0,
            0,
            Instance,
            0);

        // Main loop:
        Running = true;
        if (Window)
        {
            int ColorXOffset = 0;
            int8 YSign = 1;
            int ColorYOffset = 0;
            MSG Message;
            while (Running)
            {
                while(PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
                {
                    if (Message.message == WM_QUIT)
                    {
                        Running = false;
                    }

                    TranslateMessage(&Message);
                    DispatchMessage(&Message);
                }

                RenderAwesomeGradient(ColorXOffset, ColorYOffset);
                HDC DeviceContext = GetDC(Window);
                RECT ClientRect;
                GetClientRect(Window, &ClientRect);
                int WindowWidth = ClientRect.right - ClientRect.left;
                int WindowHeight = ClientRect.bottom - ClientRect.top;
                Win32UpdateWindow(DeviceContext, &ClientRect, 0, 0, WindowWidth, WindowHeight);
                ReleaseDC(Window, DeviceContext);

                // Simple zig-zag in y, kinda like sin.
                if (ColorYOffset == 127)
                {
                    YSign = -1;
                }
                else if (ColorYOffset == -127)
                {
                    YSign = 1;
                }
                ColorYOffset += YSign;
                ColorXOffset += 1;
            }
        }
        else
        {
            // Todo(abdallah): Log
        }
    }
    else
    {
            // Todo(abdallah): Log
    }

    return 0;
}