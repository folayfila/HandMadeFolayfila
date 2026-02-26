// (C) Copyright 2026 by Abdallah Maaliki / folayfila.

#include "win32_folayfila.h"
#include <xinput.h>
#include <dsound.h>

// TODO(abdallah): Global for now.
global_variable bool GlobalRunning;
global_variable win32_offscreen_buffer GlobalBackBuffer;
global_variable LPDIRECTSOUNDBUFFER GlobalSecondaryBuffer;

// Note(abd): This is our support for XInputGetState.
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub)
{
    return ERROR_DEVICE_NOT_CONNECTED;
}
global_variable x_input_get_state *XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

// Note(abd): This is our support for XInputSetState.
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub)
{
    return ERROR_DEVICE_NOT_CONNECTED;
}
global_variable x_input_set_state *XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND* ppDS, LPUNKNOWN pUnkOuter);
typedef DIRECT_SOUND_CREATE(direct_sound_create);

internal void Win32LoadXInput()
{
    HMODULE XInputLibrary = LoadLibraryA("xinput1_4.dll");
    if (!XInputLibrary)
    {
        XInputLibrary = LoadLibraryA("xinput1_3.dll");
    }
    if (XInputLibrary)
    {
        XInputGetState = (x_input_get_state *)GetProcAddress(XInputLibrary, "XInputGetState");
        if (!XInputGetState)
        { XInputGetState = XInputGetStateStub; }

        XInputSetState = (x_input_set_state *)GetProcAddress(XInputLibrary, "XInputSetState");
        if (!XInputSetState) { XInputSetState = XInputSetStateStub; }
    }
}

internal win32_window_deminsion Win32GetWindowDeminsion(HWND Window)
{
    win32_window_deminsion Result;

    RECT ClientRect;
    GetClientRect(Window, &ClientRect);
    Result.Width = ClientRect.right - ClientRect.left;
    Result.Height = ClientRect.bottom - ClientRect.top;

    return Result;
}

internal void Win32InitDSound(HWND Window, int32_t SamplesPerSeconds, int32_t BufferSize)
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
                else
                {
                    //TODO: Log
                }
            }
            else
            {

            }
        }
        else
        {
            //TODO: Log
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
        else
        {

        }
    }
    else
    {
        //TODO: Log
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

internal void Win32DisplayBufferInWindow(win32_offscreen_buffer *Buffer,
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
            uint32_t VKCode = WParam;
            bool WasDown = ((LParam & (1 << 30)) != 0);
            bool IsDown = ((LParam & (1 << 31)) == 0);
            if (WasDown != IsDown)
            {
                if (VKCode == 'W')
                {
                    //YOffset-=10;
                }
                else if (VKCode == 'S')
                {
                    //YOffset+=10;
                }
                else if (VKCode == 'A')
                {
                    //XOffset+=10;
                }
                else if (VKCode == 'D')
                {
                    //XOffset-=10;
                }
                else if (VKCode == 'Q')
                {
                }
                else if (VKCode == 'E')
                {
                }
                else if (VKCode == VK_UP)
                {
                }
                else if (VKCode == VK_DOWN)
                {
                }
                else if (VKCode == VK_LEFT)
                {
                }
                else if (VKCode == VK_RIGHT)
                {
                }
                else if (VKCode == VK_SPACE)
                {
                }
                else if (VKCode == VK_ESCAPE)
                {
                    GlobalRunning = false;
                }
            }

            bool32 AltIsDown = (LParam & (1 << 29));
            if (VKCode == VK_F4 && AltIsDown)
            {
                GlobalRunning = false;
            }
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
            Result = DefWindowProc(Window, Message, WParam, LParam);
        } break;
    }

    return Result;
}

internal void Win32ProcessXinputDigitalButton(DWORD XInputButtonState,
                                              game_button_state* OldState, DWORD ButtonBit,
                                              game_button_state* NewState)
{
    NewState->EndedDown = ((XInputButtonState & ButtonBit) == ButtonBit);
    NewState->HalfTransitionCount = (OldState->EndedDown != NewState->EndedDown) ? 1 : 0;
}

internal void Win32HandleControllerInput(game_input* OldInput, game_input* NewInput)
{
    int MaxControllerCount = XUSER_MAX_COUNT;
    if (MaxControllerCount > ArrayCount(NewInput->Controllers))
    {
        MaxControllerCount = ArrayCount(NewInput->Controllers);
    }

    for (DWORD ControllerIndex = 0; ControllerIndex < MaxControllerCount; ++ControllerIndex)
    {
        XINPUT_STATE ControllerState;
        if (XInputGetState(ControllerIndex, &ControllerState) == ERROR_SUCCESS)
        {
            game_controller_input* OldController = &OldInput->Controllers[ControllerIndex];
            game_controller_input* NewController = &NewInput->Controllers[ControllerIndex];

            // This controller is pluged in.
            XINPUT_GAMEPAD* Pad = &ControllerState.Gamepad;

            float X;
            if (Pad->sThumbLX < 0)
            {
                X = (float)Pad->sThumbLX / 32768.0f;
            }
            else
            {
                X = (float)Pad->sThumbLX / 32768.0f;
            }
            float Y;
            if (Pad->sThumbLY < 0)
            {
                Y = (float)Pad->sThumbLY / 32768.0f;
            }
            else
            {
                Y = (float)Pad->sThumbLY / 32768.0f;
            }

            NewController->IsAnalog = true;
            NewController->Start.X = OldController->Start.X;
            NewController->Start.Y = OldController->Start.Y;

            NewController->Min.X = NewController->Max.X = NewController->End.X = X;
            NewController->Min.Y = NewController->Max.Y = NewController->End.Y = Y;

            // Gamepad Buttons
            Win32ProcessXinputDigitalButton(Pad->wButtons,
                                            &OldController->Down, XINPUT_GAMEPAD_A,
                                            &NewController->Down);
            Win32ProcessXinputDigitalButton(Pad->wButtons,
                                            &OldController->Right, XINPUT_GAMEPAD_B,
                                            &NewController->Right);
            Win32ProcessXinputDigitalButton(Pad->wButtons,
                                            &OldController->Left, XINPUT_GAMEPAD_X,
                                            &NewController->Left);
            Win32ProcessXinputDigitalButton(Pad->wButtons,
                                            &OldController->Up, XINPUT_GAMEPAD_Y,
                                            &NewController->Up);
            // Shoulder Buttons
            Win32ProcessXinputDigitalButton(Pad->wButtons,
                                            &OldController->RightShoulder, XINPUT_GAMEPAD_RIGHT_SHOULDER,
                                            &NewController->RightShoulder);
            Win32ProcessXinputDigitalButton(Pad->wButtons,
                                            &OldController->LeftShoulder, XINPUT_GAMEPAD_LEFT_SHOULDER,
                                            &NewController->LeftShoulder);

            //// Start and Back
            // Win32ProcessXinputDigitalButton(Pad->wButtons,
            //                                OldController->State, XINPUT_GAMEPAD_START,
            //                                NewController->State);
            // Win32ProcessXinputDigitalButton(Pad->wButtons,
            //                                OldController->State, XINPUT_GAMEPAD_BACK,
            //                                NewController->State);
            //// DPad
            //Win32ProcessXinputDigitalButton(Pad->wButtons,
            //                                OldController->State, XINPUT_GAMEPAD_DPAD_UP,
            //                                NewController->State);
            //Win32ProcessXinputDigitalButton(Pad->wButtons,
            //                                OldController->State, XINPUT_GAMEPAD_DPAD_DOWN,
            //                                NewController->State);
            //Win32ProcessXinputDigitalButton(Pad->wButtons,
            //                                OldController->State, XINPUT_GAMEPAD_DPAD_LEFT,
            //                                NewController->State);
            //Win32ProcessXinputDigitalButton(Pad->wButtons,
            //                                OldController->State, XINPUT_GAMEPAD_DPAD_RIGHT,
            //                                NewController->State);

            XINPUT_VIBRATION Vibration;
            Vibration.wLeftMotorSpeed = 50000;
            Vibration.wRightMotorSpeed = 50000;
            /*XINPUT_VIBRATION ZeroVibration;
            ZeroVibration.wLeftMotorSpeed = 0;
            ZeroVibration.wRightMotorSpeed = 0;
            XInputSetState(0, &ZeroVibration);*/
        }
        else
        {
            // Controller not available.
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
        uint8_t* DestSample = (uint8_t*)Region1;
        for (DWORD ByteIndex = 0; ByteIndex < Region1Size; ++ByteIndex)
        {
            *DestSample++ = 0;
        }
        
        DestSample = (uint8_t*)Region2;
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
        int16_t* SourceSample = SourceBuffer->Samples;
        int16_t* DestSample = (int16_t*)Region1;
        for (DWORD SampleIndex = 0; SampleIndex < Region1SampleCount; ++SampleIndex)
        {
            *DestSample++ = *SourceSample++;
            *DestSample++ = *SourceSample++;
            ++SoundOutput->RunningSampleIndex;
        }

        DWORD Region2SampleCount = Region2Size / SoundOutput->BytesPerSample;
        DestSample = (int16_t*)Region2;
        for (DWORD SampleIndex = 0; SampleIndex < Region2SampleCount; ++SampleIndex)
        {
            *DestSample++ = *SourceSample++;
            *DestSample++ = *SourceSample++;
            ++SoundOutput->RunningSampleIndex;
        }
        GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
    }
}

int CALLBACK WinMain(
    HINSTANCE Instance,
    HINSTANCE PrevInstance,
    LPSTR     CommandLine,
    int       ShowCode)
{
    LARGE_INTEGER PerfCountFrequencyResult;
    QueryPerformanceFrequency(&PerfCountFrequencyResult); // Fixed at system boot and consistent across all processors.
    int64_t PerfCountFrequency = PerfCountFrequencyResult.QuadPart;

    Win32LoadXInput();

    WNDCLASSA WindowClass = {};

    Win32ResizeDIBSection(&GlobalBackBuffer, 1280, 720);

    WindowClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
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
                WS_OVERLAPPEDWINDOW | WS_VISIBLE,
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
            // Since we specified CS_OWNDC, we can just get one device
            // context and use it forever as we're not sharing it with anyone.
            HDC DeviceContext = GetDC(Window);

            win32_sound_output SoundOutput = {};
            SoundOutput.SamplesPerSecond = 48000;
            SoundOutput.RunningSampleIndex = 0;
            SoundOutput.BytesPerSample = sizeof(int16_t) * 2;
            SoundOutput.SecondaryBufferSize = SoundOutput.SamplesPerSecond * SoundOutput.BytesPerSample;
            SoundOutput.LatencySampleCount = SoundOutput.SamplesPerSecond / 15;
            Win32InitDSound(Window, SoundOutput.SamplesPerSecond, SoundOutput.SecondaryBufferSize);
            Win32ClearSoundBuffer(&SoundOutput);
            GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

            int16_t* Samples = (int16_t*)VirtualAlloc(0, SoundOutput.SecondaryBufferSize,
                                                      MEM_COMMIT, PAGE_READWRITE);

            game_input Input[2] = {};
            game_input* NewInput = &Input[0];
            game_input* OldInput = &Input[1];

            LARGE_INTEGER LastCounter;
            QueryPerformanceCounter(&LastCounter);
            uint64_t LastCycleCount = __rdtsc();

            GlobalRunning = true;
            while (GlobalRunning)
            {
                MSG Message;

                while (PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
                {
                    if (Message.message == WM_QUIT)
                    {
                        GlobalRunning = false;
                    }

                    TranslateMessage(&Message);
                    DispatchMessageA(&Message);
                }

                // Handle Input
                Win32HandleControllerInput(OldInput, NewInput);

                DWORD ByteToLock;
                DWORD TargetCursor;
                DWORD BytesToWrite;
                DWORD PlayCursor;
                DWORD WriteCursor;
                bool32 SoundIsValid = false;
                if (SUCCEEDED(GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor)))
                {
                    // NOTE(abdallah): DirectSound output test
                    ByteToLock = (SoundOutput.RunningSampleIndex * SoundOutput.BytesPerSample) %
                        SoundOutput.SecondaryBufferSize;

                    TargetCursor = ((PlayCursor +
                        (SoundOutput.LatencySampleCount * SoundOutput.BytesPerSample)) %
                        SoundOutput.SecondaryBufferSize);

                    BytesToWrite;
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
                GameUpdateAndRender(NewInput, &GraphicsBuffer, &SoundBuffer);

                if (SoundIsValid)
                {
                    Win32FillSoundBuffer(&SoundOutput, ByteToLock, BytesToWrite, &SoundBuffer);
                }

                win32_window_deminsion Deminsion = Win32GetWindowDeminsion(Window);
                Win32DisplayBufferInWindow(&GlobalBackBuffer, DeviceContext, Deminsion.Width, Deminsion.Height);

                uint64_t EndCycleCount = __rdtsc();
                LARGE_INTEGER EndCounter;
                QueryPerformanceCounter(&EndCounter);

                // TODO: Display the time value here.
                int64_t CyclesElapsed = EndCycleCount - LastCycleCount;
                int32_t MegaCyclesPerFrame = (int32_t)(CyclesElapsed / (1000 * 1000));

                int64_t CounterElapsed = EndCounter.QuadPart - LastCounter.QuadPart;
                int32_t MSPerFrame = (int32_t)((1000*CounterElapsed) / PerfCountFrequency);
                int32_t FPS = (int32_t)(PerfCountFrequency / CounterElapsed);

                char LogBuffer[256];
                wsprintf(LogBuffer, "Milliseconds/frame: %d ms / %d FPS / %d MCPF\n", MSPerFrame, FPS, MegaCyclesPerFrame);
                OutputDebugStringA(LogBuffer);

                LastCycleCount = EndCycleCount;
                LastCounter = EndCounter;

                game_input* Temp = NewInput;
                NewInput = OldInput;
                OldInput = Temp;
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