
//Command
//i686-w64-mingw32-gcc-win32 run.c -o "BAD_APPLE.EXE" my.o -lgdi32 -lgdiplus -lmsimg32 -municode -lwinmm -lshlwapi 
//-lopengl32 -lglu32 is not used for now Jan-06-2024 -credit: sothea.dev

//i686-w64-mingw32-windres my.rc -o my.o //for icon


#include <windows.h>
#include <gdiplus.h> //for gif

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <conio.h>
#include <stdbool.h>
#include <time.h>
#include <math.h>
#include <limits.h>
#include <dirent.h>
#include <direct.h>
#include <errno.h>
#include <shlwapi.h>


#define SCREEN_WIDTH    (GetSystemMetrics(SM_CXSCREEN))
#define SCREEN_HEIGHT   (GetSystemMetrics(SM_CYSCREEN))


//Global Variables Game state variables
//flags
bool flag_restart=FALSE;
bool flag_restart_audio=FALSE;
bool flag_adjust_audio=FALSE;
bool flag_adjust_wav_out_audio=FALSE;
bool load_sound=FALSE;
bool clean_up_sound=FALSE;
int FPS = 35; //minimum FPS, otherwise run according to screen refresh rate





//Game System Values
int windowx=0;
int windowy=0;

int GR_WIDTH;
int GR_HEIGHT;
int OLD_GR_WIDTH;
int OLD_GR_HEIGHT;
int RESOLUTION_X[3]={640,800,0};
int RESOLUTION_Y[3]={480,600,0};
int showoff=0;

wchar_t src_music_dir[64];


#include "gr.c"
#include "math.c"
#include "sound.c"
#include "song.c"
#include "draw_gui.c"

//Detect Exception occured
//https://stackoverflow.com/questions/1394250/detect-program-termination-c-windows
LONG myTry(LPEXCEPTION_POINTERS p)
{
   waveOutSetVolume(hWaveOut[2],wav_out_original_volume);
   return EXCEPTION_EXECUTE_HANDLER;
}

//Init
LARGE_INTEGER m_high_perf_timer_freq;
LARGE_INTEGER m_prev_end_of_frame;  
void InitTickFrequency() {
  if (!QueryPerformanceFrequency(&m_high_perf_timer_freq))
      m_high_perf_timer_freq.QuadPart = 0;
  m_prev_end_of_frame.QuadPart = 0;
}



void InitFPS() { //https://cboard.cprogramming.com/windows-programming/30730-finding-monitor-refresh-rate.html
  int index=0, currentfps=0;
  DEVMODE screen; 
  memset(&screen, 0, sizeof(DEVMODE));
  while(EnumDisplaySettings(NULL, index++, &screen)){
    //printf("The current refresh rate is %i\n", screen.dmDisplayFrequency); //debug
    currentfps=(int)screen.dmDisplayFrequency;
    //MessageBox(NULL, message, "Refresh Rate:", MB_OK);
    memset(&screen, 0, sizeof(DEVMODE));
    if (currentfps>FPS) FPS=currentfps;
  }
}


void FrameRateSleep(int max_fps)
{//http://www.geisswerks.com/ryan/FAQS/timing.html https://github.com/geissomatik
    LARGE_INTEGER t;
    QueryPerformanceCounter(&t);

    if (m_prev_end_of_frame.QuadPart != 0) 
    {

      int ticks_to_wait = (int) (m_high_perf_timer_freq.QuadPart / max_fps);
      bool done = FALSE;
      do
      {
        QueryPerformanceCounter(&t);
        
        int ticks_passed = (int)(t.QuadPart - m_prev_end_of_frame.QuadPart);
        int ticks_left = ticks_to_wait - ticks_passed;

        if (t.QuadPart < m_prev_end_of_frame.QuadPart)    // time wrap
          done = TRUE;
        if (ticks_passed >= ticks_to_wait)
          done = TRUE;
       
        if (!done)
        {
            // if > 0.002s left, do Sleep(1), which will actually sleep some 
            //   steady amount, probably 1-2 ms,
            //   and do so in a nice way (cpu meter drops; laptop battery spared).
            // otherwise, do a few Sleep(0)'s, which just give up the timeslice,
            //   but don't really save cpu or battery, but do pass a tiny
            //   amount of time.
          if (ticks_left > (int) (m_high_perf_timer_freq.QuadPart*2/1000))
            Sleep(1);
          else                        
            for (int i=0; i<10; i++) 
              Sleep(0);  // causes thread to give up its timeslice
        }
      } while (!done);            
    }
    m_prev_end_of_frame = t;
}


//gif testing
ULONG_PTR gdiplusToken;
GpImage *image;
UINT frameCount;
UINT currentFrame = 30;
int currentFrameTimer=0;
UINT *frameDelays;

void OnPaint(HDC hdc) {
    GpGraphics *graphics;
    GdipCreateFromHDC(hdc, &graphics);
    GdipDrawImage(graphics, image, 0, 0);
    GdipDeleteGraphics(graphics);
}


void UpdateFrame(HWND hwnd) 
{
    //currentFrame = (currentFrame + 1) % frameCount;
    
    if (current_song_time!=-1) {
      if(currentFrameTimer<=0) {
        currentFrameTimer=frameDelays[currentFrame];
        currentFrame++;
      } else {
        currentFrameTimer-=FPS;
      } 


      if (currentFrame>frameCount) {
        currentFrame=30;
      }

      GUID pageGuid = FrameDimensionTime;
      GdipImageSelectActiveFrame(image, &pageGuid, currentFrame);
    }
}


void RemoveFolderRecursive(const wchar_t* dirname)
{
  _WDIR *d;
  struct _wdirent *dir;
  d = _wopendir(dirname);
  if (d) {
    while ((dir=_wreaddir(d))!=NULL) {
      wchar_t indir[256];
      swprintf(indir,256,L"%s/%s",dirname,dir->d_name);
      if (PathIsDirectory(indir) && wcscmp(dir->d_name,L".")!=0 && wcscmp(dir->d_name,L"..")!=0) { //folder, check for 
        RemoveFolderRecursive(indir);
      } else {
        _wremove(indir);
      }
    }
    _wrmdir(dirname);
  }
}



LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  HDC hdc, hdcBackbuff;//, hdcBackbuff2;
  //HWND hShellWnd = FindWindowA("Shell_TrayWnd", NULL);
  //LONG originalStyle = GetWindowLong(hwnd, GWL_STYLE);

  //wchar_t le_msg[32];
  switch(msg) {


    //Mouse Movement
    case  WM_MOUSEMOVE: //https://stackoverflow.com/questions/22039413/moving-the-mouse-blocks-wm-timer-and-wm-paint
      if (!IsIconic(hwnd)) //no action when minimized
      {
        UpdateWindow(hwnd);
      }
      break;


    //Constantly Update Screen
    case WM_ERASEBKGND:
      if (!IsIconic(hwnd)) //no action when minimized
        InvalidateRect(hwnd,NULL,TRUE);
      return TRUE;
      break;



    //Graphics DrawIt()
    case WM_PAINT: //https://cplusplus.com/forum/beginner/269434/
      FrameRateSleep(FPS); // (Uncapped) //35 or 60 fps Credit: ayevdood/sharoyveduchi && y4my4m - move it here
      if (!IsIconic(hwnd)) //no action when minimized, prevents crash https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-isiconic?redirectedfrom=MSDN
      {
        showoff++;
        if (showoff>FPS)
          showoff=0;
        UpdateFrame(hwnd);
        SetUnhandledExceptionFilter((LPTOP_LEVEL_EXCEPTION_FILTER)&myTry); 
        PAINTSTRUCT ps;
        hdc=BeginPaint(hwnd, &ps);
        RECT rect;
        if(GetWindowRect(hwnd, &rect))
        {
          GR_WIDTH = rect.right - rect.left;
          GR_HEIGHT = rect.bottom - rect.top;        
          windowx = rect.left;
          windowy = rect.top;
        }

        if (GR_WIDTH!=OLD_GR_WIDTH || GR_HEIGHT!=OLD_GR_HEIGHT) {
          OLD_GR_WIDTH = GR_WIDTH;
          OLD_GR_HEIGHT = GR_HEIGHT;
        }

        hdcBackbuff=CreateCompatibleDC(hdc);
        HBITMAP screen=CreateCompatibleBitmap(hdc,GR_WIDTH,GR_HEIGHT);
        SelectObject(hdcBackbuff,screen);
  
        OnPaint(hdcBackbuff);
        DrawMainMenu(hdcBackbuff);

        BitBlt(hdc, 0, 0, GR_WIDTH, GR_HEIGHT, hdcBackbuff, 0, 0,  SRCCOPY);

        DeleteDC(hdcBackbuff);
        DeleteObject(screen);
        
        EndPaint(hwnd, &ps);
      }
      return 0;
      break;

    //Tasks to perform on start
    case WM_CREATE:
    {
      RESOLUTION_X[2]=SCREEN_WIDTH;
      RESOLUTION_Y[2]=SCREEN_HEIGHT;
      swprintf(src_music_dir,64,L"music");



      //waveOutGetVolume(hWaveOut[2],&wav_out_original_volume);

      //Delete tmp in music
      remove("music/tmp/tmp.wav");
      rmdir("music/tmp"); //remove tmp

      InitTickFrequency();
      InitFPS();
      //unsigned long long timenow=current_timestamp();
      //printf("\nSeconds Passed Since Jan-1-1970: %llu",timenow);
      stop_playing_song=FALSE;



     //Load Song
      InitSongBank();
     //

      //ShowCursor(FALSE);

       return 0;
    }
       break;


    //Tasks to perform on exit
    case WM_DESTROY:
      KillTimer(hwnd, 1);
      GdipDisposeImage(image);
      GdiplusShutdown(gdiplusToken);

      remove("music/tmp/tmp.wav");
      rmdir("music/tmp"); //remove tmp
      PostQuitMessage(0);
      return 0;
      break;


    //default:
     //break;
  }
  return DefWindowProcW(hwnd, msg, wParam, lParam);
}






int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int nCmdShow)
{
    GdiplusStartupInput gdiplusStartupInput;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    GdipLoadImageFromFile(L"sprites/bad_apple_gif.gif", &image);

    // Get the frame count
    GUID pageGuid = FrameDimensionTime;
    GdipImageGetFrameCount(image, &pageGuid, &frameCount);

    // Get the frame delays
    UINT size;
    GdipGetPropertyItemSize(image, PropertyTagFrameDelay, &size);
    PropertyItem *propertyItem = (PropertyItem*)malloc(size);
    GdipGetPropertyItem(image, PropertyTagFrameDelay, size, propertyItem);
    frameDelays = (UINT*)propertyItem->value;




  //Window Class
  WNDCLASSW wc = {0};
  wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC | CS_DBLCLKS; 
  wc.lpszClassName = L"DrawIt";
  wc.hInstance     = hInstance;
  wc.hbrBackground = GetSysColorBrush(COLOR_3DFACE);
  wc.lpfnWndProc   = WndProc;
  wc.hCursor       = LoadCursor(0, IDC_ARROW);
  wc.hIcon = LoadIcon(hInstance, L"id");

  RegisterClassW(&wc);


  //https://cplusplus.com/forum/beginner/9908/
  //create window
  int small_screen_x=640;
  int small_screen_y=480;
  HWND hwnd = CreateWindowW(wc.lpszClassName,
                L"Bad Apple!",
                WS_POPUP | WS_BORDER | WS_OVERLAPPEDWINDOW | WS_VISIBLE | CW_USEDEFAULT| CW_USEDEFAULT,
                SCREEN_WIDTH/2-small_screen_x/2,
                SCREEN_HEIGHT/2-small_screen_y/2,
                small_screen_x,
                small_screen_y,
                NULL,
                NULL,
                hInstance, 
                NULL);
  //HMONITOR hmon = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
  //SetWindowLong(hwnd, GWL_STYLE, 0);
  //ShowWindow(hwnd, SW_SHOW);
  //https://www.codeproject.com/Questions/441008/Hide-TaskBar-in-C-through-Win32
  //Make game un fast when level is run (simulates focusing tab)
  //https://batchloaf.wordpress.com/2012/10/18/simulating-a-ctrl-v-keystroke-in-win32-c-or-c-using-sendinput/

  Sleep(100);
  INPUT ip;
  ip.type = INPUT_KEYBOARD;
  ip.ki.wScan = 0;
  ip.ki.time = 0;
  ip.ki.dwExtraInfo = 0;
 
  for (int i=0;i<2;i++) {
  // Press the "Ctrl" key
    ip.ki.wVk = VK_CONTROL;
    ip.ki.dwFlags = 0; // 0 for key press
    SendInput(1, &ip, sizeof(INPUT));

    // Press the ESCAPE key
    ip.ki.wVk = VK_ESCAPE;
    ip.ki.dwFlags = 0; // 0 for key press
    SendInput(1, &ip, sizeof(INPUT));

    // Release the escape key
    ip.ki.wVk = VK_ESCAPE;
    ip.ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(1, &ip, sizeof(INPUT));

    // Release the "Ctrl" key
    ip.ki.wVk = VK_CONTROL;
    ip.ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(1, &ip, sizeof(INPUT));

    Sleep(100);
  }

  SetForegroundWindow(hwnd);
  HANDLE thread3=CreateThread(NULL,0,SongTask,NULL,0,NULL); //Spawn Song Player Thread

  //SetTimer(hwnd, 1, frameDelays[currentFrame] , NULL);

  MSG msg;
  while (true) {
    if (PeekMessage(&msg,NULL,0,0,PM_REMOVE)) {
      if (msg.message == WM_QUIT) break;

      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
  }
  waveOutSetVolume(hWaveOut[2],wav_out_original_volume);
  free(propertyItem);
  return (int) msg.wParam;
}




