#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "stb_image_write.h"

#include <windows.h>

// AVS APE (Plug-in Effect) header

// base class to derive from
class C_RBASE {
	public:
		C_RBASE() { }
		virtual ~C_RBASE() { };
		virtual int render(char visdata[2][2][576], int isBeat, int *framebuffer, int *fbout, int w, int h)=0; // returns 1 if fbout has dest, 0 if framebuffer has dest
		virtual HWND conf(HINSTANCE hInstance, HWND hwndParent){return 0;};
		virtual char *get_desc()=0;
		virtual void load_config(unsigned char *data, int len) { }
		virtual int  save_config(unsigned char *data) { return 0; }
};

// if you want to support SMP rendering, you can derive from this class instead, 
// and expose the creator via: _AVS_APE_RetrFuncEXT2
// (in general, exposing both for compatibility is a good idea)
class C_RBASE2 : public C_RBASE {
	public:
		C_RBASE2() { }
		virtual ~C_RBASE2() { };

    int getRenderVer2() { return 2; }


    virtual int smp_getflags() { return 0; } // return 1 to enable smp support

    // returns # of threads you desire, <= max_threads, or 0 to not do anything
    // default should return max_threads if you are flexible
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

//extended APE stuff

// to use this, you should have:
// APEinfo *g_extinfo;
// void __declspec(dllexport) AVS_APE_SetExtInfo(HINSTANCE hDllInstance, APEinfo *ptr)
// {
//    g_extinfo = ptr;
// }

typedef void *VM_CONTEXT;
typedef void *VM_CODEHANDLE;
typedef struct
{
  int ver; // ver=1 to start
  double *global_registers; // 100 of these

  // lineblendmode: 0xbbccdd
  // bb is line width (minimum 1)
  // dd is blend mode:
     // 0=replace
     // 1=add
     // 2=max
     // 3=avg
     // 4=subtractive (1-2)
     // 5=subtractive (2-1)
     // 6=multiplicative
     // 7=adjustable (cc=blend ratio)
     // 8=xor
     // 9=minimum
  int *lineblendmode; 

  //evallib interface
  VM_CONTEXT (*allocVM)(); // return a handle
  void (*freeVM)(VM_CONTEXT); // free when done with a VM and ALL of its code have been freed, as well

  // you should only use these when no code handles are around (i.e. it's okay to use these before
  // compiling code, or right before you are going to recompile your code. 
  void (*resetVM)(VM_CONTEXT);
  double * (*regVMvariable)(VM_CONTEXT, char *name);

  // compile code to a handle
  VM_CODEHANDLE (*compileVMcode)(VM_CONTEXT, char *code);

  // execute code from a handle
  void (*executeCode)(VM_CODEHANDLE, char visdata[2][2][576]);

  // free a code block
  void (*freeCode)(VM_CODEHANDLE);

  // requires ver >= 2
  void (*doscripthelp)(HWND hwndDlg,char *mytext); // mytext can be NULL for no custom page

  /// requires ver >= 3
  void *(*getNbuffer)(int w, int h, int n, int do_alloc); // do_alloc should be 0 if you dont want it to allocate if empty
                                                          // w and h should be the current width and height
                                                          // n should be 0-7

} APEinfo;


/**
	Example Winamp AVS plug-in
	Copyright (c) 2000, Nullsoft Inc.
	
	Hello, welcome to the first Advanced Visualization 
	Studio tutorial!
		The hope is that together, we can learn to utilize 
	AVS's powerful features: Namely direct access to the 
	frame buffer, EZ beat detection, and the ability to 
	stack plug-ins written by other developers for an 
	infinite array of possible effects.
	
	I hereby present: 
	Tutorial 0: BOX-
		Simplicity at its finest. Displays a rectangle on 
	screen on every beat. Oh, and you can change its color 
	too... Check avstut00.avs for a demonstration of a 
	spinning rectangle's power!

	good luck and have fun!
**/

#include <windows.h>
//#include "resource.h"
//#include "avs_ape.h"


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
		C_THISCLASS();
		virtual ~C_THISCLASS();

		virtual int render(char visdata[2][2][576], int isBeat, int *framebuffer, int *fbout, int w, int h);

		virtual HWND conf(HINSTANCE hInstance, HWND hwndParent);
		virtual char *get_desc();

		virtual void load_config(unsigned char *data, int len);
		virtual int  save_config(unsigned char *data);

		int enabled;	// toggles plug-in on and off
		int color;		// color of rectangle
};

// global configuration dialog pointer 
static C_THISCLASS *g_ConfigThis; 
// global DLL instance pointer (not needed in this example, but could be useful)
static HINSTANCE g_hDllInstance; 



// this is where we deal with the configuration screen
static BOOL CALLBACK g_DlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam,LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_INITDIALOG:


			return 1;

		case WM_DRAWITEM:


			return 0;

		case WM_COMMAND:


		  return 0;
	}
	return 0;
}




// set up default configuration 
C_THISCLASS::C_THISCLASS() 
{
	//set initial color
	color=RGB(255,0,0);
	enabled=1;
}

// virtual destructor
C_THISCLASS::~C_THISCLASS() 
{
}


// RENDER FUNCTION:
// render should return 0 if it only used framebuffer, or 1 if the new output data is in fbout. this is
// used when you want to do something that you'd otherwise need to make a copy of the framebuffer.
// w and h are the-*/ width and height of the screen, in pixels.
// isBeat is 1 if a beat has been detected.
// visdata is in the format of [spectrum:0,wave:1][channel][band].

int C_THISCLASS::render(char visdata[2][2][576], int isBeat, int *framebuffer, int *fbout, int w, int h)
{
	static int i = 0;
	static char outPath[ 256 ];
	sprintf( outPath, "image%05d.png", i );

	const int limit = w * h;
	for( int i = 0; i < limit; ++i )
	{
		framebuffer[ i ] |= 0xFF000000;
	}
	stbi_write_png( outPath, w, h, 4, framebuffer, 4 * w ); 
	++i;
	return 0;
}


HWND C_THISCLASS::conf(HINSTANCE hInstance, HWND hwndParent) // return NULL if no config dialog possible
{
	g_ConfigThis = this;
	return NULL;//return CreateDialog(hInstance,MAKEINTRESOURCE(IDD_CONFIG),hwndParent,g_DlgProc);
}


char *C_THISCLASS::get_desc(void)
{ 
	return MOD_NAME; 
}


// load_/save_config are called when saving and loading presets (.avs files)

#define GET_INT() (data[pos]|(data[pos+1]<<8)|(data[pos+2]<<16)|(data[pos+3]<<24))
void C_THISCLASS::load_config(unsigned char *data, int len) // read configuration of max length "len" from data.
{
	int pos=0;

	// always ensure there is data to be loaded
	if (len-pos >= 4) 
	{
		// load activation toggle
		enabled=GET_INT();
		pos+=4; 
	}

	if (len-pos >= 4) 
    { 
		// load the box color
		color=GET_INT(); 
		pos+=4; 
	}
}


// write configuration to data, return length. config data should not exceed 64k.
#define PUT_INT(y) data[pos]=(y)&255; data[pos+1]=(y>>8)&255; data[pos+2]=(y>>16)&255; data[pos+3]=(y>>24)&255
int  C_THISCLASS::save_config(unsigned char *data) 
{
	int pos=0;

	PUT_INT(enabled); 
	pos+=4;

	PUT_INT(color); 
	pos+=4;

	return pos;
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


/**
	Final Thoughts:
		Alright! Hopefully you guys can take the next step 
	and display more than just a colored rectangle ;) The 
	exciting thing is, each time you write an AVS plug-in, 
	you exponentially increase AVS's potential, unlocking
	the possibility of an effect you never expected. Good 
	luck, I hope this has helped!
	
	See you next time!
**/