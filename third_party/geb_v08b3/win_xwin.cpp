/* Copyright (c) 2011 Alastair Channon
* 
* Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
* 
* The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
* 
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

/* win_xwin.cpp */
#include "win_xwin.h"
#include <X11/Xutil.h>
#include <stdio.h>
#include <string.h>

char *display;
char chararray_empty[] = "";

int X_create_window(
    Display *&X_display,Window *X_parent_window,Window *&X_window,GC &X_gc,
    const char *window_name, const char *icon_name,
    int X_x,int X_y,int X_width,int X_height,
    long X_event_masks,
    int argc, const char *argv1, char *argv2)
{ if (!X_display)
  { if ((argc>=2) && !strcmp(argv1,"-display")) display = argv2;
    else display = chararray_empty; // using chararray_empty rather than "" prevents the warning: deprecated conversion from string constant to ‘char*’
    if (!display)
    { //fprintf(stdout,"\nRunning without a display\n"); fflush(stdout);
      display = 0;
      return 1;
    }
    X_display = XOpenDisplay(display);
    if (!X_display)
    { fprintf(stderr,"\n%s%s","Could not open display ",display);
      fprintf(stderr,"\nRunning without a display\n"); fflush(stderr);
      display = 0;
      return 2;
    }
  }
  int X_screen = DefaultScreen(X_display);
  unsigned long X_foreground,X_background;
  X_foreground = BlackPixel(X_display,X_screen);
  X_background = WhitePixel(X_display,X_screen);
  //
  if (!X_window) X_window = new Window;
  *X_window = XCreateSimpleWindow(X_display,X_parent_window?*X_parent_window:DefaultRootWindow(X_display),
                  X_x,X_y,X_width,X_height,
                  5, X_foreground, X_background);
  if (!X_window) {fprintf(stderr,"Could not open window\n"); fflush(stderr); return 3;}
  //
  XStoreName(X_display,*X_window,window_name);
  XSetIconName(X_display,*X_window,icon_name);
  //
  X_gc = XCreateGC(X_display,*X_window,0,0);
  XSetForeground(X_display,X_gc,X_foreground);
  XSetBackground(X_display,X_gc,X_background);
  //
  XSelectInput(X_display,*X_window,X_event_masks);
  XMapRaised(X_display,*X_window);
  return 0;
}
