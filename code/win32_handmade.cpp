#include <windows.h>
#include <stdint.h>

#define internal static
#define local_persist static
#define global_variable static

//unsigned integers
typedef uint8_t u8;     // 1-byte long unsigned integer
typedef uint16_t u16;   // 2-byte long unsigned integer
typedef uint32_t u32;   // 4-byte long unsigned integer
typedef uint64_t u64;   // 8-byte long unsigned integer
// signed integers
typedef int8_t s8;      // 1-byte long signed integer
typedef int16_t s16;    // 2-byte long signed integer
typedef int32_t s32;    // 4-byte long signed integer
typedef int64_t s64;    // 8-byte long signed integer

global_variable bool Running;

global_variable BITMAPINFO BitmapInfo;
global_variable void *BitmapMemory;
global_variable int BitmapWidth;
global_variable int BitmapHeight;
global_variable int BytesPerPixel;

internal void
RenderWeirdGradient(int XOffset, int YOffset)
{
    int Pitch = BitmapWidth * BytesPerPixel;
    u8 *Row = (u8 *)BitmapMemory;
    for (int Y = 0; Y < BitmapHeight; ++Y)
    {
        u32 *Pixel = (u32 *)Row;
        for(int X = 0; X < BitmapWidth; ++X)
        {
            //
            // Pixel in memory: BB GG RR XX
            //
            u8 Red = 0;
            u8 Green = (u8)(Y + YOffset);
            u8 Blue = (u8)(X + XOffset);
            
            *Pixel++ = Red << 16 | Green <<8 | Blue;// << 0
        }
        Row += Pitch;
    }
}

internal void
Win32ResizeDIBSection(int Width, int Height)
{
    if(BitmapMemory)
    {
        VirtualFree(BitmapMemory, 0, MEM_RELEASE);
    }
    
    BitmapWidth = Width;
    BitmapHeight = Height;
    
    BitmapInfo.bmiHeader.biSize = sizeof(BitmapInfo.bmiHeader);
    BitmapInfo.bmiHeader.biWidth = BitmapWidth;
    BitmapInfo.bmiHeader.biHeight = -BitmapHeight;//negative value: top-down pitch
    BitmapInfo.bmiHeader.biPlanes = 1;
    BitmapInfo.bmiHeader.biBitCount = 32;
    BitmapInfo.bmiHeader.biCompression = BI_RGB;
    
    BytesPerPixel = 4;
    int BitmapMemorySize = (BitmapWidth*BitmapHeight)*BytesPerPixel;
    BitmapMemory = VirtualAlloc(0, BitmapMemorySize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
}
internal void
Win32UpdateWindow(HDC DeviceContext, RECT *ClientRect)
{
    int WindowWidth = ClientRect->right - ClientRect->left;
    int WindowHeight = ClientRect->bottom - ClientRect->top;
    StretchDIBits(DeviceContext,
                  0, 0, WindowWidth, WindowHeight,//Destination rectangle (window)
                  0, 0, BitmapWidth, BitmapHeight,//Source rectangle (bitmap buffer)
                  BitmapMemory,
                  &BitmapInfo,
                  DIB_RGB_COLORS, SRCCOPY);
}

LRESULT CALLBACK
Win32MainWindowCallback(HWND Window,
                        UINT Message,
                        WPARAM WParam,
                        LPARAM LParam)
{
    LRESULT Result = 0;
    
    switch(Message)
    {
        case WM_SIZE:
        {
            RECT ClientRect;
            GetClientRect(Window, &ClientRect);
            int Width = ClientRect.right - ClientRect.left;
            int Height = ClientRect.bottom - ClientRect.top;
            Win32ResizeDIBSection(Width, Height);
            //OutputDebugStringA("WM_SIZE\n");
        } break;
        
        case WM_CLOSE:
        {
            Running = false;
            DestroyWindow(Window);
            OutputDebugStringA("WM_CLOSE\n");
        } break;
        
        case WM_DESTROY:
        {
            Running = false;
            PostQuitMessage(0);
            OutputDebugStringA("WM_DESTROY\n");
        } break;
        
        case WM_ACTIVATEAPP:
        {
            OutputDebugStringA("WM_ACTIVATEAPP\n");
        } break;
        
        case WM_PAINT:
        {
            PAINTSTRUCT Paint;
            HDC DeviceContext = BeginPaint(Window, &Paint);
            
            RECT ClientRect;
            GetClientRect(Window, &ClientRect);
            
            Win32UpdateWindow(DeviceContext, &ClientRect);
            EndPaint(Window, &Paint);
        } break;
        
        default:
        {
            //OutputDebugStringA("default\n");
            Result = DefWindowProc(Window, Message, WParam, LParam);
        } break;
    }
    
    return(Result);
}

int CALLBACK
WinMain (HINSTANCE Instance,
         HINSTANCE PrevInstance,
         LPSTR CommandLine,
         int ShowCode)
{
    WNDCLASS WindowClass = {};
    
    WindowClass.style = CS_OWNDC|CS_HREDRAW|CS_VREDRAW;
    WindowClass.lpfnWndProc = Win32MainWindowCallback;
    WindowClass.hInstance = Instance;
    //WindowClass.hIcon;
    WindowClass.lpszClassName = "HandmadeHeroWindowClass";
    
    if(RegisterClass(&WindowClass))
    {
        HWND Window =
            CreateWindowEx(
                           0,
                           WindowClass.lpszClassName,
                           "Handmade Hero",
                           WS_OVERLAPPEDWINDOW|WS_VISIBLE,
                           CW_USEDEFAULT,
                           CW_USEDEFAULT,
                           CW_USEDEFAULT,
                           CW_USEDEFAULT,
                           0,
                           0,
                           Instance,
                           0);
        if(Window)
        {
            int XOffset = 0;
            int YOffset = 0;
            Running = true;
            while(Running)
            {
                MSG Message;
                while(PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
                {
                    if(Message.message == WM_QUIT)
                    {
                        Running = false;
                    }
                    TranslateMessage(&Message);
                    DispatchMessage(&Message);
                }
                
                RenderWeirdGradient(XOffset, YOffset);
                ++XOffset;
                
                HDC DeviceContext = GetDC(Window);
                RECT ClientRect;
                GetClientRect(Window, &ClientRect);
                
                Win32UpdateWindow(DeviceContext, &ClientRect);
                
                ReleaseDC(Window, DeviceContext);
            }
            
        }
        else
        {
            // TODO(banunu): Logging
        }
    }
    else
    {
        // TODO(banunu): Logging
    }
    
    return(0);
}
