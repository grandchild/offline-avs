// SE - 19/10/2012: this is a monolithic pile of shit code
// do not take as a good example of anything, except maybe
// how you can make something useful very quickly with
// ~200 lines of code...

#define STB_IMAGE_WRITE_IMPLEMENTATION
#define MY_WM_MESSAGE ( WM_APP + 1 )

#include "stb_image_write.h"

#include <windows.h>
#include <iostream>

using namespace std;

class C_RBASE
{
public:
	C_RBASE() { }
	virtual ~C_RBASE() { };
	virtual int render(char visdata[2][2][576], int isBeat, int *framebuffer, int *fbout, int w, int h)=0; // returns 1 if fbout has dest, 0 if framebuffer has dest
	virtual HWND conf(HINSTANCE hInstance, HWND hwndParent){return 0;};
	virtual char *get_desc()=0;
	virtual void load_config(unsigned char *data, int len) { }
	virtual int  save_config(unsigned char *data) { return 0; }
};


class C_RBASE2 : public C_RBASE
{
public:
	C_RBASE2() { }
	virtual ~C_RBASE2() { };

    int getRenderVer2() { return 2; }


    virtual int smp_getflags() { return 0; } // return 1 to enable smp support
    virtual int smp_begin(int max_threads, char visdata[2][2][576], int isBeat, int *framebuffer, int *fbout, int w, int h) { return 0; }  
    virtual void smp_render(int this_thread, int max_threads, char visdata[2][2][576], int isBeat, int *framebuffer, int *fbout, int w, int h) { }; 
    virtual int smp_finish(char visdata[2][2][576], int isBeat, int *framebuffer, int *fbout, int w, int h) { return 0; }; // return value is that of render() for fbstuff etc

};

// lovely helper functions for blending
static unsigned int __inline BLEND(unsigned int a, unsigned int b)
{
	register unsigned int r,t;
	r=(a&255)+((b)&255);
	t=min(r,255);
	r=((a>>8)&255)+((b>>8)&255);
	t|=min(r,255)<<8;
	r=((a>>16)&255)+((b>>16)&255);
	return t|min(r,255)<<16;
}

static unsigned int __inline BLEND_AVG(unsigned int a, unsigned int b)
{
	return ((a>>1)&~((1<<7)|(1<<15)|(1<<23)))+((b>>1)&~((1<<7)|(1<<15)|(1<<23)));
}

typedef void *VM_CONTEXT;
typedef void *VM_CODEHANDLE;
typedef struct
{
  int ver; // ver=1 to start
  double *global_registers; // 100 of these
  int *lineblendmode; 
  //evallib interface
  VM_CONTEXT (*allocVM)(); // return a handle
  void (*freeVM)(VM_CONTEXT); // free when done with a VM and ALL of its code have been freed, as well
  void (*resetVM)(VM_CONTEXT);
  double * (*regVMvariable)(VM_CONTEXT, char *name);
  VM_CODEHANDLE (*compileVMcode)(VM_CONTEXT, char *code);
  void (*executeCode)(VM_CODEHANDLE, char visdata[2][2][576]);
  void (*freeCode)(VM_CODEHANDLE);
  void (*doscripthelp)(HWND hwndDlg,char *mytext); // mytext can be NULL for no custom page
  void *(*getNbuffer)(int w, int h, int n, int do_alloc); // do_alloc should be 0 if you dont want it to allocate if empty
                                                          // w and h should be the current width and height
                                                          // n should be 0-7

} APEinfo;


#define MOD_NAME "PNG / Save Every Frame"
#define UNIQUEIDSTRING "FuckYourSelfInTheNoseBitch"

// extended APE api support
APEinfo *g_extinfo;
extern "C"
{
  void __declspec(dllexport) _AVS_APE_SetExtInfo(HINSTANCE hDllInstance, APEinfo *ptr)
  {
    g_extinfo = ptr;
  }
}

class C_THISCLASS : public C_RBASE 
{
protected:
public:
	C_THISCLASS() {}
	virtual ~C_THISCLASS() {}

	virtual int render(char visdata[2][2][576], int isBeat, int *framebuffer, int *fbout, int w, int h);

	virtual HWND conf(HINSTANCE hInstance, HWND hwndParent);
	virtual char *get_desc();

	virtual void load_config(unsigned char *data, int len);
	virtual int  save_config(unsigned char *data);
};

// global configuration dialog pointer 
static C_THISCLASS *g_ConfigThis; 
// global DLL instance pointer (not needed in this example, but could be useful)
static HINSTANCE g_hDllInstance; 



static BOOL CALLBACK g_DlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam,LPARAM lParam)
{
	return uMsg == WM_INITDIALOG ? 1 : 0 ;
}

int C_THISCLASS::render(char visdata[2][2][576], int isBeat, int *framebuffer, int *fbout, int w, int h)
{
	static int i = 0;
	static char outPath[ 256 ];
	sprintf( outPath, "image%05d.png", i );

	const int limit = w * h;
	for( int i = 0; i < limit; ++i )
	{
		fbout[ i ] = 0xFF000000 +
							( framebuffer[ i ] & 0x0000FF00 ) +
							( ( framebuffer[ i ]<<16 ) & 0x00FF0000 ) +
							( ( framebuffer[ i ]>>16 ) & 0x000000FF );
	}
	
	HWND recieverHwnd = FindWindow( L"Dummy", NULL );
	if ( recieverHwnd != NULL )
	{
		TCHAR szBuf[80];
		//GetWindowText( recieverHwnd, szBuf, 80 );
		SendMessage( recieverHwnd, 12345, 80, (LPARAM)szBuf );

		cout << "=== SAVING FRAME " << i << " ===\n";

		stbi_write_png( outPath, w, h, 4, fbout, 4 * w ); 
		++i;
	}
	
	return 0;
}


HWND C_THISCLASS::conf(HINSTANCE hInstance, HWND hwndParent) // return NULL if no config dialog possible
{
	g_ConfigThis = this;
	return NULL;
}


char *C_THISCLASS::get_desc(void)
{ 
	return MOD_NAME; 
}


// load_/save_config are called when saving and loading presets (.avs files)
#define GET_INT() (data[pos]|(data[pos+1]<<8)|(data[pos+2]<<16)|(data[pos+3]<<24))
void C_THISCLASS::load_config(unsigned char *data, int len) // read configuration of max length "len" from data.
{
}


// write configuration to data, return length. config data should not exceed 64k.
#define PUT_INT(y) data[pos]=(y)&255; data[pos+1]=(y>>8)&255; data[pos+2]=(y>>16)&255; data[pos+3]=(y>>24)&255
int  C_THISCLASS::save_config(unsigned char *data) 
{
	return 0;
}

// export stuff
C_RBASE *R_RetrFunc(char *desc) // creates a new effect object if desc is NULL, otherwise fills in desc with description
{
	if (desc) 
	{ 
		strcpy(desc,MOD_NAME); 
		return NULL; 
	}
	return (C_RBASE *) new C_THISCLASS();
}

extern "C"
{
	__declspec (dllexport) int _AVS_APE_RetrFunc(HINSTANCE hDllInstance, char **info, int *create) // return 0 on failure
	{
		g_hDllInstance=hDllInstance;
		*info=UNIQUEIDSTRING;
		*create=(int)(void*)R_RetrFunc;
		return 1;
	}
};
