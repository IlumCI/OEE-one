/* Copyright (c) 2001 Alastair Channon
* 
* Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
* 
* The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
* 
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

/* ann_win.h */
#ifndef ann_win_h
#define ann_win_h
#include <owl/framewin.h>
#include "ann_core.h"

extern int number_of_ann_windows_open;
const int max_num_ann_windows = 50;

class Cursor : public TDC
{ public:    Cursor() : TDC() {init_pens();}
             Cursor(HDC hdc) : TDC(hdc) {init_pens();}
            ~Cursor() {delete Inhibitory_Pen, Excitatory_Pen;}
             TPoint at; //used so metafile devices cann tell where they are
             bool LineTo(int x, int y);
             bool LineTo(const TPoint& point);
             bool MoveTo(int x, int y);
             bool MoveTo(const TPoint& point);
             void Inhibitory_LineTo(TPoint &point);
             void Excitatory_LineTo(TPoint &point);
  protected: void Draw_Link_To(TPoint &point,int excitatory);
             TPen *Inhibitory_Pen;
             TPen *Excitatory_Pen;
             void init_pens();
};

class Ann_Window : public TFrameWindow
{ public:	 Ann_Window(Ann *_ann,TWindow *parent);
            ~Ann_Window();
  protected: Ann *ann;
             int num_nodes;
             Node **nodes;
             TPoint *node_points;
             char **node_chars;
             TPoint grid_to_window(TPoint &point);
             TPoint window_to_grid(TPoint &point);
             void draw_inhibitory_to_node_using_cursor(Node*node,Cursor*cursor);
             void draw_excitatory_to_node_using_cursor(Node*node,Cursor*cursor);
             void draw_Ann_using_cursor(Cursor *cursor);
             void EvLButtonDown(uint,TPoint &point); //Message response function
             void EvMouseMove(uint,TPoint &point);   //Message response function
             void EvLButtonUp(uint,TPoint &point);   //Message response function
//             void EvRButtonDown(uint,TPoint &);      //Message response function
             void EvSize(UINT sizeType,TSize &size)  //Message response function
               {Invalidate(); TFrameWindow::EvSize(sizeType, size);}
             void Paint(TDC &cursor,BOOL,TRect &)             //A message resp.
               {draw_Ann_using_cursor(&Cursor(HDC(cursor)));} // fn. calls Paint
  DECLARE_RESPONSE_TABLE(Ann_Window);
};


class Ann_Window_ostream : public ostream
{ public:    Ann_Window_ostream() : ostream() {}
             Cursor *cursor;
             char string[17];
             void operator << (int &i);
             void operator << (char *chars);
             void endl();
};


#endif
