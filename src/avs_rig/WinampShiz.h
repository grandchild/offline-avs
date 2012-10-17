#ifndef WINAMP_SHIZ
#define WINAMP_SHIZ

#define IPC_SETVISWND 611 // param is hwnd, setting this allows you to receive ID_VIS_NEXT/PREVOUS/RANDOM/FS wm_commands
#define ID_VIS_NEXT                     40382
#define ID_VIS_PREV                     40383
#define ID_VIS_RANDOM                   40384
#define ID_VIS_FS                       40389
#define ID_VIS_CFG                      40390
#define ID_VIS_MENU                     40391

#define IPC_GET_EMBEDIF 505 // pass an embedWindowState
// returns an HWND embedWindow(embedWindowState *); if the data is NULL, otherwise returns the HWND directly
typedef struct
{
  HWND me; //hwnd of the window

  int flags;

  RECT r;

  void *user_ptr; // for application use

  int extra_data[64]; // for internal winamp use
} embedWindowState;

typedef struct winampVisModule {
  char *description; // description of module
  HWND hwndParent;   // parent window (filled in by calling app)
  HINSTANCE hDllInstance; // instance handle to this DLL (filled in by calling app)
  int sRate;		 // sample rate (filled in by calling app)
  int nCh;			 // number of channels (filled in...)
  int latencyMs;     // latency from call of RenderFrame to actual drawing
                     // (calling app looks at this value when getting data)
  int delayMs;       // delay between calls in ms

  // the data is filled in according to the respective Nch entry
  int spectrumNch;
  int waveformNch;
  unsigned char spectrumData[2][576];
  unsigned char waveformData[2][576];

  void (*Config)(struct winampVisModule *this_mod);  // configuration dialog
  int (*Init)(struct winampVisModule *this_mod);     // 0 on success, creates window, etc
  int (*Render)(struct winampVisModule *this_mod);   // returns 0 if successful, 1 if vis should end
  void (*Quit)(struct winampVisModule *this_mod);    // call when done

  void *userData; // user data, optional
} winampVisModule;

typedef struct {
  int version;       // VID_HDRVER
  char *description; // description of library
  winampVisModule* (*getModule)(int);
} winampVisHeader;

#endif
