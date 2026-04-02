// (C) Copyright 2026 by Abdallah Maaliki / folayfila.

#include "win32_folayfila.h"
#include "folayfila_intrinsics.h"
#include <xinput.h>
#include <dsound.h>
#include <stdio.h>

/******************* Global Variables *****************************/
global_variable win32_offscreen_buffer GlobalBackBuffer;
global_variable LPDIRECTSOUNDBUFFER GlobalSecondaryBuffer;
global_variable int64 GlobalPerfCountFrequency;
global_variable bool32 GlobalRunning;
global_variable bool32 GlobalShowCursor;
global_variable WINDOWPLACEMENT GlobalWindowPosition = { sizeof(GlobalWindowPosition) };
/******************************************************************/

/******************* Game Code *****************************/
internal FILETIME Win32GetLastWriteTime(char* FileName)
{
    FILETIME lastWriteTime = {};

    WIN32_FILE_ATTRIBUTE_DATA data;
    if (GetFileAttributesExA(FileName, GetFileExInfoStandard, &data))
    {
        lastWriteTime = data.ftLastWriteTime;
    }
    return lastWriteTime;
}

internal win32_game_code Win32LoadGameCode(char* SourceDLLName, char* TempDLLName)
{
    win32_game_code result = {};
    
    result.DLLLastWriteTime = Win32GetLastWriteTime(SourceDLLName);

    CopyFile(SourceDLLName, TempDLLName, FALSE);
    result.GameCodeDLL = LoadLibraryA(TempDLLName);
    if (result.GameCodeDLL)
    {
        result.UpdateAndRender = (game_update_and_render*)GetProcAddress(result.GameCodeDLL, "GameUpdateAndRender");
        result.IsValid = (result.UpdateAndRender != nullptr);
    }

    if(!result.IsValid)
    {
        result.UpdateAndRender = 0;
    }

    return result;
}

internal void Win32UnloadGameCode(win32_game_code* GameCode)
{
    if(GameCode->GameCodeDLL)
    {
        FreeLibrary(GameCode->GameCodeDLL);
    }
    GameCode->GameCodeDLL = 0;
    GameCode->IsValid = false;
    GameCode->UpdateAndRender = 0;
}
/******************************************************************/

/************************** I/O ************************************/
DEBUG_PLATFORM_FREE_FILE_MEMORY(DEBUGPlatformFreeFileMemory)
{
    if (Memeory)
    {
        VirtualFree(Memeory, 0, MEM_RELEASE);
    }
}

DEBUG_PLATFORM_READ_ENTIRE_FILE(DEBUGPlatformReadEntireFile)
{
    debug_read_file_result result = {};

    HANDLE fileHandle = CreateFileA(FileName, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if (fileHandle != INVALID_HANDLE_VALUE)
    {
        LARGE_INTEGER fileSize;
        if (GetFileSizeEx(fileHandle, &fileSize))
        {
            uint32 fileSize32 = SafeTruncateUInt64((uint64)fileSize.QuadPart);
            result.Contents = VirtualAlloc(0, fileSize32, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
            if (result.Contents)
            {
                DWORD bytesRead;
                if (ReadFile(fileHandle, result.Contents, fileSize32, &bytesRead, 0) &&
                    (fileSize32 == bytesRead))
                {
                    result.ContentSize = fileSize32;
                }
                else
                {
                    DEBUGPlatformFreeFileMemory(Thread, result.Contents);
                    result.Contents = 0;
                }
            }
        }
        CloseHandle(fileHandle);
    }
    return result;
}

DEBUG_PLATFORM_WRITE_ENTIRE_FILE(DEBUGPlatformWriteEntireFile)
{
    bool32 result = false;

    HANDLE fileHandle = CreateFileA(FileName, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
    if (fileHandle != INVALID_HANDLE_VALUE)
    {
        DWORD bytesWritten;
        if (WriteFile(fileHandle, Memory, MemorySize, &bytesWritten, 0))
        {
            result = (bytesWritten == MemorySize);
        }
        CloseHandle(fileHandle);
    }
    return result;
}

internal void Win32GetEXEFileName(win32_state* State)
{
    DWORD sizeOfFilename = GetModuleFileNameA(0, State->EXEFileName, sizeof(State->EXEFileName));
    State->OnePastLastEXEFileNameSlash = State->EXEFileName;
    for (char* scan = State->EXEFileName; *scan; ++scan)
    {
        if (*scan == '\\')
        {
            State->OnePastLastEXEFileNameSlash = scan + 1;
        }
    }
}

internal void Win32BuildEXEPathFileName(win32_state* State, char* FileName, int DestCount, char* Dest)
{
    CatStrings(State->OnePastLastEXEFileNameSlash - State->EXEFileName,
        State->EXEFileName, StringLength(FileName), FileName,
        DestCount, Dest);
}
/******************************************************************/

/******************* Display Window *****************************/
internal win32_window_deminsion Win32GetWindowDeminsion(HWND Window)
{
    win32_window_deminsion result;

    RECT clientRect;
    GetClientRect(Window, &clientRect);
    result.Width = clientRect.right - clientRect.left;
    result.Height = clientRect.bottom - clientRect.top;

    return result;
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
    Buffer->Info.bmiHeader.biHeight = -Buffer->Height;
    Buffer->Info.bmiHeader.biPlanes = 1;
    Buffer->Info.bmiHeader.biBitCount = 32;
    Buffer->Info.bmiHeader.biCompression = BI_RGB;

    // Allocate memory ourselves. As long as it's the right size, windows will handle it.
    int bitmapMemorySize = (Buffer->Width * Buffer->Height) * Buffer->BytesPerPixel;
    Buffer->Memory = VirtualAlloc(0, bitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);

    Buffer->Pitch = Width * Buffer->BytesPerPixel;
}

internal void Win32DisplayBufferInWindow(win32_offscreen_buffer* Buffer,
    HDC DeviceContext, int WindowWidth, int WindowHeight)
{
    int offsetX = 10;
    int offsetY = 10;
    int width = Buffer->Width;
    int height = Buffer->Height;

    if ((WindowWidth >= Buffer->Width * 2) &&
        (WindowHeight >= Buffer->Height * 2))
    {
        offsetX = 0;
        offsetY = 0;
        width = WindowWidth;
        height = WindowHeight;
    }

    StretchDIBits(
        DeviceContext,
        offsetX, offsetY, width, height,
        0, 0, Buffer->Width, Buffer->Height,
        Buffer->Memory,
        &Buffer->Info,
        DIB_RGB_COLORS, SRCCOPY);
}

internal LRESULT CALLBACK Win32MainWindowCallback(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam)
{
    LRESULT result = 0;

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

    case WM_SETCURSOR:
    {
        if (GlobalShowCursor)
        {
            result = DefWindowProcA(Window, Message, WParam, LParam);
        }
        else
        {
            SetCursor(0);
        }
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
        PAINTSTRUCT paint;
        HDC deviceContext = BeginPaint(Window, &paint);
        win32_window_deminsion deminsion = Win32GetWindowDeminsion(Window);;
        Win32DisplayBufferInWindow(&GlobalBackBuffer, deviceContext, deminsion.Width, deminsion.Height);
        EndPaint(Window, &paint);
    } break;

    default:
    {
        result = DefWindowProcA(Window, Message, WParam, LParam);
    } break;
    }

    return result;
}
/******************************************************************/


/******************* DirectSound Defines *****************************/
#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND* ppDS, LPUNKNOWN pUnkOuter);
typedef DIRECT_SOUND_CREATE(direct_sound_create);
/******************************************************************/

/**************************** Sound *******************************/

internal void Win32InitDSound(HWND Window, int32 SamplesPerSeconds, int32 BufferSize)
{
    // Load the DSound library.
    HMODULE dSoundLibrary = LoadLibraryA("dsound.dll");
    if (!dSoundLibrary)
    {
        return;
    }

    // Get a DirectSound object.
    direct_sound_create* directSoundCreate = (direct_sound_create*)
        GetProcAddress(dSoundLibrary, "DirectSoundCreate");

    LPDIRECTSOUND directSound;
    if (directSoundCreate && SUCCEEDED(directSoundCreate(0, &directSound, 0)))
    {
        WAVEFORMATEX waveFormat = {};
        waveFormat.wFormatTag = WAVE_FORMAT_PCM;
        waveFormat.nChannels = 2;
        waveFormat.nSamplesPerSec = SamplesPerSeconds;
        waveFormat.wBitsPerSample = 16;
        waveFormat.nBlockAlign = (waveFormat.nChannels * waveFormat.wBitsPerSample) / 8;
        waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
        waveFormat.cbSize = 0;

        if (SUCCEEDED(directSound->SetCooperativeLevel(Window, DSSCL_PRIORITY)))
        {
            // Create the primary buffer.
            DSBUFFERDESC bufferDescription = {};
            bufferDescription.dwSize = sizeof(bufferDescription);
            bufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;
            LPDIRECTSOUNDBUFFER primaryBuffer;
            if (SUCCEEDED(directSound->CreateSoundBuffer(&bufferDescription, &primaryBuffer, 0)))
            {
                if (SUCCEEDED(primaryBuffer->SetFormat(&waveFormat)))
                {
                    OutputDebugStringA("PRIMARYYYY\n");
                }
            }
        }

        // Create a secondary buffer.
        DSBUFFERDESC bufferDescription = {};
        bufferDescription.dwSize = sizeof(bufferDescription);
        bufferDescription.dwFlags = 0;
        bufferDescription.dwBufferBytes = BufferSize;
        bufferDescription.lpwfxFormat = &waveFormat;
        if (SUCCEEDED(directSound->CreateSoundBuffer(&bufferDescription, &GlobalSecondaryBuffer, 0)))
        {
            OutputDebugStringA("SECONDARRYYY\n");
        }
    }
}

internal void Win32ClearSoundBuffer(win32_sound_output* SoundOutput)
{
    VOID* region1;
    DWORD region1Size;
    VOID* region2;
    DWORD region2Size;
    if (SUCCEEDED(GlobalSecondaryBuffer->Lock(0, SoundOutput->SecondaryBufferSize,
        &region1, &region1Size,
        &region2, &region2Size,
        0)))
    {
        uint8* destSample = (uint8*)region1;
        for (DWORD ByteIndex = 0; ByteIndex < region1Size; ++ByteIndex)
        {
            *destSample++ = 0;
        }

        destSample = (uint8*)region2;
        for (DWORD ByteIndex = 0; ByteIndex < region2Size; ++ByteIndex)
        {
            *destSample++ = 0;
        }

        GlobalSecondaryBuffer->Unlock(region1, region1Size, region2, region2Size);
    }
}

internal void Win32FillSoundBuffer(win32_sound_output* SoundOutput, DWORD ByteToLock, DWORD BytesToWrite,
    game_output_sound_buffer* SourceBuffer)
{
    VOID* region1;
    DWORD region1Size;
    VOID* region2;
    DWORD region2Size;
    if (SUCCEEDED(GlobalSecondaryBuffer->Lock(ByteToLock, BytesToWrite,
        &region1, &region1Size,
        &region2, &region2Size,
        0)))
    {
        DWORD region1SampleCount = region1Size / SoundOutput->BytesPerSample;
        int16* sourceSample = SourceBuffer->Samples;
        int16* destSample = (int16*)region1;
        for (DWORD sampleIndex = 0; sampleIndex < region1SampleCount; ++sampleIndex)
        {
            *destSample++ = *sourceSample++;
            *destSample++ = *sourceSample++;
            ++SoundOutput->RunningSampleIndex;
        }

        DWORD region2SampleCount = region2Size / SoundOutput->BytesPerSample;
        destSample = (int16*)region2;
        for (DWORD sampleIndex = 0; sampleIndex < region2SampleCount; ++sampleIndex)
        {
            *destSample++ = *sourceSample++;
            *destSample++ = *sourceSample++;
            ++SoundOutput->RunningSampleIndex;
        }
        GlobalSecondaryBuffer->Unlock(region1, region1Size, region2, region2Size);
    }
}

internal void Win32DebugDrawVertical(win32_offscreen_buffer* BackBuffer, int X, int Top, int Bottom, uint32 Color)
{
    uint8* pixel = ((uint8*)BackBuffer->Memory + X*BackBuffer->BytesPerPixel + Top*BackBuffer->Pitch);
    for (int y = 0; y < Bottom; ++y)
    {
        *(uint32*)pixel = Color;
        pixel += BackBuffer->Pitch;
    }
}
/******************************************************************/

/********************** Sound Debug Draw **************************/
#if 0
inline void Win32DrawSoundBufferMarker(win32_offscreen_buffer* BackBuffer,
    win32_sound_output* SoundOutput, float C, int PadX, int Top, int Bottom,
    DWORD Value, uint32 Color)
{
    float XFloat = (C * (float)Value);
    int X = PadX + (int)XFloat;
    Win32DebugDrawVertical(BackBuffer, X, Top, Bottom, Color);
}

internal void Win32DebugSyncDisplay(win32_offscreen_buffer *BackBuffer, int MarkersCount,
    win32_debug_time_marker* Markers,
    win32_sound_output *SoundOutput, float TargetSecondsPerFrame)
{
    int PadX = 16;
    int PadY = 16;

    int Top = PadY;
    int Bottom = BackBuffer->Height - PadY;

    float C = (float)(BackBuffer->Width - 2*PadX)/ (float)SoundOutput->SecondaryBufferSize;
    for (int MarkerIndex = 0; MarkerIndex < MarkersCount; ++MarkerIndex)
    {
        win32_debug_time_marker* ThisMarker = &Markers[MarkerIndex];
        Win32DrawSoundBufferMarker(BackBuffer, SoundOutput, C, PadX, Top, Bottom, ThisMarker->PlayCursor, 0xFFFFFFFF);
        Win32DrawSoundBufferMarker(BackBuffer, SoundOutput, C, PadX, Top, Bottom, ThisMarker->WriteCursor, 0xFF00000);
    }
}
#endif // 0
/******************************************************************/

/*********************** Input Recording **************************/

internal void Win32GetInputFileLocation(win32_state* State, int SlotIndex, int DestCount, char* Dest)
{
    Assert(SlotIndex == 1);
    Win32BuildEXEPathFileName(State, "input.recording", DestCount, Dest);
}

internal void Win32BeginRecordingInput(win32_state* State, int InputRecordingIndex)
{
    State->InputRecordingIndex = InputRecordingIndex;

    char fileName[WIN32_STATE_FILE_NAME_COUNT];
    Win32GetInputFileLocation(State, 1, sizeof(fileName), fileName);
    State->RecordingHandle = CreateFileA(fileName, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);

    DWORD bytesToWrite = (DWORD)State->TotalSize;
    Assert(State->TotalSize == bytesToWrite);
    WriteFile(State->RecordingHandle, State->GameMemoryBlock, bytesToWrite, &bytesToWrite, 0);
}

internal void Win32EndRecordingInput(win32_state* State)
{
    CloseHandle(State->RecordingHandle);
    State->InputRecordingIndex = 0;
}

internal void Win32RecordInput(win32_state* State, game_input* Input)
{
    DWORD bytesWritten;
    WriteFile(State->RecordingHandle, Input, sizeof(*Input), &bytesWritten, 0);
}

internal void Win32BeginInputPlayback(win32_state* State, int InputPlayingIndex)
{
    State->InputPlayingIndex = InputPlayingIndex;

    char fileName[WIN32_STATE_FILE_NAME_COUNT];
    Win32GetInputFileLocation(State, 1, sizeof(fileName), fileName);
    State->PlaybackHandle = CreateFileA(fileName, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);

    DWORD bytesToRead = (DWORD)State->TotalSize;
    Assert(State->TotalSize == bytesToRead);
    ReadFile(State->PlaybackHandle, State->GameMemoryBlock, bytesToRead, &bytesToRead, 0);
}

internal void Win32EndInputPlayback(win32_state* State)
{
    CloseHandle(State->PlaybackHandle);
    State->InputPlayingIndex = 0;
}

internal void Win32PlaybackInput(win32_state* State, game_input* Input)
{
    DWORD bytesRead = 0;
    if (ReadFile(State->PlaybackHandle, Input, sizeof(*Input), &bytesRead, 0))
    {
        if (bytesRead == 0)
        {
            int playingIndex = State->InputPlayingIndex;
            Win32EndInputPlayback(State);
            Win32BeginInputPlayback(State, playingIndex);
        }
    }
}
/******************************************************************/

/**************************** Input *******************************/
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub)
{
    return ERROR_DEVICE_NOT_CONNECTED;
}
global_variable x_input_get_state* XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub)
{
    return ERROR_DEVICE_NOT_CONNECTED;
}
global_variable x_input_set_state* XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

internal void Win32LoadXInput()
{
    HMODULE xInputLibrary = LoadLibraryA("xinput1_4.dll");
    if (!xInputLibrary)
    {
        xInputLibrary = LoadLibraryA("xinput1_3.dll");
    }
    if (xInputLibrary)
    {
        XInputGetState = (x_input_get_state*)GetProcAddress(xInputLibrary, "XInputGetState");
        if (!XInputGetState) { XInputGetState = XInputGetStateStub; }

        XInputSetState = (x_input_set_state*)GetProcAddress(xInputLibrary, "XInputSetState");
        if (!XInputSetState) { XInputSetState = XInputSetStateStub; }
    }
}

internal void Win32ProcessKeyboardMessage(game_button_state* NewState, bool32 IsDown)
{
    NewState->EndedDown = IsDown;
    ++NewState->HalfTransitionCount;
}

internal void ToggleFullscreen(HWND Window)
{
    // >Note: Copied code from internet
    DWORD style = GetWindowLong(Window, GWL_STYLE);
    if (style & WS_OVERLAPPEDWINDOW)
    {
        MONITORINFO monitorInfo = { sizeof(monitorInfo) };
        if (GetWindowPlacement(Window, &GlobalWindowPosition) &&
            GetMonitorInfo(MonitorFromWindow(Window, MONITOR_DEFAULTTOPRIMARY), &monitorInfo))
        {
            SetWindowLong(Window, GWL_STYLE, style & ~WS_OVERLAPPEDWINDOW);
            SetWindowPos(Window, HWND_TOP,
                monitorInfo.rcMonitor.left, monitorInfo.rcMonitor.top,
                monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left,
                monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top,
                SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        }
    }
    else
    {
        SetWindowLong(Window, GWL_STYLE, style | WS_OVERLAPPEDWINDOW);
        SetWindowPlacement(Window, &GlobalWindowPosition);
        SetWindowPos(Window, 0, 0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    }
}

internal void Win32HandleKeyboardInput(MSG *Message, win32_state* State, WPARAM WParam, LPARAM LParam, game_controller_input* KeyboardController)
{
    uint32 vKCode = (uint32)WParam;
    bool wasDown = ((LParam & (1 << 30)) != 0);
    bool isDown = ((LParam & (1 << 31)) == 0);
    if (wasDown != isDown)
    {
        if (vKCode == 'W')
        {
            Win32ProcessKeyboardMessage(&KeyboardController->MoveUp, isDown);
        }
        else if (vKCode == 'S')
        {
            Win32ProcessKeyboardMessage(&KeyboardController->MoveDown, isDown);
        }
        else if (vKCode == 'A')
        {
            Win32ProcessKeyboardMessage(&KeyboardController->MoveLeft, isDown);
        }
        else if (vKCode == 'D')
        {
            Win32ProcessKeyboardMessage(&KeyboardController->MoveRight, isDown);
        }
        else if (vKCode == 'Q')
        {
            Win32ProcessKeyboardMessage(&KeyboardController->LeftShoulder, isDown);
        }
        else if (vKCode == 'E')
        {
            Win32ProcessKeyboardMessage(&KeyboardController->RightShoulder, isDown);
        }
        else if (vKCode == VK_UP)
        {
            Win32ProcessKeyboardMessage(&KeyboardController->MoveUp, isDown);
        }
        else if (vKCode == VK_DOWN)
        {
            Win32ProcessKeyboardMessage(&KeyboardController->MoveDown, isDown);
        }
        else if (vKCode == VK_LEFT)
        {
            Win32ProcessKeyboardMessage(&KeyboardController->MoveLeft, isDown);
        }
        else if (vKCode == VK_RIGHT)
        {
            Win32ProcessKeyboardMessage(&KeyboardController->MoveRight, isDown);
        }
        else if (vKCode == VK_SPACE)
        {
            Win32ProcessKeyboardMessage(&KeyboardController->ActionUp, isDown);
        }
        else if (vKCode == VK_ESCAPE)
        {
            Win32ProcessKeyboardMessage(&KeyboardController->Back, isDown);
            //temp fix:
            GlobalRunning = false;
        }
        else if(vKCode == 'L')
        {
            if(isDown)
            {
                if(!State->InputRecordingIndex)
                {
                    Win32BeginRecordingInput(State, 1);
                }
                else
                {
                    Win32EndRecordingInput(State);
                    Win32BeginInputPlayback(State, 1);
                }
            }
        }
    }

    bool32 altIsDown = (LParam & (1 << 29));
    if ((vKCode == VK_F4) && altIsDown && isDown)
    {
        GlobalRunning = false;
    }
    if ((vKCode == VK_RETURN) && altIsDown && isDown)
    {
        ToggleFullscreen(Message->hwnd);
    }
}

internal void Win32HandleWindowsMessageLoop(win32_state* State, game_controller_input* KeyboardController)
{
    MSG message;
    while (PeekMessage(&message, 0, 0, 0, PM_REMOVE))
    {
        switch (message.message)
        {
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP:
        {
            Win32HandleKeyboardInput(&message, State, message.wParam, message.lParam, KeyboardController);
        } break;

        default:
        {
            TranslateMessage(&message);
            DispatchMessageA(&message);
        } break;
        }
        if (message.message == WM_QUIT)
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
    float result = 0;
    if (Value > DeadZoneThreshold)
    {
        result = (float)Value / 32768.0f;
    }
    else if (Value < -DeadZoneThreshold)
    {
        result = (float)Value / 32767.0f;
    }
    return result;
}

internal void Win32HandleControllerInput(HWND Window, game_input* OldInput, game_input* NewInput)
{
    DWORD maxControllerCount = XUSER_MAX_COUNT;
    if (maxControllerCount > (ArrayCount(NewInput->Controllers) - 1))
    {
        maxControllerCount = (ArrayCount(NewInput->Controllers) - 1);
    }

    POINT mousePoint;
    GetCursorPos(&mousePoint);
    ScreenToClient(Window, &mousePoint);
    NewInput->MouseX = mousePoint.x;
    NewInput->MouseY = mousePoint.y;
    NewInput->MouseZ = 0;
    Win32ProcessKeyboardMessage(&NewInput->MouseButtons[0], (GetKeyState(VK_LBUTTON) & (1 << 15)));
    Win32ProcessKeyboardMessage(&NewInput->MouseButtons[1], (GetKeyState(VK_MBUTTON) & (1 << 15)));
    Win32ProcessKeyboardMessage(&NewInput->MouseButtons[2], (GetKeyState(VK_RBUTTON) & (1 << 15)));
    Win32ProcessKeyboardMessage(&NewInput->MouseButtons[3], (GetKeyState(VK_XBUTTON1) & (1 << 15)));
    Win32ProcessKeyboardMessage(&NewInput->MouseButtons[4], (GetKeyState(VK_XBUTTON2) & (1 << 15)));

    for (DWORD controllerIndex = 0; controllerIndex < maxControllerCount; ++controllerIndex)
    {
        // Index 0 is for the keyboard, so we icrement 1.
        DWORD ourControllerIndex = controllerIndex + 1;
        game_controller_input* oldController = GetController(OldInput, ourControllerIndex);
        game_controller_input* newController = GetController(NewInput, ourControllerIndex);

        // XINPUT always expects index 0 for the first controller.
        XINPUT_STATE controllerState;
        if (XInputGetState(controllerIndex, &controllerState) == ERROR_SUCCESS)
        {
            newController->IsConnected = true;
            newController->IsAnalog = oldController->IsAnalog;

            // This controller is pluged in.
            XINPUT_GAMEPAD* pad = &controllerState.Gamepad;

            newController->StickAverage.X = Win32ProcessXInputStickValue(
                pad->sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
            newController->StickAverage.Y = Win32ProcessXInputStickValue(
                pad->sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);

            if(newController->StickAverage.X != 0 || newController->StickAverage.Y)
            {
                newController->IsAnalog = true;
            }

            // Analog
            float threshold = 0.5f;
            Win32ProcessXinputDigitalButton(
                (newController->StickAverage.Y > -threshold) ? 1 : 0,
                &oldController->MoveDown, 1,
                &newController->MoveDown);
            Win32ProcessXinputDigitalButton(
                (newController->StickAverage.Y > threshold) ? 1 : 0,
                &oldController->MoveUp, 1,
                &newController->MoveUp);
            Win32ProcessXinputDigitalButton(
                (newController->StickAverage.X < -threshold) ? 1 : 0,
                &oldController->MoveLeft, 1,
                &newController->MoveLeft);
            Win32ProcessXinputDigitalButton(
                (newController->StickAverage.X > threshold) ? 1 : 0,
                &oldController->MoveRight, 1,
                &newController->MoveRight);

            // Gamepad Buttons
            Win32ProcessXinputDigitalButton(pad->wButtons,
                &oldController->ActionDown, XINPUT_GAMEPAD_A,
                &newController->ActionDown);
            Win32ProcessXinputDigitalButton(pad->wButtons,
                &oldController->ActionRight, XINPUT_GAMEPAD_B,
                &newController->ActionRight);
            Win32ProcessXinputDigitalButton(pad->wButtons,
                &oldController->ActionLeft, XINPUT_GAMEPAD_X,
                &newController->ActionLeft);
            Win32ProcessXinputDigitalButton(pad->wButtons,
                &oldController->ActionUp, XINPUT_GAMEPAD_Y,
                &newController->ActionUp);

            // Dpad Buttons
            Win32ProcessXinputDigitalButton(pad->wButtons,
                &oldController->MoveDown, XINPUT_GAMEPAD_DPAD_DOWN,
                &newController->MoveDown);
            Win32ProcessXinputDigitalButton(pad->wButtons,
                &oldController->MoveUp, XINPUT_GAMEPAD_DPAD_UP,
                &newController->MoveUp);
            Win32ProcessXinputDigitalButton(pad->wButtons,
                &oldController->MoveLeft, XINPUT_GAMEPAD_DPAD_LEFT,
                &newController->MoveLeft);
            Win32ProcessXinputDigitalButton(pad->wButtons,
                &oldController->MoveRight, XINPUT_GAMEPAD_DPAD_RIGHT,
                &newController->MoveRight);

            // Shoulder Buttons
            Win32ProcessXinputDigitalButton(pad->wButtons,
                &oldController->RightShoulder, XINPUT_GAMEPAD_RIGHT_SHOULDER,
                &newController->RightShoulder);
            Win32ProcessXinputDigitalButton(pad->wButtons,
                &oldController->LeftShoulder, XINPUT_GAMEPAD_LEFT_SHOULDER,
                &newController->LeftShoulder);

            // Start & Back
            Win32ProcessXinputDigitalButton(pad->wButtons,
                &oldController->Start, XINPUT_GAMEPAD_START,
                &newController->Start);
            Win32ProcessXinputDigitalButton(pad->wButtons,
                &oldController->Back, XINPUT_GAMEPAD_BACK,
                &newController->Back);

            if (newController->Back.EndedDown)
            {
                GlobalRunning = false;
            }
        }
        else
        {
            newController->IsConnected = false;
        }
    }
}
/******************************************************************/
inline LARGE_INTEGER Win32GetWallClock()
{
    LARGE_INTEGER result;
    QueryPerformanceCounter(&result);
    return result;
}

inline float Win32GetSecondsElapsed(LARGE_INTEGER Start, LARGE_INTEGER End)
{
    float result = ((float)(End.QuadPart - Start.QuadPart) / (float)GlobalPerfCountFrequency);
    return result;
}

/**************************** WinMain *******************************/
int CALLBACK WinMain( HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CommandLine, int ShowCode)
{
    LARGE_INTEGER perfCountFrequencyResult;
    QueryPerformanceFrequency(&perfCountFrequencyResult);  // Fixed at system boot and consistent across all processors.
    GlobalPerfCountFrequency = perfCountFrequencyResult.QuadPart;

    win32_state win32State = {};
    Win32GetEXEFileName(&win32State);

    char sourceGameCodeDLLFullPath[WIN32_STATE_FILE_NAME_COUNT];
    Win32BuildEXEPathFileName(&win32State, "folayfila.dll", sizeof(sourceGameCodeDLLFullPath), sourceGameCodeDLLFullPath);

    char tempGameCodeDLLFullPath[WIN32_STATE_FILE_NAME_COUNT];
    Win32BuildEXEPathFileName(&win32State, "folayfila_temp.dll", sizeof(tempGameCodeDLLFullPath), tempGameCodeDLLFullPath);

    // Set the Windows scheduler granularity to 1ms
    // so the our Sleep can be more granular.
    UINT desiredSchedulerMS = 1;
    bool32 sleepIsGranular = (timeBeginPeriod(desiredSchedulerMS) == TIMERR_NOERROR);

#if FOLAYFILA_INTERNAL
    GlobalShowCursor = true;
#endif
    Win32ResizeDIBSection(&GlobalBackBuffer, 960, 540);

    WNDCLASSA windowClass = {};
    windowClass.style =  CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = Win32MainWindowCallback;
    windowClass.hInstance = Instance;
    windowClass.hCursor = LoadCursorA(0, IDC_CROSS);
    //WindowClass.hIcon = ;
    windowClass.lpszClassName = "HandmadeFolayfilaWindowClass";
    windowClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);

    if (!RegisterClassA(&windowClass))
    {
        return 0;
    }

    HWND window = CreateWindowExA(
            0,//WS_EX_TOPMOST,
            windowClass.lpszClassName, "Handmade Folayfila",
            WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
            0, 0, Instance, 0);

    if (!window)
    {
        return 0;
    }

    int monitorRefreshHz = 60;
    HDC refreshDC = GetDC(window);
    int win32RefreshRate = GetDeviceCaps(refreshDC, VREFRESH);
    ReleaseDC(window, refreshDC);    
    if(win32RefreshRate > 1)
    {
        monitorRefreshHz = win32RefreshRate;
    }
    float gameUpdateHz = (float)(monitorRefreshHz/2);
    float targetSecondsPerFrame = 1.0f / gameUpdateHz;

    int framesOfAudioLatency = 3;
    win32_sound_output soundOutput = {};
    soundOutput.SamplesPerSecond = 48000;
    soundOutput.RunningSampleIndex = 0;
    soundOutput.BytesPerSample = sizeof(int16) * 2;
    soundOutput.SecondaryBufferSize = soundOutput.SamplesPerSecond * soundOutput.BytesPerSample;
    soundOutput.LatencySampleCount = (int)((float)framesOfAudioLatency*(float)(soundOutput.SamplesPerSecond / gameUpdateHz));
    Win32InitDSound(window, soundOutput.SamplesPerSecond, soundOutput.SecondaryBufferSize);
    Win32ClearSoundBuffer(&soundOutput);
    GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

    int16* samples = (int16*)VirtualAlloc(0, soundOutput.SecondaryBufferSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);;

#if FOLAYFILA_INTERNAL
    LPVOID baseAddress = (LPVOID)Gigabytes((uint64)4);
#else
    LPVOID baseAddress = 0;
#endif  // FOLAYFILA_INTERNAL

    game_memory gameMemory = {};
    gameMemory.PermanentStorageSize = Megabytes(64);
    gameMemory.TransientStorageSize = Gigabytes((uint64)1);
    win32State.TotalSize = gameMemory.PermanentStorageSize + gameMemory.TransientStorageSize;
    win32State.GameMemoryBlock = VirtualAlloc(baseAddress, (size_t)win32State.TotalSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    gameMemory.PermanentStorage = win32State.GameMemoryBlock;
    gameMemory.TransientStorage = ((uint8*)gameMemory.PermanentStorage + gameMemory.PermanentStorageSize);
    gameMemory.DEBUGPlatformFreeFileMemory = DEBUGPlatformFreeFileMemory;
    gameMemory.DEBUGPlatformReadEntireFile = DEBUGPlatformReadEntireFile;
    gameMemory.DEBUGPlatformWriteEntireFile = DEBUGPlatformWriteEntireFile;

    if (!samples || !gameMemory.PermanentStorage || !gameMemory.TransientStorage)
    {
        // Can't run the game without memory.
        return 0;
    }

    Win32LoadXInput();
    game_input input[2] = {};
    game_input* newInput = &input[0];
    game_input* oldInput = &input[1];

    LARGE_INTEGER lastCounter = Win32GetWallClock();

    uint64 lastCycleCount = __rdtsc();

    DWORD lastPlayCursor = 0;
    bool32 soundIsValid = false;

    win32_game_code game = Win32LoadGameCode(sourceGameCodeDLLFullPath, tempGameCodeDLLFullPath);

    // Main Loop
    GlobalRunning = true;
    while (GlobalRunning)
    {
        newInput->DeltaTime = targetSecondsPerFrame;

        FILETIME newDLLWriteTime = Win32GetLastWriteTime(sourceGameCodeDLLFullPath);
        if(CompareFileTime(&newDLLWriteTime, &game.DLLLastWriteTime) != 0)
        {
            Win32UnloadGameCode(&game);
            game = Win32LoadGameCode(sourceGameCodeDLLFullPath, tempGameCodeDLLFullPath);
        }

        game_controller_input* oldKeyboardController = GetController(oldInput, 0);
        game_controller_input* newKeyboardController = GetController(newInput, 0);
        *newKeyboardController = {};
        newKeyboardController->IsConnected = true;
        for (int buttonIndex = 0; buttonIndex < ArrayCount(newKeyboardController->Buttons); ++buttonIndex)
        {
            newKeyboardController->Buttons[buttonIndex].EndedDown = oldKeyboardController->Buttons[buttonIndex].EndedDown;
        }
        Win32HandleWindowsMessageLoop(&win32State, newKeyboardController);
        Win32HandleControllerInput(window, oldInput, newInput);

        thread_context thread = {};

        DWORD byteToLock = 0;
        DWORD targetCursor = 0;
        DWORD bytesToWrite = 0;
        if (soundIsValid)
        {
            byteToLock = ((soundOutput.RunningSampleIndex * soundOutput.BytesPerSample) %
                soundOutput.SecondaryBufferSize);

            targetCursor = ((lastPlayCursor +
                (soundOutput.LatencySampleCount * soundOutput.BytesPerSample)) %
                soundOutput.SecondaryBufferSize);

            if (byteToLock > targetCursor)
            {
                bytesToWrite = (soundOutput.SecondaryBufferSize - byteToLock);
                bytesToWrite += targetCursor;
            }
            else
            {
                bytesToWrite = targetCursor - byteToLock;
            }
        }
        game_output_sound_buffer soundBuffer = {};
        soundBuffer.SamplesPerSecond = soundOutput.SamplesPerSecond;
        soundBuffer.SampleCount = bytesToWrite / soundOutput.BytesPerSample;
        soundBuffer.Samples = samples;

        game_graphics_buffer graphicsBuffer = {};
        graphicsBuffer.Memory = GlobalBackBuffer.Memory;
        graphicsBuffer.Width = GlobalBackBuffer.Width;
        graphicsBuffer.Height = GlobalBackBuffer.Height;
        graphicsBuffer.Pitch = GlobalBackBuffer.Pitch;
        graphicsBuffer.BytesPerPixel = GlobalBackBuffer.BytesPerPixel;

        if(win32State.InputRecordingIndex)
        {
            Win32RecordInput(&win32State, newInput);
        }
        if(win32State.InputPlayingIndex)
        {
            Win32PlaybackInput(&win32State, newInput);
        }

        if (game.UpdateAndRender)
        {
            game.UpdateAndRender(&thread, &gameMemory, newInput, &graphicsBuffer, &soundBuffer);
        }

        if (soundIsValid)
        {
            Win32FillSoundBuffer(&soundOutput, byteToLock, bytesToWrite, &soundBuffer);
        }

        LARGE_INTEGER workCounter = Win32GetWallClock();
        float workSecondsElapsed = Win32GetSecondsElapsed(lastCounter, workCounter);

        float secondsElapsedForFrame = workSecondsElapsed;
        while (secondsElapsedForFrame < targetSecondsPerFrame)
        {
            if (sleepIsGranular)
            {
                DWORD sleepMS = (DWORD)(1000.0f * (targetSecondsPerFrame - secondsElapsedForFrame));
                Sleep(sleepMS);
                secondsElapsedForFrame = Win32GetSecondsElapsed(lastCounter, Win32GetWallClock());
            }
        }

        LARGE_INTEGER endCounter = Win32GetWallClock();
        float msPerFrame = 1000.0f * Win32GetSecondsElapsed(lastCounter, endCounter);
        lastCounter = endCounter;

        win32_window_deminsion deminsion = Win32GetWindowDeminsion(window);

        HDC deviceContext = GetDC(window);
        Win32DisplayBufferInWindow(&GlobalBackBuffer, deviceContext, deminsion.Width, deminsion.Height);
        ReleaseDC(window, deviceContext);

        win32_debug_time_marker marker = {};
        if (SUCCEEDED(GlobalSecondaryBuffer->GetCurrentPosition(&marker.PlayCursor, &marker.WriteCursor)))
        {
            lastPlayCursor = marker.PlayCursor;
            if (!soundIsValid)
            {
                soundOutput.RunningSampleIndex = marker.WriteCursor / soundOutput.BytesPerSample;
            }
            soundIsValid = true;
        }
        else
        {
            soundIsValid = false;
        }

        // Refresh input for the next cycle.
        game_input* tempInput = newInput;
        newInput = oldInput;
        oldInput = tempInput;

        uint64 endCycleCount = __rdtsc();
        uint64 cyclesElapsed = endCycleCount - lastCycleCount;
        lastCycleCount = endCycleCount;


        float fps = 1000.0f / msPerFrame;
        float megaCyclesPerFrame = ((float)cyclesElapsed / (1000.0f * 1000.0f));

        char logBuffer[256];
        sprintf_s(logBuffer, "Milliseconds/frame: %.02f ms / %.02f FPS / %.02f MCPF\n", msPerFrame, fps, megaCyclesPerFrame);
        OutputDebugStringA(logBuffer);
    }
    return 0;
}