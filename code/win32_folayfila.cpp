// (C) Copyright 2026 by Abdallah Maaliki / folayfila.

#include "win32_folayfila.h"
#include <xinput.h>
#include <dsound.h>
#include <stdio.h>

/******************* Global Variables *****************************/
global_variable win32_offscreen_buffer GlobalBackBuffer;
global_variable LPDIRECTSOUNDBUFFER GlobalSecondaryBuffer;
global_variable int64 GlobalPerfCountFrequency;
/******************************************************************/

/******************* XINPUT Defines *****************************/
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub)
{
    return ERROR_DEVICE_NOT_CONNECTED;
}
global_variable x_input_get_state *XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub)
{
    return ERROR_DEVICE_NOT_CONNECTED;
}
global_variable x_input_set_state *XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_
/******************************************************************/

/************************** I/O ************************************/
internal debug_read_file_result DEBUGPlatformReadEntireFile(char* FileName)
{
    debug_read_file_result Result = {};

    HANDLE FileHandle = CreateFileA(FileName, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if (FileHandle != INVALID_HANDLE_VALUE)
    {
        LARGE_INTEGER FileSize;
        if (GetFileSizeEx(FileHandle, &FileSize))
        {
            uint32 FileSize32 = SafeTruncateUInt64((uint64)FileSize.QuadPart);
            Result.Contents = VirtualAlloc(0, FileSize32, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
            if (Result.Contents)
            {
                DWORD BytesRead;
                if (ReadFile(FileHandle, Result.Contents, FileSize32, &BytesRead, 0) &&
                    (FileSize32 == BytesRead))
                {
                    Result.ContentSize = FileSize32;
                }
                else
                {
                    DEBUGPlatformFreeFileMemory(Result.Contents);
                    Result.Contents = 0;
                }
            }
        }
        CloseHandle(FileHandle);
    }
    return Result;
}

internal void DEBUGPlatformFreeFileMemory(void* Memeory)
{
    if (Memeory)
    {
        VirtualFree(Memeory, 0, MEM_RELEASE);
    }
}

internal bool32 DEBUGPlatformWriteEntireFile(char* FileName, uint32 MemorySize, void* Memory)
{
    bool32 Result = false;

    HANDLE FileHandle = CreateFileA(FileName, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
    if (FileHandle != INVALID_HANDLE_VALUE)
    {
        DWORD BytesWritten;
        if (WriteFile(FileHandle, Memory, MemorySize, &BytesWritten, 0))
        {
            Result = (BytesWritten == MemorySize);
        }
        CloseHandle(FileHandle);
    }
    return Result;
}
/******************************************************************/

/******************* DirectSound Defines *****************************/
#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND* ppDS, LPUNKNOWN pUnkOuter);
typedef DIRECT_SOUND_CREATE(direct_sound_create);
/******************************************************************/

/******************* Display Window *****************************/
internal win32_window_deminsion Win32GetWindowDeminsion(HWND Window)
{
    win32_window_deminsion Result;

    RECT ClientRect;
    GetClientRect(Window, &ClientRect);
    Result.Width = ClientRect.right - ClientRect.left;
    Result.Height = ClientRect.bottom - ClientRect.top;

    return Result;
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

internal void Win32DisplayBufferInWindow(win32_offscreen_buffer* Buffer,
    HDC DeviceContext, int WindowWidth, int WindowHeight)
{
    StretchDIBits(
        DeviceContext,
        0, 0, WindowWidth, WindowHeight,
        0, 0, Buffer->Width, Buffer->Height,
        Buffer->Memory,
        &Buffer->Info,
        DIB_RGB_COLORS, SRCCOPY);
}
/******************************************************************/

/**************************** Sound *******************************/

internal void Win32InitDSound(HWND Window, int32 SamplesPerSeconds, int32 BufferSize)
{
    // Load the DSound library.
    HMODULE DSoundLibrary = LoadLibraryA("dsound.dll");
    if (!DSoundLibrary)
    {
        return;
    }

    // Get a DirectSound object.
    direct_sound_create* DirectSoundCreate = (direct_sound_create*)
        GetProcAddress(DSoundLibrary, "DirectSoundCreate");

    LPDIRECTSOUND DirectSound;
    if (DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &DirectSound, 0)))
    {
        WAVEFORMATEX WaveFormat = {};
        WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
        WaveFormat.nChannels = 2;
        WaveFormat.nSamplesPerSec = SamplesPerSeconds;
        WaveFormat.wBitsPerSample = 16;
        WaveFormat.nBlockAlign = (WaveFormat.nChannels * WaveFormat.wBitsPerSample) / 8;
        WaveFormat.nAvgBytesPerSec = WaveFormat.nSamplesPerSec * WaveFormat.nBlockAlign;
        WaveFormat.cbSize = 0;

        if (SUCCEEDED(DirectSound->SetCooperativeLevel(Window, DSSCL_PRIORITY)))
        {
            // Create the primary buffer.
            DSBUFFERDESC BufferDescription = {};
            BufferDescription.dwSize = sizeof(BufferDescription);
            BufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;
            LPDIRECTSOUNDBUFFER PrimaryBuffer;
            if (SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDescription, &PrimaryBuffer, 0)))
            {
                if (SUCCEEDED(PrimaryBuffer->SetFormat(&WaveFormat)))
                {
                    OutputDebugStringA("PRIMARYYYY\n");
                }
            }
        }

        // Create a secondary buffer.
        DSBUFFERDESC BufferDescription = {};
        BufferDescription.dwSize = sizeof(BufferDescription);
        BufferDescription.dwFlags = 0;
        BufferDescription.dwBufferBytes = BufferSize;
        BufferDescription.lpwfxFormat = &WaveFormat;
        if (SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDescription, &GlobalSecondaryBuffer, 0)))
        {
            OutputDebugStringA("SECONDARRYYY\n");
        }
    }
}


internal void Win32ClearSoundBuffer(win32_sound_output* SoundOutput)
{
    VOID* Region1;
    DWORD Region1Size;
    VOID* Region2;
    DWORD Region2Size;
    if (SUCCEEDED(GlobalSecondaryBuffer->Lock(0, SoundOutput->SecondaryBufferSize,
        &Region1, &Region1Size,
        &Region2, &Region2Size,
        0)))
    {
        uint8* DestSample = (uint8*)Region1;
        for (DWORD ByteIndex = 0; ByteIndex < Region1Size; ++ByteIndex)
        {
            *DestSample++ = 0;
        }

        DestSample = (uint8*)Region2;
        for (DWORD ByteIndex = 0; ByteIndex < Region2Size; ++ByteIndex)
        {
            *DestSample++ = 0;
        }

        GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
    }
}

internal void Win32FillSoundBuffer(win32_sound_output* SoundOutput, DWORD ByteToLock, DWORD BytesToWrite,
    game_output_sound_buffer* SourceBuffer)
{
    VOID* Region1;
    DWORD Region1Size;
    VOID* Region2;
    DWORD Region2Size;
    if (SUCCEEDED(GlobalSecondaryBuffer->Lock(ByteToLock, BytesToWrite,
        &Region1, &Region1Size,
        &Region2, &Region2Size,
        0)))
    {
        DWORD Region1SampleCount = Region1Size / SoundOutput->BytesPerSample;
        int16* SourceSample = SourceBuffer->Samples;
        int16* DestSample = (int16*)Region1;
        for (DWORD SampleIndex = 0; SampleIndex < Region1SampleCount; ++SampleIndex)
        {
            *DestSample++ = *SourceSample++;
            *DestSample++ = *SourceSample++;
            ++SoundOutput->RunningSampleIndex;
        }

        DWORD Region2SampleCount = Region2Size / SoundOutput->BytesPerSample;
        DestSample = (int16*)Region2;
        for (DWORD SampleIndex = 0; SampleIndex < Region2SampleCount; ++SampleIndex)
        {
            *DestSample++ = *SourceSample++;
            *DestSample++ = *SourceSample++;
            ++SoundOutput->RunningSampleIndex;
        }
        GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
    }
}
/******************************************************************/

/**************************** Input *******************************/
internal void Win32LoadXInput()
{
    HMODULE XInputLibrary = LoadLibraryA("xinput1_4.dll");
    if (!XInputLibrary)
    {
        XInputLibrary = LoadLibraryA("xinput1_3.dll");
    }
    if (XInputLibrary)
    {
        XInputGetState = (x_input_get_state*)GetProcAddress(XInputLibrary, "XInputGetState");
        if (!XInputGetState) { XInputGetState = XInputGetStateStub; }

        XInputSetState = (x_input_set_state*)GetProcAddress(XInputLibrary, "XInputSetState");
        if (!XInputSetState) { XInputSetState = XInputSetStateStub; }
    }
}

internal LRESULT CALLBACK Win32MainWindowCallback(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam)
{
    LRESULT Result = 0;

    switch (Message)
    {
        case WM_ACTIVATEAPP:
        {
        } break;

        case WM_DESTROY:
        {
            GlobalRunning = false;
        } break;

        case WM_CLOSE:
        {
            GlobalRunning = false;
        } break;

        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP:
        {
            Assert(!"Keyboard input came in through a non-dispatch message.");
        } break;

        case WM_PAINT:
        {
            PAINTSTRUCT Paint;
            HDC DeviceContext = BeginPaint(Window, &Paint);
            win32_window_deminsion Deminsion = Win32GetWindowDeminsion(Window);;
            Win32DisplayBufferInWindow(&GlobalBackBuffer, DeviceContext, Deminsion.Width, Deminsion.Height);
            EndPaint(Window, &Paint);
        } break;

        default:
        {
            Result = DefWindowProcA(Window, Message, WParam, LParam);
        } break;
    }

    return Result;
}

internal void Win32ProcessKeyboardMessage(game_button_state* NewState, bool32 IsDown)
{
    NewState->EndedDown = IsDown;
    ++NewState->HalfTransitionCount;
}

internal void Win32HandleKeyboardInput(WPARAM WParam, LPARAM LParam, game_controller_input* KeyboardController)
{
    uint32 VKCode = (uint32)WParam;
    bool WasDown = ((LParam & (1 << 30)) != 0);
    bool IsDown = ((LParam & (1 << 31)) == 0);
    if (WasDown != IsDown)
    {
        if (VKCode == 'W')
        {
            Win32ProcessKeyboardMessage(&KeyboardController->MoveUp, IsDown);
        }
        else if (VKCode == 'S')
        {
            Win32ProcessKeyboardMessage(&KeyboardController->MoveDown, IsDown);
        }
        else if (VKCode == 'A')
        {
            Win32ProcessKeyboardMessage(&KeyboardController->MoveLeft, IsDown);
        }
        else if (VKCode == 'D')
        {
            Win32ProcessKeyboardMessage(&KeyboardController->MoveRight, IsDown);
        }
        else if (VKCode == 'Q')
        {
            Win32ProcessKeyboardMessage(&KeyboardController->LeftShoulder, IsDown);
        }
        else if (VKCode == 'E')
        {
            Win32ProcessKeyboardMessage(&KeyboardController->RightShoulder, IsDown);
        }
        else if (VKCode == VK_UP)
        {
            Win32ProcessKeyboardMessage(&KeyboardController->MoveUp, IsDown);
        }
        else if (VKCode == VK_DOWN)
        {
            Win32ProcessKeyboardMessage(&KeyboardController->MoveDown, IsDown);
        }
        else if (VKCode == VK_LEFT)
        {
            Win32ProcessKeyboardMessage(&KeyboardController->MoveLeft, IsDown);
        }
        else if (VKCode == VK_RIGHT)
        {
            Win32ProcessKeyboardMessage(&KeyboardController->MoveRight, IsDown);
        }
        else if (VKCode == VK_SPACE)
        {
        }
        else if (VKCode == VK_ESCAPE)
        {
            Win32ProcessKeyboardMessage(&KeyboardController->Back, IsDown);
        }
    }

    bool32 AltIsDown = (LParam & (1 << 29));
    if (VKCode == VK_F4 && AltIsDown)
    {
        GlobalRunning = false;
    }
}

internal void Win32HandleWindowsMessageLoop(game_controller_input* KeyboardController)
{
    MSG Message;
    while (PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
    {
        switch (Message.message)
        {
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP:
        {
            Win32HandleKeyboardInput(Message.wParam, Message.lParam, KeyboardController);
        } break;

        default:
        {
            TranslateMessage(&Message);
            DispatchMessageA(&Message);
        } break;
        }
        if (Message.message == WM_QUIT)
        {
            GlobalRunning = false;
        }
    }
}

internal void Win32ProcessXinputDigitalButton(DWORD XInputButtonState,
                                              game_button_state* OldState, DWORD ButtonBit,
                                              game_button_state* NewState)
{
    NewState->EndedDown = ((XInputButtonState & ButtonBit) == ButtonBit);
    NewState->HalfTransitionCount = (OldState->EndedDown != NewState->EndedDown) ? 1 : 0;
}

internal float Win32ProcessXInputStickValue(SHORT Value, SHORT DeadZoneThreshold)
{
    float Result = 0;
    if (Value > DeadZoneThreshold)
    {
        Result = (float)Value / 32768.0f;
    }
    else if (Value < -DeadZoneThreshold)
    {
        Result = (float)Value / 32767.0f;
    }
    return Result;
}

internal void Win32HandleControllerInput(game_input* OldInput, game_input* NewInput)
{
    DWORD MaxControllerCount = XUSER_MAX_COUNT;
    if (MaxControllerCount > (ArrayCount(NewInput->Controllers) - 1))
    {
        MaxControllerCount = (ArrayCount(NewInput->Controllers) - 1);
    }

    for (DWORD ControllerIndex = 0; ControllerIndex < MaxControllerCount; ++ControllerIndex)
    {
        // Index 0 is for the keyboard, so we icrement 1.
        DWORD OurControllerIndex = ControllerIndex + 1;
        game_controller_input* OldController = GetController(OldInput, OurControllerIndex);
        game_controller_input* NewController = GetController(NewInput, OurControllerIndex);

        // XINPUT always expects index 0 for the first controller.
        XINPUT_STATE ControllerState;
        if (XInputGetState(ControllerIndex, &ControllerState) == ERROR_SUCCESS)
        {
            NewController->IsConnected = true;

            // This controller is pluged in.
            XINPUT_GAMEPAD* Pad = &ControllerState.Gamepad;

            NewController->IsAnalog = true;
            NewController->StickAverage.X = Win32ProcessXInputStickValue(
                Pad->sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
            NewController->StickAverage.Y = Win32ProcessXInputStickValue(
                Pad->sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);

            // Analog
            float Threshold = 0.5f;
            Win32ProcessXinputDigitalButton(
                (NewController->StickAverage.Y > -Threshold) ? 1 : 0,
                &OldController->MoveDown, 1,
                &NewController->MoveDown);
            Win32ProcessXinputDigitalButton(
                (NewController->StickAverage.Y > Threshold) ? 1 : 0,
                &OldController->MoveUp, 1,
                &NewController->MoveUp);
            Win32ProcessXinputDigitalButton(
                (NewController->StickAverage.X < -Threshold) ? 1 : 0,
                &OldController->MoveLeft, 1,
                &NewController->MoveLeft);
            Win32ProcessXinputDigitalButton(
                (NewController->StickAverage.X > Threshold) ? 1 : 0,
                &OldController->MoveRight, 1,
                &NewController->MoveRight);

            // Gamepad Buttons
            Win32ProcessXinputDigitalButton(Pad->wButtons,
                &OldController->ActionDown, XINPUT_GAMEPAD_A,
                &NewController->ActionDown);
            Win32ProcessXinputDigitalButton(Pad->wButtons,
                &OldController->ActionRight, XINPUT_GAMEPAD_B,
                &NewController->ActionRight);
            Win32ProcessXinputDigitalButton(Pad->wButtons,
                &OldController->ActionLeft, XINPUT_GAMEPAD_X,
                &NewController->ActionLeft);
            Win32ProcessXinputDigitalButton(Pad->wButtons,
                &OldController->ActionUp, XINPUT_GAMEPAD_Y,
                &NewController->ActionUp);

            // Dpad Buttons
            Win32ProcessXinputDigitalButton(Pad->wButtons,
                &OldController->MoveDown, XINPUT_GAMEPAD_DPAD_DOWN,
                &NewController->MoveDown);
            Win32ProcessXinputDigitalButton(Pad->wButtons,
                &OldController->MoveUp, XINPUT_GAMEPAD_DPAD_UP,
                &NewController->MoveUp);
            Win32ProcessXinputDigitalButton(Pad->wButtons,
                &OldController->MoveLeft, XINPUT_GAMEPAD_DPAD_LEFT,
                &NewController->MoveLeft);
            Win32ProcessXinputDigitalButton(Pad->wButtons,
                &OldController->MoveRight, XINPUT_GAMEPAD_DPAD_RIGHT,
                &NewController->MoveRight);

            // Shoulder Buttons
            Win32ProcessXinputDigitalButton(Pad->wButtons,
                &OldController->RightShoulder, XINPUT_GAMEPAD_RIGHT_SHOULDER,
                &NewController->RightShoulder);
            Win32ProcessXinputDigitalButton(Pad->wButtons,
                &OldController->LeftShoulder, XINPUT_GAMEPAD_LEFT_SHOULDER,
                &NewController->LeftShoulder);

            // Start & Back
            Win32ProcessXinputDigitalButton(Pad->wButtons,
                &OldController->Start, XINPUT_GAMEPAD_START,
                &NewController->Start);
            Win32ProcessXinputDigitalButton(Pad->wButtons,
                &OldController->Back, XINPUT_GAMEPAD_BACK,
                &NewController->Back);
        }
        else
        {
            NewController->IsConnected = false;
        }
    }
}
/******************************************************************/

inline LARGE_INTEGER Win32GetWallClock()
{
    LARGE_INTEGER Result;
    QueryPerformanceCounter(&Result);
    return Result;
}

inline float Win32GetSecondsElapsed(LARGE_INTEGER Start, LARGE_INTEGER End)
{
    float Result = ((float)(End.QuadPart - Start.QuadPart) / (float)GlobalPerfCountFrequency);
    return Result;
}

/**************************** WinMain *******************************/
int CALLBACK WinMain( HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CommandLine, int ShowCode)
{
    LARGE_INTEGER PerfCountFrequencyResult;
    QueryPerformanceFrequency(&PerfCountFrequencyResult);  // Fixed at system boot and consistent across all processors.
    GlobalPerfCountFrequency = PerfCountFrequencyResult.QuadPart;

    // Set the Windows scheduler granularity to 1ms
    // so the our Sleep can be more granular.
    UINT DesiredSchedulerMS = 1;
    bool32 SleepIsGranular = (timeBeginPeriod(DesiredSchedulerMS) == TIMERR_NOERROR);

    Win32ResizeDIBSection(&GlobalBackBuffer, 1280, 720);

    WNDCLASSA WindowClass = {};
    WindowClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    WindowClass.lpfnWndProc = Win32MainWindowCallback;
    WindowClass.hInstance = Instance;
    //WindowClass.hIcon = ;
    WindowClass.lpszClassName = "HandmadeFolayfilaWindowClass";

    // TODO: How do we query this on windows?
    int MonitorRefreshHz = 120;
    int GameUpdateHz = MonitorRefreshHz/2;
    float TargetSecondsPerFrame = 1.0f / (float)GameUpdateHz;

    if (!RegisterClass(&WindowClass))
    {
        return 0;
    }

    HWND Window = CreateWindowExA(
            0, WindowClass.lpszClassName, "Handmade Folayfila",
            WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
            0, 0, Instance, 0);

    if (!Window)
    {
        return 0;
    }

    // Since we specified CS_OWNDC, we can just get one device
    // context and use it forever as we're not sharing it with anyone.
    HDC DeviceContext = GetDC(Window);

    win32_sound_output SoundOutput = {};
    SoundOutput.SamplesPerSecond = 48000;
    SoundOutput.RunningSampleIndex = 0;
    SoundOutput.BytesPerSample = sizeof(int16) * 2;
    SoundOutput.SecondaryBufferSize = SoundOutput.SamplesPerSecond * SoundOutput.BytesPerSample;
    SoundOutput.LatencySampleCount = SoundOutput.SamplesPerSecond / 15;
    Win32InitDSound(Window, SoundOutput.SamplesPerSecond, SoundOutput.SecondaryBufferSize);
    Win32ClearSoundBuffer(&SoundOutput);
    GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

    int16* Samples = (int16*)VirtualAlloc(0, SoundOutput.SecondaryBufferSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);;

#if FOLAYFILA_INTERNAL
    LPVOID BaseAddress = (LPVOID)Gigabytes((uint64)4);
#else
    LPVOID BaseAddress = 0;
#endif  // FOLAYFILA_INTERNAL

    game_memory GameMemory = {};
    GameMemory.PermanentStorageSize = Megabytes(64);
    GameMemory.TransientStorageSize = Gigabytes((uint64)1);
    uint64 TotalSize = GameMemory.PermanentStorageSize + GameMemory.TransientStorageSize;
    GameMemory.PermanentStorage = VirtualAlloc(BaseAddress, (size_t)TotalSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    GameMemory.TransientStorage = ((uint8*)GameMemory.PermanentStorage + GameMemory.PermanentStorageSize);

    if (!Samples || !GameMemory.PermanentStorage || !GameMemory.TransientStorage)
    {
        // Can't run the game without memory.
        return 0;
    }

    Win32LoadXInput();
    game_input Input[2] = {};
    game_input* NewInput = &Input[0];
    game_input* OldInput = &Input[1];

    LARGE_INTEGER LastCounter = Win32GetWallClock();

    uint64 LastCycleCount = __rdtsc();

    // Main Loop
    GlobalRunning = true;
    while (GlobalRunning)
    {
        game_controller_input* OldKeyboardController = GetController(OldInput, 0);
        game_controller_input* NewKeyboardController = GetController(NewInput, 0);
        *NewKeyboardController = {};
        NewKeyboardController->IsConnected = true;
        for (int ButtonIndex = 0; ButtonIndex < ArrayCount(NewKeyboardController->Buttons); ++ButtonIndex)
        {
            NewKeyboardController->Buttons[ButtonIndex].EndedDown =
                OldKeyboardController->Buttons[ButtonIndex].EndedDown;
        }

        Win32HandleWindowsMessageLoop(NewKeyboardController);
        Win32HandleControllerInput(OldInput, NewInput);

        DWORD ByteToLock = 0;
        DWORD TargetCursor = 0;
        DWORD BytesToWrite = 0;
        DWORD PlayCursor = 0;
        DWORD WriteCursor = 0;
        bool32 SoundIsValid = false;
        if (SUCCEEDED(GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor)))
        {
            // NOTE(abdallah): DirectSound output test
            ByteToLock = (SoundOutput.RunningSampleIndex * SoundOutput.BytesPerSample) %
                SoundOutput.SecondaryBufferSize;

            TargetCursor = ((PlayCursor +
                (SoundOutput.LatencySampleCount * SoundOutput.BytesPerSample)) %
                SoundOutput.SecondaryBufferSize);

            if (ByteToLock > TargetCursor)
            {
                BytesToWrite = (SoundOutput.SecondaryBufferSize - ByteToLock);
                BytesToWrite += TargetCursor;
            }
            else
            {
                BytesToWrite = TargetCursor - ByteToLock;
            }

            SoundIsValid = true;
        }

        game_output_sound_buffer SoundBuffer = {};
        SoundBuffer.SamplesPerSecond = SoundOutput.SamplesPerSecond;
        SoundBuffer.SampleCount = BytesToWrite / SoundOutput.BytesPerSample;
        SoundBuffer.Samples = Samples;

        game_graphics_buffer GraphicsBuffer = {};
        GraphicsBuffer.Memory = GlobalBackBuffer.Memory;
        GraphicsBuffer.Width = GlobalBackBuffer.Width;
        GraphicsBuffer.Height = GlobalBackBuffer.Height;
        GraphicsBuffer.Pitch = GlobalBackBuffer.Pitch;
        GameUpdateAndRender(&GameMemory, NewInput, &GraphicsBuffer, &SoundBuffer);

        if (SoundIsValid)
        {
            Win32FillSoundBuffer(&SoundOutput, ByteToLock, BytesToWrite, &SoundBuffer);
        }

        LARGE_INTEGER WorkCounter = Win32GetWallClock();
        float WorkSecondsElapsed = Win32GetSecondsElapsed(LastCounter, WorkCounter);

        float SecondsElapsedForFrame = WorkSecondsElapsed;
        while (SecondsElapsedForFrame < TargetSecondsPerFrame)
        {
            if (SleepIsGranular)
            {
                DWORD SleepMS = (DWORD)(1000.0f * (TargetSecondsPerFrame - SecondsElapsedForFrame));
                Sleep(SleepMS);
                SecondsElapsedForFrame = Win32GetSecondsElapsed(LastCounter, Win32GetWallClock());
            }
        }

        win32_window_deminsion Deminsion = Win32GetWindowDeminsion(Window);
        Win32DisplayBufferInWindow(&GlobalBackBuffer, DeviceContext, Deminsion.Width, Deminsion.Height);

        // Refresh input for the next cycle.
        game_input* Temp = NewInput;
        NewInput = OldInput;
        OldInput = Temp;

        LARGE_INTEGER EndCounter = Win32GetWallClock();
        float MSPerFrame = 1000.0f * Win32GetSecondsElapsed(LastCounter, EndCounter);
        LastCounter = EndCounter;

        uint64 EndCycleCount = __rdtsc();
        uint64 CyclesElapsed = EndCycleCount - LastCycleCount;
        LastCycleCount = EndCycleCount;


        float FPS = 1000.0f / MSPerFrame;
        float MegaCyclesPerFrame = ((float)CyclesElapsed / (1000.0f * 1000.0f));

        char LogBuffer[256];
        sprintf_s(LogBuffer, "Milliseconds/frame: %.02f ms / %.02f FPS / %.02f MCPF\n", MSPerFrame, FPS, MegaCyclesPerFrame);
        OutputDebugStringA(LogBuffer);
    }
    return 0;
}