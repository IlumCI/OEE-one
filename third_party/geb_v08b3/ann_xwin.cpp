/* Copyright (c) 2011 Alastair Channon
* 
* Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
* 
* The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
* 
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

/* ann_xwin.cpp */
#include "ann_xwin.h"
#include "win_xwin.h"
#include <stdio.h>
#include <string.h>
#include <X11/Xatom.h>

const int max_character_length = 13; //for "long part_character" - can be extended using "long long"&"%Ld", but that's non-portable & non-essential

int max(int x,int y) {return x>y?x:y;}
int abs(int x) {return x>0?x:-x;}

int number_of_ann_windows_open;
int selected_node_num;

int grid_fineness=60; //a window co-ordinate snaps to 1 of grid_fineness on grid

Ann_Window *ann_window_array[max_num_ann_windows];
Ann_Window *ann_window_for_text_output;
const char *default_font = "8x13";
const int default_font_width = 8;
const int default_font_height = 13;
int ButtonDown_x,ButtonDown_y;
const int X_line_width = 0;            //0 is fastest, but:
const int X_inhibitory_line_width = 1; //note some systems need width >0 for dashed lines

void process_ann_xwin_events(XEvent &X_event)
{ int ann_window_num;
  Ann_Window *ann_window;
  for (ann_window_num=0,ann_window=ann_window_array[0];
       (ann_window_num<number_of_ann_windows_open) &&
       (*(ann_window->X_window) != X_event.xany.window);
       ann_window_num++,ann_window=ann_window_array[ann_window_num]);
  if (ann_window_num == number_of_ann_windows_open) return; //error
  switch(X_event.type)
  { case Expose:
      if (X_event.xexpose.count == 0)
      { ann_window->draw_Ann();
        ann_window->X_has_been_exposed=1;
      }
      break;
    case ConfigureNotify:
      ann_window->X_width  = X_event.xconfigure.width;
      ann_window->X_height = X_event.xconfigure.height;
      ann_window->set_X_dashes();
      if (ann_window->X_has_been_exposed)
      { XClearWindow(X_event.xany.display,*(ann_window->X_window));
        ann_window->draw_Ann();
      }
      break;
    case MappingNotify:
      XRefreshKeyboardMapping(&(X_event.xmapping));
      break;
    case ButtonPress:
      if (X_event.xbutton.button != 1)
      { delete ann_window;
        break;
      }
      ButtonDown_x = X_event.xbutton.x;
      ButtonDown_y = X_event.xbutton.y;
      ann_window->ButtonDown(ButtonDown_x,ButtonDown_y);
      break;
    case MotionNotify:
      ann_window->QueryMouseMove();
      break;
    case ButtonRelease:
      ann_window->ButtonUp();
      break;
    case LeaveNotify:
      ann_window->MouseMove(ButtonDown_x,ButtonDown_y); //reset if needed
      ann_window->ButtonUp();                           //unselect
      break;
  }
}

Ann_Window::Ann_Window(Ann *_ann,Display *_X_display,Window *X_parent_window)
{ X_display=_X_display;
  X_width=400; X_height=200;
  X_window=0;
  X_has_been_exposed=0;
  //int err=X_create_window(X_display,X_parent_window,X_window,
  int err=X_create_window(X_display,X_window,X_window,
                          X_Ann_Standard_pen,
                          "GEB: Ann Window","geb:ann",100,100,X_width,X_height,
                          ExposureMask|StructureNotifyMask|
                          PointerMotionHintMask|
                          ButtonPressMask|ButtonMotionMask|ButtonReleaseMask|
                          LeaveWindowMask,
                          2,"-display",0);//note would always have a display
  if (err)
  { fprintf(stderr,"Error %d when creating Ann window\n",err); fflush(stderr);
    return;
  }
  ann_window_array[number_of_ann_windows_open] = this;
  number_of_ann_windows_open++;
  //Get font structure of standard pen:
  X_Ann_Standard_pen_fontstruct =0;
  XGCValues temp_XGCValues;
  if (   XGetGCValues(X_display,X_Ann_Standard_pen,GCFont,&temp_XGCValues)
      && (((int)temp_XGCValues.font)!=-1))
      // above line put in to fix intermittent "Bad Font" error
    X_Ann_Standard_pen_fontstruct = XQueryFont(X_display,temp_XGCValues.font);
  if (   !X_Ann_Standard_pen_fontstruct
      || !XGetFontProperty(X_Ann_Standard_pen_fontstruct,
                           XA_CAP_HEIGHT,&X_Ann_Standard_pen_font_height)
     )
  { if (X_Ann_Standard_pen_fontstruct) XFreeFont(X_display,X_Ann_Standard_pen_fontstruct);
    //Having failed to get fontstruct and XA_CAP_HEIGHT from GC, set defaults:
    X_Ann_Standard_pen_fontstruct = XLoadQueryFont(X_display,default_font);
    if (X_Ann_Standard_pen_fontstruct)
    { XSetFont(X_display,X_Ann_Standard_pen,X_Ann_Standard_pen_fontstruct->fid);
      if (!XGetFontProperty(X_Ann_Standard_pen_fontstruct,
                            XA_CAP_HEIGHT,&X_Ann_Standard_pen_font_height))
        X_Ann_Standard_pen_font_height=default_font_height; //no height given
      //XFreeFont(X_display,X_Ann_Standard_pen_fontstruct);
      //Above line commented out because, despite claims that the font's
      //  resources would not be destroyed until after all references to it
      //  have gone, this is not true on some systems.  
    }
    else X_Ann_Standard_pen_font_height=default_font_height; //no default font
  }
  //Initialise inhibitory and excitatory pens:
  X_Ann_Inhibitory_pen = XCreateGC(X_display,*X_window,0,0);
  XSetLineAttributes(X_display,X_Ann_Inhibitory_pen,X_inhibitory_line_width,
    LineOnOffDash,CapRound,JoinRound); //may need width>0 for dashed
  set_X_dashes();
  XCopyGC(X_display,X_Ann_Standard_pen,GCForeground|GCBackground,
    X_Ann_Inhibitory_pen);
  Colormap X_cmap = DefaultColormap(X_display,DefaultScreen(X_display));
  XColor X_redF; X_redF.red=65535; X_redF.green=10000; X_redF.blue=10000;
  if (XAllocColor(X_display,X_cmap,&X_redF))
    XSetForeground(X_display,X_Ann_Inhibitory_pen,X_redF.pixel);
  X_Ann_Excitatory_pen = XCreateGC(X_display,*X_window,0,0);
  XSetLineAttributes(X_display,X_Ann_Excitatory_pen,X_line_width,
    LineSolid,CapRound,JoinRound);     //may need width>0 for dashed
  XCopyGC(X_display,X_Ann_Standard_pen,GCForeground|GCBackground,
    X_Ann_Excitatory_pen);
  //
  ann = _ann;
  Node *node;
  // Count the number of nodes (note: not including the dummy):
  for (num_nodes = 0,node=ann->nodes->next;node;node=node->next) num_nodes++;
  // Store the nodes (pointers to) and their display positions in arrays:
  nodes = new Node *[num_nodes];
  node_points = new XPoint[num_nodes];
  node_chars = new char *[num_nodes];
  //Give each node a sensible(ish) position in the window to start with:
  int node_num;
  int x_for_inputs,x_for_outputs,x_for_neither;
  x_for_inputs = x_for_outputs = x_for_neither = grid_fineness;
  for (node_num=0,node=ann->nodes->next; //not dummy
       node_num<num_nodes;
       node_num++,node=node->next)
  { nodes[node_num] = node;
    node_chars[node_num] = new char [50]; //should be enough
    if (node->net_input_node_ptr)       //if node is a net input node
    { int net_input_num=0;
      Node_ptr *node_ptr = ann->net_inputs->next; //not dummy
      for (;node_ptr->node!=node;net_input_num++,node_ptr=node_ptr->next);
      x_for_inputs  -= grid_fineness / (num_nodes+2);
      node_points[node_num].x = x_for_inputs;      // in grid co-ordinates
      node_points[node_num].y = grid_fineness*3/4; // in grid co-ordinates
      node_chars[node_num][0] = 'i';
      long part_character=1; //extra one at start (for leading zeros) - will be replaced by L or R
      int part_character_length=2; //i and (L or R)
      for (Gene *gene_ptr=node->character.first_gene + 3;
           gene_ptr<=node->character.last_gene;gene_ptr+=2)
      { part_character *= 10;
        part_character |= (int) *(gene_ptr);
        part_character_length++;
        if (part_character_length==(max_character_length-3)) break;
      }
      sprintf(node_chars[node_num]+1,"%ld",part_character);
      if (part_character_length==(max_character_length-3))
        sprintf(node_chars[node_num]+part_character_length,"...");
      if (*(node->character.first_gene +1) == 0)
        node_chars[node_num][1] = 'L';
      else
        node_chars[node_num][1] = 'R';
    }
    if (node->net_output_node_ptr) // if node is a net output node
    { int net_output_num=0;
      Node_ptr *node_ptr = ann->net_outputs->next; //not dummy
      for (;node_ptr->node!=node;net_output_num++,node_ptr=node_ptr->next);
      if (node->net_input_node_ptr)  // both
        sprintf(node_chars[node_num],"ERROR: net_input & net_output");
      else
      { x_for_outputs -= grid_fineness / (num_nodes+2);
        node_points[node_num].x = x_for_outputs;     // in grid co-ordinates
        node_points[node_num].y = grid_fineness*1/4; // in grid co-ordinates
        //long part_character=0;
        long part_character=1; //extra one at start (for leading zeros) - will be replaced by o
        int part_character_length=1; //o
        for (Gene *gene_ptr=node->character.first_gene + 1;
             gene_ptr<=node->character.last_gene;gene_ptr+=2)
        { //part_character <<= 1;
          part_character *= 10;
          part_character |= (int) *(gene_ptr);
          part_character_length++;
          if (part_character_length==(max_character_length-3)) break;
        }
        sprintf(node_chars[node_num],"%ld",part_character);
        if (part_character_length==(max_character_length-3))
          sprintf(node_chars[node_num]+part_character_length,"...");
        node_chars[node_num][0] = 'o';
      }
    }
    if ((!(node->net_input_node_ptr)) && (!(node->net_output_node_ptr))) //else (if neither)
    { x_for_neither -= grid_fineness / (num_nodes+2);
      node_points[node_num].x = x_for_neither;     // in grid co-ordinates
      node_points[node_num].y = grid_fineness*1/2; // in grid co-ordinates
      node_chars[node_num][0] = 0;
    }
  }
  selected_node_num = -1; //no node selected to be moved in window
}

Ann_Window::~Ann_Window()
{ delete [] nodes; //deletes the array of pointers, not the nodes themselves
  delete [] node_points;
  int node_num;
  for (node_num=0;node_num<num_nodes;node_num++) delete [] node_chars[node_num];
  delete [] node_chars;
  int ann_window_num;
  Ann_Window *ann_window;
  for (ann_window_num=0,ann_window=ann_window_array[0];
       (ann_window_num<number_of_ann_windows_open) &&
       (*(ann_window->X_window) != *X_window);
       ann_window_num++,ann_window=ann_window_array[ann_window_num]);
  number_of_ann_windows_open--;
  if (ann_window_num < number_of_ann_windows_open) //else error
    for (;ann_window_num<number_of_ann_windows_open;ann_window_num++)
      ann_window_array[ann_window_num] = ann_window_array[ann_window_num+1];
  if (X_Ann_Standard_pen_fontstruct) XFreeFont(X_display,X_Ann_Standard_pen_fontstruct);
  XFreeGC(X_display,X_Ann_Inhibitory_pen);
  XFreeGC(X_display,X_Ann_Excitatory_pen);
  XFreeGC(X_display,X_Ann_Standard_pen);
  XDestroyWindow(X_display,*X_window);
  delete X_window;
}

void Ann_Window::set_X_dashes()
{ int X_dashes_scale = (X_width + X_height) / 150; 
  char X_dashes [] = {(char)(X_dashes_scale*2),(char)X_dashes_scale};
  XSetDashes(X_display,X_Ann_Inhibitory_pen,0,X_dashes,2);
}

XPoint Ann_Window::grid_to_window(XPoint point)
{ XPoint result;
  result.x = (int)((  ((long)X_width)      *point.x)/grid_fineness);
  result.y = (int)(( (((long)X_height)-22) *point.y)/grid_fineness);
  return result;
}

XPoint Ann_Window::window_to_grid(unsigned int x,unsigned int y)
{ XPoint result;
  result.x = (int)(( ((long)x) *grid_fineness+ X_width     /2)/ X_width     );
  result.y = (int)(( ((long)y) *grid_fineness+(X_height-22)/2)/(X_height-22));
  return result;
}

void Ann_Window::draw_link(XPoint from,XPoint to,excitatory_bool excitatoryYN)
{ XPoint midpoint;
  GC *X_pen;
  if (excitatoryYN) X_pen = &X_Ann_Excitatory_pen;
  else X_pen = &X_Ann_Inhibitory_pen;
  if (excitatoryYN)
  { midpoint.x = (from.x+to.x)>>1;
    midpoint.y = (from.y+to.y)>>1;
  }
  //else inhibitory - want arrow at different to on line in case have both
  else
  { midpoint.x = (3*from.x+2*to.x)/5;
    midpoint.y = (3*from.y+2*to.y)/5;
  }
  // at midpoint, an arrow is drawn showing direction
  int x_diff = (to.x - from.x); int y_diff = (to.y - from.y);
  int normalise = max(16,max(abs(x_diff),abs(y_diff))) >>3; //pseudo-normalise
  x_diff /= normalise; y_diff /= normalise;
  //XPoint points[6] = {from,midpoint,midpoint,midpoint,midpoint,to};
  XPoint points[6] = {{from.x,from.y},{midpoint.x,midpoint.y},{midpoint.x,midpoint.y},{midpoint.x,midpoint.y},{midpoint.x,midpoint.y},{to.x,to.y}};
  points[2].x -= x_diff+(y_diff>>1); points[2].y -= y_diff-(x_diff>>1),
  points[3].x -= x_diff-(y_diff>>1); points[3].y -= y_diff+(x_diff>>1),
  XDrawLines(X_display,*X_window,*X_pen,points,6,CoordModeOrigin);
}

void Ann_Window::draw_link_to_node(XPoint from,Node *node,
                                   excitatory_bool excitatoryYN)
{ int node_num;
  for (node_num=0;(node_num<num_nodes)&&(nodes[node_num]!=node);node_num++);
  if (node_num != num_nodes)
    draw_link(from,grid_to_window(node_points[node_num]),excitatoryYN);
  //else error
}

void Ann_Window::ButtonDown(unsigned int x,unsigned int y)
{ XPoint grid_point = window_to_grid(x,y);
  int node_num;
  for (node_num=0;
       (node_num<num_nodes) &&
       ( (node_points[node_num].x != grid_point.x) ||
         (node_points[node_num].y != grid_point.y) );
       node_num++);
  if (node_num != num_nodes)
    selected_node_num = node_num; //set this node to be moved
  //else no node where point snaps to the grid
}

void Ann_Window::MouseMove(unsigned int x,unsigned int y)
{ if (selected_node_num != -1)    //if a node has been selected to be moved
  { node_points[selected_node_num] = window_to_grid(x,y);
    XClearWindow(X_display,*X_window);
    draw_Ann();
  }
}

void Ann_Window::QueryMouseMove()
{ if (selected_node_num == -1) return;  //check that a node has been selected
  Window d1,d2; int d3,d4,x,y; unsigned int d5;
  XQueryPointer(X_display,*X_window,&d1,&d2,&d3,&d4,&x,&y,&d5);
  MouseMove(x,y);
}

void Ann_Window::ButtonUp()
{ selected_node_num = -1;       //unselect the node
}

void X_draw_text_with_Ann_Standard_pen(const char *text)
{ int strlen_text = strlen(text);
  XDrawString(ann_window_for_text_output->X_display,
              *(ann_window_for_text_output->X_window),
              ann_window_for_text_output->X_Ann_Standard_pen,
              ann_window_for_text_output->X_Ann_Standard_pen_at.x,
              ann_window_for_text_output->X_Ann_Standard_pen_at.y,
              text,strlen_text);
  if (ann_window_for_text_output->X_Ann_Standard_pen_fontstruct)
    ann_window_for_text_output->X_Ann_Standard_pen_at.x
        += XTextWidth(ann_window_for_text_output->X_Ann_Standard_pen_fontstruct,
                      text,strlen_text);
  else //no font structure
    ann_window_for_text_output->X_Ann_Standard_pen_at.x
        += strlen_text * default_font_width;
}

void X_draw_int_with_Ann_Standard_pen(int number)
{ char text[20];
  sprintf(text,"%d",number);
  X_draw_text_with_Ann_Standard_pen(text);
}

void X_newline_on_Ann_Standard_pen()
{ ann_window_for_text_output->X_Ann_Standard_pen_at.x = 0;
  ann_window_for_text_output->X_Ann_Standard_pen_at.y
      += ann_window_for_text_output->X_Ann_Standard_pen_font_height;
}

void Ann_Window::draw_Ann()
{ ann_window_for_text_output = this;
  X_Ann_Standard_pen_at.x = X_Ann_Standard_pen_at.y = 0;
/*No longer show genotype - commented out:
  X_newline_on_Ann_Standard_pen();
  X_draw_text_with_Ann_Standard_pen("Genotype: ");
  for (Gene *gene=ann->genotype->first_gene;
       gene<=ann->genotype->last_gene;gene++)
    X_draw_int_with_Ann_Standard_pen((int) *gene);
*/
  X_newline_on_Ann_Standard_pen();
  X_draw_text_with_Ann_Standard_pen("Genotype length: ");
  X_draw_int_with_Ann_Standard_pen(ann->genotype->length());
  //Show rules:
  X_newline_on_Ann_Standard_pen();
  ann->output_rules();
  //Now draw the net:
  int node_num;
  for (node_num=0;node_num<num_nodes;node_num++)
  { Node *node = nodes[node_num];
    Link *link;
    XPoint from = grid_to_window(node_points[node_num]);
    for (link=node->inhibitory_output_links->next;link;link=link->next)
      draw_link_to_node(from,link->other_node,inhibitory);
    for (link=node->excitatory_output_links->next;link;link=link->next)
      draw_link_to_node(from,link->other_node,excitatory);
  }
  for (node_num=0;node_num<num_nodes;node_num++)
  { X_Ann_Standard_pen_at = grid_to_window(node_points[node_num]);
    if (node_chars[node_num][0] == 'i')
      X_Ann_Standard_pen_at.y += X_Ann_Standard_pen_font_height;
    X_draw_text_with_Ann_Standard_pen(node_chars[node_num]);
  }
}


//GENES

void Gene_string::output_seconds_of_pairs()
{ for (Gene *this_gene=first_gene+1;this_gene<=last_gene;this_gene+=2)
    X_draw_int_with_Ann_Standard_pen((int) *this_gene);
}


//L-SYSTEMS

void Production_rule::output_rule()
{ X_draw_text_with_Ann_Standard_pen("Rule: ");
#ifndef dont_use_contexts
  if (left_context.first_gene)
  { X_draw_int_with_Ann_Standard_pen((int) left_context_tory);      
    X_draw_text_with_Ann_Standard_pen(",");
    left_context.output_seconds_of_pairs();
  }
#endif  //dont_use_contexts
  X_draw_text_with_Ann_Standard_pen("<");
  predecessor.output_seconds_of_pairs();
  X_draw_text_with_Ann_Standard_pen(">");
#ifndef dont_use_contexts
  if (right_context.first_gene)
  { X_draw_int_with_Ann_Standard_pen((int) right_context_tory);      
    X_draw_text_with_Ann_Standard_pen(",");
    right_context.output_seconds_of_pairs();
  }
#endif  //dont_use_contexts
  X_draw_text_with_Ann_Standard_pen("->");
  successor_replacement.output_seconds_of_pairs();
  X_draw_text_with_Ann_Standard_pen("*");
  successor_new.output_seconds_of_pairs();
  X_draw_text_with_Ann_Standard_pen("*");
  X_draw_int_with_Ann_Standard_pen((int) reverse_inputs_type_on_new);
  X_draw_int_with_Ann_Standard_pen((int) reverse_outputs_type_on_new);
  X_draw_int_with_Ann_Standard_pen((int) inhibitory_replacement_to_new);
  X_draw_int_with_Ann_Standard_pen((int) excitatory_replacement_to_new);
  X_draw_int_with_Ann_Standard_pen((int) inhibitory_new_to_replacement);
  X_draw_int_with_Ann_Standard_pen((int) excitatory_new_to_replacement);
  X_newline_on_Ann_Standard_pen();
}


//ANNS

//Note: nodes contain lists of links; nets contain lists of nodes.
//      in each case, the first element in a list is a dummy
//      (this is so that the first real element can be easily removed)

void Ann::output_rules()
{ if (first_rule)
  { X_draw_text_with_Ann_Standard_pen("Valid Rules (after some filtering):");
    X_newline_on_Ann_Standard_pen();
    for (Production_rule *rule=first_rule;rule;rule=rule->next)
       rule->output_rule();
  }
  else
  { X_draw_text_with_Ann_Standard_pen("No Valid Rules (after filtering).");
    X_newline_on_Ann_Standard_pen();
  }
}
