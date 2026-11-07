/* Copyright (c) 2004 Alastair Channon
* 
* Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
* 
* The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
* 
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

/* ann_win.cpp */
#include "ann_win.h"
#include <stdio.h>

const int max_character_length = 13; //for "long part_character" - can be extended using "long long"&"%Ld", but that's non-portable & non-essential

int number_of_ann_windows_open;
int selected_node_num;
Ann_Window_ostream tout;    //tout: pseudo-streamed text output into Ann_Windows
const int tout_char_height =20; //      tout <<{1st-thing} <<{2nd-thing} not allowed

int grid_fineness=60; //a window co-ordinate snaps to 1 of grid_fineness on grid

DEFINE_RESPONSE_TABLE1(Ann_Window,TFrameWindow)
  EV_WM_SIZE,
  EV_WM_LBUTTONDOWN,
  EV_WM_MOUSEMOVE,
  EV_WM_LBUTTONUP,
//  EV_WM_RBUTTONDOWN,
END_RESPONSE_TABLE;

Ann_Window::Ann_Window(Ann *_ann,TWindow *parent)
  : TFrameWindow(parent,"Ann Window")
{ number_of_ann_windows_open++;
  Attr.Style = WS_VISIBLE | WS_POPUP | WS_OVERLAPPEDWINDOW;
  Attr.W = 400; Attr.H = 200;
  ann = _ann;
  Node *node;
  // Count the number of nodes (note: not including the dummy):
  for (num_nodes = 0,node=ann->nodes->next;node;node=node->next) num_nodes++;
  // Store the nodes (pointers to) and their display positions in arrays:
  nodes = new Node *[num_nodes];
  node_points = new TPoint[num_nodes];
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
  number_of_ann_windows_open--;
}

TPoint Ann_Window::grid_to_window(TPoint &point)
{ return TPoint((int)((  ((long)Attr.W)     *point.x)/grid_fineness),
					 (int)(( (((long)Attr.H)-22) *point.y)/grid_fineness));
}

TPoint Ann_Window::window_to_grid(TPoint &point)
{ return
	 TPoint((int)(( ((long)point.x) *grid_fineness+ Attr.W    /2)/ Attr.W    ),
			  (int)(( ((long)point.y) *grid_fineness+(Attr.H-22)/2)/(Attr.H-22)));
}

void Ann_Window::draw_inhibitory_to_node_using_cursor(Node *node,Cursor *cursor)
{ for (int node_num=0;(node_num<num_nodes)&&(nodes[node_num]!=node);node_num++);
  if (node_num != num_nodes)
    cursor->Inhibitory_LineTo(grid_to_window(node_points[node_num]));
  //else error
}

void Ann_Window::draw_excitatory_to_node_using_cursor(Node *node,Cursor *cursor)
{ for (int node_num=0;(node_num<num_nodes)&&(nodes[node_num]!=node);node_num++);
  if (node_num != num_nodes)
    cursor->Excitatory_LineTo(grid_to_window(node_points[node_num]));
  //else error
}

void Ann_Window::draw_Ann_using_cursor(Cursor *cursor)
{ tout.cursor = cursor;
  cursor->MoveTo(0,0);
  tout << "GENOTYPE: ";
  for (Gene *gene=ann->genotype->first_gene;
       gene<=ann->genotype->last_gene;gene++)
    tout << (int) *gene;
  tout.endl();
  ann->output_rules();
  //Now draw the net:
  int node_num;
  for (node_num=0;node_num<num_nodes;node_num++)
  { Node *node = nodes[node_num];
    Link *link;
    for (link=node->inhibitory_output_links->next;link;link=link->next)
    { cursor->MoveTo(grid_to_window(node_points[node_num]));
      draw_inhibitory_to_node_using_cursor(link->other_node,cursor);
    }
    for (link=node->excitatory_output_links->next;link;link=link->next)
    { cursor->MoveTo(grid_to_window(node_points[node_num]));
	   draw_excitatory_to_node_using_cursor(link->other_node,cursor);
    }
  }
  for (node_num=0;node_num<num_nodes;node_num++)
  { if (node_chars[node_num][0] == 'o')
      cursor->MoveTo(grid_to_window(node_points[node_num])
                     -TPoint(0,tout_char_height));
    else cursor->MoveTo(grid_to_window(node_points[node_num]));
    tout << node_chars[node_num];
  }
}

void Ann_Window::EvLButtonDown(uint,TPoint &point)
{ TPoint grid_point = window_to_grid(point);
  for (int node_num=0;
       (node_num<num_nodes)&&(node_points[node_num]!=grid_point);
       node_num++);
  if (node_num != num_nodes)
    selected_node_num = node_num; //set this node to be moved
  //else no node where point snaps to the grid
}

void Ann_Window::EvMouseMove(uint,TPoint &point)
{ if (selected_node_num != -1)    //if a node has been selected to be moved
  { node_points[selected_node_num] = window_to_grid(point);
    Invalidate(); //mark the window for re-painting
  }
}

void Ann_Window::EvLButtonUp(uint,TPoint &point)
{ if (selected_node_num != -1)    //if a node has been selected to be moved
  { node_points[selected_node_num] = window_to_grid(point);
    Invalidate(); //mark the window for re-painting
    selected_node_num = -1;       //unselect the node
  }
}

/*
void Ann_Window::EvRButtonDown(uint,TPoint &)
{ //Put a metafile picture of the Ann onto the clipboard
  //Used for documentation
  HDC metafile_device = CreateMetaFile(NULL);
  Cursor metafile_cursor(metafile_device);
  int old_grid_fineness = grid_fineness;
  grid_fineness /= 2; //temporarily adjust output scale
  draw_Ann_using_cursor(&metafile_cursor);
  grid_fineness = old_grid_fineness;
  TMetaFilePict metafilepict(CloseMetaFile(metafile_device));
  TClipboard clipboard(HWindow);
  clipboard.EmptyClipboard();
  metafilepict.ToClipboard(clipboard,MM_TEXT); //MM_TEXT: map to device pixels
  clipboard.CloseClipboard();
}
*/

void Ann_Window_ostream::operator << (int &i)
{ cursor->SetTextAlign(TA_UPDATECP);
  cursor->TextOut(0,0,itoa(i,string,10));
}

void Ann_Window_ostream::operator << (char *chars)
{ cursor->SetTextAlign(TA_UPDATECP);
  cursor->TextOut(0,0,chars);
}

void Ann_Window_ostream::endl()
{ //TDC::GetCurrentPosition() does not work with metafile devices, so need 'at'
  cursor->MoveTo(0,cursor->at.y+tout_char_height);
}


//GENES

void Gene_string::output_seconds_of_pairs()
{ for (Gene *this_gene=first_gene+1;this_gene<=last_gene;this_gene+=2)
    tout << (int) *this_gene;
}


//L-SYSTEMS

void Production_rule::output_rule()
{ tout << "Rule: ";
#ifndef dont_use_contexts
  if (left_context.first_gene)
  { tout << (int) left_context_tory;                tout << ",";
    left_context.output_seconds_of_pairs();
  }
#endif  //dont_use_contexts
                                                    tout << "<";
  predecessor.output_seconds_of_pairs();            tout << ">";
#ifndef dont_use_contexts
  if (right_context.first_gene)
  { tout << (int) right_context_tory;               tout << ",";
    right_context.output_seconds_of_pairs();
  }
#endif  //dont_use_contexts
                                                    tout << "->";
  successor_replacement.output_seconds_of_pairs();  tout << "*";
  successor_new.output_seconds_of_pairs();          tout << "*";
  tout << (int) reverse_inputs_type_on_new;
  tout << (int) reverse_outputs_type_on_new;
  tout << (int) inhibitory_replacement_to_new;
  tout << (int) excitatory_replacement_to_new;
  tout << (int) inhibitory_new_to_replacement;
  tout << (int) excitatory_new_to_replacement;
  tout.endl();
}


//ANNS

//Note: nodes contain lists of links; nets contain lists of nodes.
//      in each case, the first element in a list is a dummy
//      (this is so that the first real element can be easily removed)

void Ann::output_rules()
{ if (first_rule)
  { tout << "Valid Rules (after some filtering):"; tout.endl();
    for (Production_rule *rule=first_rule;rule;rule=rule->next)
	   rule->output_rule();
  }
  else
  { tout << "No Valid Rules (after filtering)."; tout.endl();
  }
}

bool Cursor::LineTo(int x, int y) {return LineTo(TPoint(x,y));}

bool Cursor::LineTo(const TPoint& point)
{ at = point;
  return (TDC::LineTo(point));
}

bool Cursor::MoveTo(int x, int y) {return MoveTo(TPoint(x,y));}

bool Cursor::MoveTo(const TPoint& point)
{ at = point;
  return (TDC::MoveTo(point));
}

void Cursor::Inhibitory_LineTo(TPoint &point)
{ SelectObject(*Inhibitory_Pen);
  Draw_Link_To(point,0);
}

void Cursor::Excitatory_LineTo(TPoint &point)
{ SelectObject(*Excitatory_Pen);
  Draw_Link_To(point,1);
}

void Cursor::Draw_Link_To(TPoint &point,int excitatory)
{ TPoint midpoint;
  if (excitatory) midpoint = TPoint((at.x+point.x)>>1,(at.y+point.y)>>1);
  //else inhibitory - want arrow at different point on line in case have both
  else midpoint = TPoint((3*at.x+2*point.x)/5,(3*at.y+2*point.y)/5);
  LineTo(midpoint);
  // at midpoint, draw an arrow showing direction:
  int x_diff = (point.x - at.x); int y_diff = (point.y - at.y);
  int normalise = max(16,max(abs(x_diff),abs(y_diff))) >>3; //pseudo-normalise
  x_diff /= normalise; y_diff /= normalise;
  LineTo(midpoint-TPoint(x_diff+(y_diff>>1),y_diff-(x_diff>>1)));
  MoveTo(midpoint-TPoint(x_diff-(y_diff>>1),y_diff+(x_diff>>1)));
  LineTo(midpoint);
  // contine with main line:
  LineTo(point);
}

void Cursor::init_pens()
{ Inhibitory_Pen = new TPen(TColor::LtRed,1,PS_DASH );
  Excitatory_Pen = new TPen(TColor::Black,1,PS_SOLID);
}

