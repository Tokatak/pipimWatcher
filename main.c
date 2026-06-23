#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "gdi32.lib")

#include <windows.h>
#include <stdio.h>
#include <stdint.h>
#include <sys/stat.h>
#include <time.h>

#define TIMER_SEC 1

typedef uint8_t uint8;
typedef uint32_t uint32;

typedef struct {
    BITMAPINFO Info;
    void* Memory;
    int Width;
    int Height;
    int Pitch;
    int BytesPerPixel;
} Win32_offscreen_buffer;

typedef struct {
    FILETIME LastWriteTime;
    FILETIME LastAccessTime;
    FILETIME CreationTime;
} FileState;


Win32_offscreen_buffer globalBackbuffer;
FileState fileState = { 0 };

int cxClient, cyClient;
int pixelsFrame = 10;
char filename[MAX_PATH];
char headerName[] = "PPM Watcher";
HBRUSH whiteBrush, crossBrush;

int GetFileState(const char* filename, FileState* state);
int CheckFileChanged(const char* filename, FileState* currentState);
void Win32ResizeDIBSection(Win32_offscreen_buffer* buffer, int width, int height);
int ReadPPMToBuffer_Safe(const char* filename, Win32_offscreen_buffer* buffer);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
    PSTR szCmdLine, int iCmdShow)
{
    HWND hwnd;
    MSG msg;
    WNDCLASS wndclass;
    wndclass.style = CS_HREDRAW | CS_VREDRAW;
    wndclass.lpfnWndProc = WndProc;
    wndclass.cbClsExtra = 0;
    wndclass.cbWndExtra = 0;
    wndclass.hInstance = hInstance;
    wndclass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wndclass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wndclass.lpszMenuName = NULL;
    wndclass.lpszClassName = headerName;

    if (!RegisterClass(&wndclass))
    {
        MessageBox(NULL, TEXT("This program requires Windows NT!"),
            headerName, MB_ICONERROR);
        return 0;
    }

    hwnd = CreateWindow(headerName,  // window class name
        TEXT(headerName),           // window caption
        WS_OVERLAPPEDWINDOW,        // window style
        CW_USEDEFAULT,              // initial x position
        CW_USEDEFAULT,              // initial y position
        CW_USEDEFAULT,              // initial x size
        CW_USEDEFAULT,              // initial y size
        NULL,                       // parent window handle
        NULL,                       // window menu handle
        hInstance,                  // program instance handle
        NULL);                      // creation parameters

    whiteBrush = (HBRUSH)GetStockObject(WHITE_BRUSH);
    crossBrush = CreateHatchBrush(HS_CROSS, 0xBBBBBB);

    ShowWindow(hwnd, iCmdShow);
    UpdateWindow(hwnd);
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    HDC hdc;
    PAINTSTRUCT ps;
    RECT rect;

    int aspect = 1;
    int xScale = 1, yScale = 1;

    switch (message)
    {
        case WM_CREATE:
            SetTimer(hwnd, TIMER_SEC, 1000, NULL);
            DragAcceptFiles(hwnd, TRUE);
            return 0;
        case WM_SIZE:
            cxClient = LOWORD(lParam);
            cyClient = HIWORD(lParam);
            return 0;
        case WM_DROPFILES:
        {
            HDROP hDrop = (HDROP)wParam;
            UINT fileCount = DragQueryFile(hDrop, 0xFFFFFFFF, NULL, 0);
            if (fileCount == 1) {
                TCHAR szFilePath[MAX_PATH];
                int index = 0;
                DragQueryFile(hDrop, index, szFilePath, MAX_PATH);

                snprintf(filename, sizeof(filename), szFilePath);

                ReadPPMToBuffer_Safe(filename, &globalBackbuffer);

                SetWindowText(hwnd, TEXT(filename));

                if (!GetFileState(filename, &fileState)) {
                    MessageBox(NULL, "Failed to get fileState", "Error", MB_OK);
                    return -1;
                }
                InvalidateRect(hwnd, NULL, TRUE);
            }
            DragFinish(hDrop);
            return 0;
        }
        case WM_PAINT:
            hdc = BeginPaint(hwnd, &ps);
            GetClientRect(hwnd, &rect);

            SelectObject(hdc, crossBrush);
            FillRect(hdc, &rect, crossBrush);
            SelectObject(hdc, whiteBrush);

            // Importnat: avoid billiniear filtration
            SetStretchBltMode(hdc, COLORONCOLOR);

            if (globalBackbuffer.Width != 0 && globalBackbuffer.Height != 0 && globalBackbuffer.Memory) {
                xScale = (cxClient - 2 * pixelsFrame) / globalBackbuffer.Width;
                yScale = (cyClient - 2 * pixelsFrame) / globalBackbuffer.Height;
                aspect = xScale < yScale ? xScale : yScale;
                if (aspect == 0) {
                    aspect = 1;
                }
                StretchDIBits(hdc,
                    pixelsFrame, pixelsFrame, globalBackbuffer.Width * aspect, globalBackbuffer.Height * aspect,
                    0, 0, globalBackbuffer.Width, globalBackbuffer.Height,
                    globalBackbuffer.Memory,
                    &globalBackbuffer.Info,
                    DIB_RGB_COLORS, SRCCOPY);
            }
            EndPaint(hwnd, &ps);
            return 0;
        case WM_TIMER:
            if (CheckFileChanged(filename, &fileState)) {

                ReadPPMToBuffer_Safe(filename, &globalBackbuffer);
                InvalidateRect(hwnd, NULL, TRUE);
            }
            return 0;
        case WM_DESTROY:
            // app closed
            // VirtualFree(GlobalBackbuffer.Memory, 0, MEM_RELEASE);
            // DeleteObject(crossBrush);
            KillTimer(hwnd, TIMER_SEC);
            DragAcceptFiles(hwnd, FALSE);
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hwnd, message, wParam, lParam);
}

int GetFileState(const char* filename, FileState* state)
{
    HANDLE hFile = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, NULL,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hFile == INVALID_HANDLE_VALUE) {
        return 0;
    }

    if (!GetFileTime(hFile, &state->CreationTime, &state->LastAccessTime, &state->LastWriteTime)) {
        CloseHandle(hFile);
        return 0;
    }

    CloseHandle(hFile);
    return 1;
}

int CheckFileChanged(const char* filename, FileState* currentState)
{
    FileState newState = { 0 };

    // File doesn't exist or can't be accessed
    if (!GetFileState(filename, &newState)) {
        return 0;
    }

    if (CompareFileTime(&currentState->LastWriteTime, &newState.LastWriteTime) != 0) {
        *currentState = newState;
        return 1;
    }

    return 0;
}

void
Win32ResizeDIBSection(Win32_offscreen_buffer* buffer, int width, int height) {
    if (buffer->Memory) {
        VirtualFree(buffer->Memory, 0, MEM_RELEASE);
    }

    buffer->Width = width;
    buffer->Height = height;

    int BytesPerPixel = 4;
    buffer->BytesPerPixel = BytesPerPixel;

    buffer->Info.bmiHeader.biSize = sizeof(buffer->Info.bmiHeader);
    buffer->Info.bmiHeader.biWidth = buffer->Width;
    buffer->Info.bmiHeader.biHeight = -buffer->Height;
    buffer->Info.bmiHeader.biPlanes = 1;
    buffer->Info.bmiHeader.biBitCount = 32;
    buffer->Info.bmiHeader.biCompression = BI_RGB;

    int BitmapMemorySize = (buffer->Width * buffer->Height) * BytesPerPixel;
    buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    buffer->Pitch = width * BytesPerPixel;
    // todo: probably clear to black?
}


int ReadPPMToBuffer_Safe(const char* filename, Win32_offscreen_buffer* buffer)
{
    FILE* file;
    fopen_s(&file, filename, "rb");
    if (!file) {
        MessageBox(NULL, "Failed to open PPM file", "Error", MB_OK);
        return 0;
    }

    char magic[3];
    fscanf_s(file, "%2s", magic, (unsigned)sizeof(magic));
    if (magic[0] != 'P' || (magic[1] != '3' && magic[1] != '6')) {
        MessageBox(NULL, "Not a valid PPM file", "Error", MB_OK);
        fclose(file);
        return 0;
    }
    int isBinary = (magic[1] == '6');

    int ch = fgetc(file);
    while (ch == '#') {
        while (fgetc(file) != '\n') {}  // Skip to end of line
        ch = fgetc(file);
    }
    ungetc(ch, file);

    // consider maxVal for normalizing
    int width, height, maxVal;
    fscanf_s(file, "%d %d %d", &width, &height, &maxVal);
    buffer->Width = width;
    buffer->Height = height;

    Win32ResizeDIBSection(buffer, buffer->Width, buffer->Height);

    int bytesPerPixel = 4;
    int pitch = width * bytesPerPixel;
    if (isBinary) {
        MessageBox(NULL, "isBinary needs check", "Error", MB_OK);

        //// Binary P6 format - read raw RGB data
        //unsigned char* pixel = (unsigned char*)buffer->Memory;
        //// Skip the whitespace after maxVal
        //fgetc(file);
        //for (int y = 0; y < height; y++) {
        //    for (int x = 0; x < width; x++) {
        //        // Read 3 bytes (R, G, B)
        //        unsigned char r, g, b;
        //        fread(&r, 1, 1, file);
        //        fread(&g, 1, 1, file);
        //        fread(&b, 1, 1, file);

        //        // Store as BGRA (Windows GDI format)
        //        int index = y * pitch + x * 4;
        //        pixel[index + 0] = b;  // Blue
        //        pixel[index + 1] = g;  // Green
        //        pixel[index + 2] = r;  // Red
        //        pixel[index + 3] = 255; // Alpha (opaque)
        //    }
        //}

    }
    else 
    {
        // ASCII P3 format - read text RGB values
        unsigned char* pixel = (unsigned char*)buffer->Memory;

        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                int r, g, b;
                fscanf_s(file, "%d %d %d", &r, &g, &b);

                // Store as BGRA
                int index = y * pitch + x * 4;
                pixel[index + 0] = (unsigned char)b;
                pixel[index + 1] = (unsigned char)g;
                pixel[index + 2] = (unsigned char)r;
                pixel[index + 3] = 255;
            }
        }
    }

    fclose(file);
    return 1;
}
