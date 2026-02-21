// (C) Copyright 2026 by Abdallah Maaliki / folayfila.

#include <windows.h>

int CALLBACK WinMain(
    HINSTANCE hInstance,
    HINSTANCE hPrevInstance,
    LPSTR     lpCmdLine,
    int       nCmdShow)
{
    MessageBoxA(0, "This is handmade folayfila!", "Folayfila", MB_OK|MB_ICONINFORMATION);

    return 0;
}