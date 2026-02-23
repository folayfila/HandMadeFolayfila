// (C) Copyright 2026 by Abdallah Maaliki / folayfila.

#include <windows.h>
#include <stdint.h>

#define internal static         // Don't allow this function to be called from another file than its.
#define local_presist static    // static local variable
#define global_variable static

struct win32_offscreen_buffer
{
    BITMAPINFO Info;
    void* Memory;
    int Width;
    int Height;
    int Pitch;
    int BytesPerPixel;
};

// TODO(abdallah): Global for now.
global_variable bool GlobalRunning;
global_variable win32_offscreen_buffer GlobalBackBuffer;

struct win32_window_deminsion
{
    int Width;
    int Height;
};

win32_window_deminsion Win32GetWindowDeminsion(HWND Window)
{
    win32_window_deminsion Result;

    RECT ClientRect;
    GetClientRect(Window, &ClientRect);
    Result.Width = ClientRect.right - ClientRect.left;
    Result.Height = ClientRect.bottom - ClientRect.top;

    return Result;
}

internal void RenderAwesomeGradient(win32_offscreen_buffer Buffer, int XOffset, int YOffset)
{
    uint8_t* Row = (uint8_t*)Buffer.Memory;
    for (int Y = 0; Y < Buffer.Height; ++Y)
    {
        uint32_t* Pixel = (uint32_t*)Row;
        for (int X = 0; X < Buffer.Width; ++X)
        {

            uint8_t Blue = (uint8_t)(X + XOffset);
            uint8_t Green = (uint8_t)(Y + YOffset);
            uint8_t Red = 100;

            // Memory: BB GG RR xx
            *Pixel++ = (Blue | Green << 8 | Red << 16);
        }
        Row += Buffer.Pitch;
    }
}

// We create a buffer for our content that we want to display, then we pass it to windows to paint it when we want.
internal void Win32ResizeDIBSection(win32_offscreen_buffer* Buffer, int Width, int Height)    // DIB: Devise Independent Bitmap
{
    if (Buffer->Memory)
    {
        VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
    }

    Buffer->Width = Width;
    Buffer->Height = Height;
    Buffer->BytesPerPixel = 4;

    Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
    Buffer->Info.bmiHeader.biWidth = Buffer->Width;
    Buffer->Info.bmiHeader.biHeight = Buffer->Height;
    Buffer->Info.bmiHeader.biPlanes = 1;
    Buffer->Info.bmiHeader.biBitCount = 32;
    Buffer->Info.bmiHeader.biCompression = BI_RGB;

    // Allocate memory ourselves. As long as it's the right size, windows will handle it.
    int BitmapMemorySize = (Buffer->Width * Buffer->Height) * Buffer->BytesPerPixel;
    Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);

    Buffer->Pitch = Width * Buffer->BytesPerPixel;
}

internal void Win32DisplayBufferInWindow(HDC DeviceContext, int WindowWidth, int WindowHeight,
                                      win32_offscreen_buffer Buffer,
                                      int X, int Y, int Width, int Height)
{
    StretchDIBits(
        DeviceContext,
        0, 0, WindowWidth, WindowHeight,
        0, 0, Buffer.Width, Buffer.Height,
        Buffer.Memory,
        &Buffer.Info,
        DIB_RGB_COLORS, SRCCOPY);
}

LRESULT CALLBACK Win32MainWindowCallback(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam)
{
    LRESULT Result = 0;

    switch (Message)
    {
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

            win32_window_deminsion Deminsion = Win32GetWindowDeminsion(Window);;
            Win32DisplayBufferInWindow(DeviceContext, Deminsion.Width, Deminsion.Height,
                                       GlobalBackBuffer, X, Y, Width, Height);
            EndPaint(Window, &Paint);
        } break;

        case WM_DESTROY:
        {
            GlobalRunning = false;
        } break;

        case WM_CLOSE:
        {
            GlobalRunning = false;
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

    Win32ResizeDIBSection(&GlobalBackBuffer, 1280, 720);

    WindowClass.style = CS_HREDRAW|CS_VREDRAW;
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
        if (Window)
        {
            MSG Message;
            int ColorXOffset = 0;
            int ColorYOffset = 0;
            int8_t YSign = 1;
            
            GlobalRunning = true;
            while (GlobalRunning)
            {
                while(PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
                {
                    if (Message.message == WM_QUIT)
                    {
                        GlobalRunning = false;
                    }

                    TranslateMessage(&Message);
                    DispatchMessage(&Message);
                }

                RenderAwesomeGradient(GlobalBackBuffer, ColorXOffset, ColorYOffset);
                HDC DeviceContext = GetDC(Window);
                win32_window_deminsion Deminsion = Win32GetWindowDeminsion(Window);;
                Win32DisplayBufferInWindow(DeviceContext, Deminsion.Width, Deminsion.Height,
                                           GlobalBackBuffer, 0, 0, Deminsion.Width, Deminsion.Height);
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