/* Copyright (c) 2011 Alastair Channon
* 
* Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
* 
* The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
* 
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

/* ann_xwin.h */
#ifndef ann_xwin_h
#define ann_xwin_h
#include "ann_core.h"
#include <X11/Xlib.h>

extern int number_of_ann_windows_open;
const int max_num_ann_windows = 50;

void process_ann_xwin_events(XEvent &X_event);

enum excitatory_bool {inhibitory=0,excitatory=1};

class Ann_Window
{ friend void X_draw_text_with_Ann_Standard_pen(const char *text);
  friend void X_newline_on_Ann_Standard_pen();
  public:    Ann_Window(Ann *_ann,Display *_X_display,Window *X_parent_window);
    virtual ~Ann_Window();
             unsigned int X_width;
             unsigned int X_height;
             void draw_Ann();
             void ButtonDown(unsigned int x,unsigned int y);
             void MouseMove(unsigned int x,unsigned int y);
             void QueryMouseMove();
             void ButtonUp();
             Window *X_window;
             int X_has_been_exposed;
             void set_X_dashes();
  protected: Ann *ann;
             int num_nodes;
             Node **nodes;
             XPoint *node_points;
             char **node_chars;
             XPoint grid_to_window(XPoint point);
             XPoint window_to_grid(unsigned int x,unsigned int y);
             void draw_link(XPoint from,XPoint to,excitatory_bool excitatory);
             void draw_link_to_node(XPoint from,Node*node,
                                    excitatory_bool excitatoryYN);
             Display *X_display;
             GC X_Ann_Standard_pen;
             XFontStruct *X_Ann_Standard_pen_fontstruct;
             unsigned long X_Ann_Standard_pen_font_height;
             XPoint X_Ann_Standard_pen_at;
             GC X_Ann_Inhibitory_pen;
             GC X_Ann_Excitatory_pen;
};

#endif
