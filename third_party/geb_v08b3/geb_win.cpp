/* Copyright (c) 2011 Alastair Channon
* 
* Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
* 
* The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
* 
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

/* geb_win.cpp */
#include "geb_rs.h"
#include "geb_file.h"
#include "geb_core.h"
#include "ann_win.h"
#include <owl/framewin.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

const char *geb_version = "08b";

#define disable_exceptions //in case of difficulties with exceptions

#define memory_defragmentation_by_rereading_on_autofile
#ifndef memory_defragmentation_by_rereading_on_autofile
#warning: NOT Using memory_defragmentation_by_rereading_on_autofile
// memory_defragmentation_by_rereading_on_autofile can save a significant amount of
// both memory and run-time on long runs, especially when rulestats are turned on.
#endif  //memory_defragmentation_by_rereading_on_autofile

//#define display_delay_with_f_and_s_keys
//#ifdef  display_delay_with_f_and_s_keys
//#define detect_key_presses
//#include <time.h>
//int display_delay_level=0;
//const long MAX_DISPLAY_DELAY_LEVEL=10;
//const long MIN_DISPLAY_DELAY_NANOSEC=999999999L>>(MAX_DISPLAY_DELAY_LEVEL-1);
//#endif  //display_delay_with_f_and_s_keys

#ifndef disable_exceptions
#include <exception>
#include <new>
#include <stdio.h>
#include <stdlib.h>
void gebNewHandler()        {fprintf(stderr,"\nMemory allocation exception caught. Exiting.\n"); fflush(stderr); exit(101);}
void gebUnexpectedHandler() {fprintf(stderr,"\nUnexpected exception caught. Exiting.\n");        fflush(stderr); exit(102);}
void gebTerminateHandler()  {fprintf(stderr,"\nUnhandled exception caught. Aborting.\n");        fflush(stderr); abort();}
void (*oldNewHandler)()        = std::set_new_handler(gebNewHandler);
void (*oldUnexpectedHandler)() = std::set_unexpected(gebUnexpectedHandler);
void (*oldTerminateHandler)()  = std::set_terminate(gebTerminateHandler);
#endif  //disable_exceptions

char geb_file__write__name[] = "snapshot.geb";
char *geb_file__ga_initialise__name;

int messages_on = 0;        //default is off
int stats_on = 0;           //default is off
const int stats_every = 100;
int rulestat_on = 0;        //default is off
int advanced_on = 0;        //default is off
int file_on = 0;            //default is off
int autofile_on = 0;        //default is off
/*const*/ int autofile_every = 10000;
//const char *autofile_every_string = "10k";
int grid_on = 0;            //default is off
int memjog_on = 0;          //default is off
int stopat = 0;            //default is off

int* memjog_array [100];

int bxl=10,byt=10,bxr=10,byb=10; // border

//int display_organisms = 1; // 0=no (display halted, apart from time), 1=yes
TClientDC *world_window_pen = 0;
TClientDC *world_window_pen_blue;
TClientDC *world_window_pen_red;
TClientDC *world_window_pen_green;
TClientDC *world_window_clear_pen;

//long int GA_time initialised and incremented in geb_core
long int old_GA_time;
char GA_time_string[17];
const int GA_time_text_height = 10;
void show_GA_time()
{ world_window_pen->TextOut(80,0, ltoa(GA_time,GA_time_string,10));
}

int world_window_x_step_size=0, world_window_y_step_size=0;

void show_grid()
{ int _bxl = bxl + (world_window_x_step_size>>1); int _bxr = bxr - (world_window_x_step_size>>1);
  int _byt = byt + (world_window_y_step_size>>1); int _byb = byb - (world_window_y_step_size>>1);
  for (int n=0;n<=world_length;n++) 
  { world_window_pen->MoveTo(_bxl,_byt+n*world_window_y_step_size); world_window_pen->LineTo(_bxr,_byt+n*world_window_y_step_size);
    world_window_pen->MoveTo(_bxl+n*world_window_x_step_size,_byt); world_window_pen->LineTo(_bxl+n*world_window_x_step_size,_byb);
  }
}

void output_individual(int at_x_64,int at_y_64,int direction_deg,TClientDC *pen)
{ if (!pen) return;
  int x = (((long)at_x_64 * world_window_x_step_size)>>6) + bxl;
  int y = (((long)at_y_64 * world_window_y_step_size)>>6) + byt;
  x += ((world_window_x_step_size>>2)*sindeg64[direction_deg])/(1<<6);
  y -= ((world_window_y_step_size>>2)*cosdeg64[direction_deg])/(1<<6);
  /*
  pen->Pie(x,y,x+world_window_x_step_size,y+world_window_y_step_size,
           x+(((world_window_x_step_size>>1)*((1<<6)-sindeg64[(direction_deg+10)%360]))>>6),
           y+(((world_window_y_step_size>>1)*((1<<6)+cosdeg64[(direction_deg+10)%360]))>>6),
           x+(((world_window_x_step_size>>1)*((1<<6)-sindeg64[(direction_deg+350)%360]))>>6),
           y+(((world_window_y_step_size>>1)*((1<<6)+cosdeg64[(direction_deg+350)%360]))>>6));

  */
  //Faster:
  TPoint tst[3] = {TPoint(x+(world_window_x_step_size>>1),y+(world_window_y_step_size>>1)),
                   TPoint(x+(((world_window_x_step_size>>1)*((1<<6)-sindeg64[(direction_deg+10)%360]))>>6),
                          y+(((world_window_y_step_size>>1)*((1<<6)+cosdeg64[(direction_deg+10)%360]))>>6)),
                   TPoint(x+(((world_window_x_step_size>>1)*((1<<6)-sindeg64[(direction_deg+350)%360]))>>6),
                          y+(((world_window_y_step_size>>1)*((1<<6)+cosdeg64[(direction_deg+350)%360]))>>6))};
  pen->Polygon(tst,3);
}

void show_individual(int at_x_64,int at_y_64,int direction_deg,int pen_colour)
{ //if (!display_organisms) return;
  TClientDC *pen;
  switch (pen_colour)
  { case 1 : pen = world_window_pen_blue;  break;
    case 2 : pen = world_window_pen_red;   break;
    case 3 : pen = world_window_pen_green; break;
    default: pen = world_window_pen;       break;
  }
  output_individual(at_x_64,at_y_64,direction_deg,pen);
}

void unshow_individual(int at_x_64,int at_y_64,int direction_deg)
{ //if (display_organisms)
  output_individual(at_x_64,at_y_64,direction_deg,world_window_clear_pen);
}

void draw_world()
{ if (!world_window_pen) return;
  world_window_pen->TextOut(0,0,"Time:");
  show_GA_time();
  world_window_pen->MoveTo(bxl,byt);
  world_window_pen->LineTo(bxr,byt);
  world_window_pen->LineTo(bxr,byb);
  world_window_pen->LineTo(bxl,byb);
  world_window_pen->LineTo(bxl,byt);
  for (int at=0;at<world_size;at++)
    if (world[at])
      show_individual(world[at]->at_x_64,world[at]->at_y_64,
                      world[at]->direction_deg,world[at]->pen_colour);
  if (grid_on) show_grid();
}

//-----------------------------------------------------------------------------------------------------------------------

void output_simple_functionality_message()
{ fprintf(stdout,"%s","\nSimple functionality:\n");
  fprintf(stdout,"%s","=====================\n");
//#ifdef  display_delay_with_f_and_s_keys
//  fprintf(stdout,     "With World Window selected: S-key = Slower , F-key = Faster\n");
//#endif  //display_delay_with_f_and_s_keys
  fprintf(stdout,"%s","Click on an organism to display its network and pause the program\n");
  fprintf(stdout,"%s","Then: Left-click on a network's nodes to drag and drop them\n");
//  fprintf(stdout,"%s","      Middle/Right-click on a network window to remove it\n");
  fflush(stdout);
}

void output_advanced_functionality_message()
{ fprintf(stdout,"%s","\nAdvanced functionality:\n");
  fprintf(stdout,"%s","=======================\n\n");
  fprintf(stdout,"%s","In the World Window:\n");
//#ifdef  display_delay_with_f_and_s_keys
//  fprintf(stdout,     "    S-key = Slower , F-key = Faster. (The World Window must be selected.)\n");
//#endif  //display_delay_with_f_and_s_keys
  fprintf(stdout,"%s","    Left-click within the world box pauses the program.  If the (mouse)\n");
  fprintf(stdout,"%s","               pointer is over an organism, its network will be displayed.\n");
  fprintf(stdout,"%s","               If not, it should now be easy to click on an organism.\n");
  fprintf(stdout,"%s","    Left-click outside the world box will continue the program.  \n");
  fprintf(stdout,"%s%s%s",
                      "    Right-click writes a snapshot of the program to a file '",geb_file__write__name,"',\n");
  fprintf(stdout,"%s","                which may be used with the '-file' argument.  \n");
/*
  fprintf(stdout,"%s","    Middle/Right-click within  the world box writes a snapshot of the\n");
  fprintf(stdout,"%s%s%s","                       program to a file '",geb_file__write__name,"', which may be\n");
  fprintf(stdout,"%s","                       used with the '-file' argument.  \n");
  //fprintf(stdout,"%s%s%s","                       the program to both stdout and a file '",geb_file__write__name,"',\n");
  //fprintf(stdout,"%s","                       which may be used with the '-file' argument.  \n");
  fprintf(stdout,"%s","    Middle/Right-click outside the world box cancels the display - useful\n");
  fprintf(stdout,"%s","                       for leaving a run going when logged off.\n");
  fprintf(stdout,"%s","                       (Same as using arguments -pid PID -display blank.)\n");
  fprintf(stdout,"%s","                       [If the program was compiled with disable_signals]\n");
  fprintf(stdout,"%s","                       [defined then this click will toggle the display.]\n\n");
*/
  fprintf(stdout,"%s","In a Network Window: As with simple functionality:\n");
  fprintf(stdout,"%s","    Left-click on a network's nodes to drag and drop them\n");
//  fprintf(stdout,"%s","    Middle/Right-click on a network window to remove it\n");
  fflush(stdout);
}

void usage(int argc,char *argv[])
{ fprintf(stdout,"\n");
  fprintf(stdout,"WARNING: This Windows port of Geb has not been fully\n");
  fprintf(stdout,"WARNING: tested.  It is recommended that you shut down\n");
  fprintf(stdout,"WARNING: all other applications before continuing.\n\n");
  // fprintf(stdout,"WARNING: Do not attempt to run under Windows 3.x.\n\n");
  fprintf(stdout,"Do you want to continue? ");
  if (fgetchar() != 'y') exit(0);  fprintf(stdout,"\n");
  if (argc==1)
  { output_simple_functionality_message();
    fprintf(stdout,"%s%s%s","Try ",argv[0]," -help for information on arguments\n"); fflush(stdout);
  }
  for (int arg=1;arg<argc;)
  { if      (!strcmp(argv[arg],"-neurons") && ((arg+1)<argc) && atol(argv[arg+1]))
         { max_num_nodes = (int) atol(argv[arg+1]);
           fprintf(stdout,"%s%d%s","Maximum number of neurons per agent set to ",max_num_nodes,"\n"); fflush(stdout);
           arg+=2;
         }
    else if (!strcmp(argv[arg],"-length") && ((arg+1)<argc) && atol(argv[arg+1]))
         { world_length = (int) atol(argv[arg+1]);
           arg+=2;
			}
    else if (!strcmp(argv[arg],"-messages"))
         { messages_on = 1;
           arg++;
         }
    else if (!strcmp(argv[arg],"-stats"))
         { stats_on = 1;
           arg++;
         }
    else if (!strcmp(argv[arg],"-advanced"))
         { advanced_on = 1;
           arg++;
           output_advanced_functionality_message();
         }
    else if (!strcmp(argv[arg],"-full"))
         { advanced_on = 1;
           stats_on = 1;
           messages_on = 1;
           arg++;
         }
    else if (!strcmp(argv[arg],"-file"))
         { if (((arg+1)==argc) || ((argv[arg+1][0] == '-')&&(strcmp(argv[arg+1],"-"))))
           { geb_file__ga_initialise__name = geb_file__write__name;
				 arg++;
           }
           else
           { geb_file__ga_initialise__name = argv[arg+1];;
             arg+=2;
           }
             file_on = 1;
         }
    else if (!strcmp(argv[arg],"-autofile"))
         { autofile_on = 1;
           arg++;
         }
    else if (!strcmp(argv[arg],"-autofile_every") && ((arg+1)<argc) && atol(argv[arg+1]))
         { autofile_on = 1;
           autofile_every = (int) atol(argv[arg+1]);
           fprintf(stdout,"%s%d%s","autofile_every set to ",autofile_every,"\n"); fflush(stdout);
           arg+=2;
         }
    else if (!strcmp(argv[arg],"-rulestat"))
         { rulestat_on = 1;
           arg++;
         }
    else if (!strcmp(argv[arg],"-grid"))
         { grid_on = 1;
           arg++;
         }
    else if (!strcmp(argv[arg],"-seed") && ((arg+1)<argc) && atol(argv[arg+1]))
         { geb_seed = (int) atol(argv[arg+1]);
           arg+=2;
			}
    else if (!strcmp(argv[arg],"-memjog")) // undocumented - used for testing
         { memjog_on = 1;
           arg++;
         }
    else if (!strcmp(argv[arg],"-stopat") && ((arg+1)<argc) && atol(argv[arg+1]))
         { stopat = (int) atol(argv[arg+1]);
           arg+=2;
         }
    else
    { fprintf(stderr,"%s%s%s","usage: ",argv[0]," [arguments]\n");
      fprintf(stderr,"%s%d%s"," -neurons  MAX_NEURONS  = maximum number of neurons per agent (default is ",max_num_nodes,")\n");
      fprintf(stderr,"%s%d%s"," -length   WORLD_LENGTH = length of world in organisms (default is ",world_length,")\n");
      fprintf(stderr,"%s"    ," -messages              = output messages during run\n");
      fprintf(stderr,"%s"    ," -stats                 = output statistics during run\n");
      fprintf(stderr,"%s"    ," -advanced              = extra functionality (explained on execution)\n");
      fprintf(stderr,"%s"    ," -full                  = -messages -stats -advanced (without explanation)\n");
      fprintf(stderr,"%s"    ," -file     FILE         = start run from snapshot stored in file FILE\n");
      fprintf(stderr,"%s%s%s%d%s"," -autofile              = store a snapshot in ",geb_file__write__name," every ",autofile_every," time steps\n");
      //fprintf(stderr,"%s%s%s%s%s"," -autofile              = store a snapshot in ",geb_file__write__name," every ",autofile_every_string," time steps\n");
      fprintf(stderr,"%s%d%s"," -autofile_every        = change autofile frequency from default (",autofile_every,")\n");
		fprintf(stderr,"%s"    ," -rulestat              = output rule statistics during run, to file rulestat\n");
		fprintf(stderr,"%s"    ," -grid                  = superimpose grid onto world\n");
		fprintf(stderr,"%s"    ," -seed     SEED         = specify a seed for the random number generator\n");
		fprintf(stderr,"%s"    ," -stopat   GEB_TIME     = specify a stop point\n");
		fprintf(stderr,"%s"    ," If no arguments are used, simple functionality is explained on execution\n");
		exit(0);
	 }
  }
}

void geb_terminate(int graphics_need_terminating = 1,int doexit=1)
{ ga_terminate();
  if (rulestat_on) geb_rulestat_terminate(doexit);
  // if (graphics_need_terminating) ...
  if (!doexit) return;
  // if (messages_on) {fprintf(stdout,"\nProcess Terminated\n"); fflush(stdout);}
  fprintf(stderr,"\nProcess Terminated\n"); fflush(stderr);
//  exit(100);
}

void geb_stats() //Statistics:
{ int age,youngest,oldest,age_total,num_individs;
  int total_genome_lengths,SHADOWtotal_genome_lengths;
  int pen_colours_total[5]; for (int c=0;c<5;c++) pen_colours_total[c]=0;
  Organism *individual;
  age_total = num_individs = 0; youngest = GA_time; oldest = 0;
  total_genome_lengths = SHADOWtotal_genome_lengths = 0;
  for (int individual_at=0;individual_at<world_size;individual_at++)
  { individual = world[individual_at];
    if (individual)
    { num_individs++;
      age = individual->stats__age;
      age_total += age;
      if (age >   oldest)   oldest = age;
      if (age < youngest) youngest = age;
      pen_colours_total[individual->pen_colour+1]++;
      total_genome_lengths += individual->genotype->length();
    }
    if (individual_at<SHADOWstats__num_individuals)
#ifndef only_generate_rules_for_shadows_when_needed
      SHADOWtotal_genome_lengths += SHADOWworld[individual_at]->genotype->length();
#else   //only_generate_rules_for_shadows_when_needed
      SHADOWtotal_genome_lengths += SHADOWgenotypes[individual_at]->length();
#endif  //only_generate_rules_for_shadows_when_needed
  }
  fprintf(stdout,"time: T%ldT   average_age: A%fA       oldest: O%dO    youngest: Y%dY number: N%dN     number of reproductions: R%ldR  blacK%dK        B%dBlue reD%dD  G%dGreen\n",GA_time,(double)age_total/num_individs,oldest,youngest,num_individs,stats__num_reproductions,pen_colours_total[0],pen_colours_total[2],pen_colours_total[3],pen_colours_total[4]); fflush(stdout);
  fprintf(stdout,"time: S%ldS   SHADOWstats__num_individuals %d\n",GA_time,SHADOWstats__num_individuals); fflush(stdout);
  fprintf(stdout,"time: L%ldL   mean_genotype_length %d SHADOWmean_genotype_length %d\n",GA_time,total_genome_lengths/num_individs,SHADOWtotal_genome_lengths/SHADOWstats__num_individuals); fflush(stdout);
  if (num_individs<2) {printf("Terminating because number of individuals = %d\n",num_individs); fflush(stdout); exit(108);}
}

void geb_autofile()
{ int err;
  int need_to_remove_renamed_file = 0;
  char *rename_file=0;
  FILE *fp;
  if ((fp = fopen(geb_file__write__name,"r")) != NULL)
  { fclose(fp);
    const char *rename_extension = ".bak";
    rename_file = new char[1+strlen(geb_file__write__name)+strlen(rename_extension)];
    strcpy(rename_file,geb_file__write__name);
	 strcat(rename_file,rename_extension);
    err = rename(geb_file__write__name,rename_file);
    if (err)
    { fprintf(stderr,"%s%s%s%s%s","Error: autofile failed to rename file ",geb_file__write__name," to ",rename_file,"\n");
      fflush(stderr);
	 }
    else need_to_remove_renamed_file = 1;
  }
  err = geb_file__write(geb_file__write__name);
  //note: rulestat components now included in snapshot if rulestat_on - writing called from within geb_file__write
  if (err) {fprintf(stderr,"%s%s%s","Error: autofile failed to write snapshot to file ",geb_file__write__name,"\n"); fflush(stderr);}
  else if (!err && messages_on) {fprintf(stdout,"%s%s%s","Autofile: snapshot written to file ",geb_file__write__name,"\n"); fflush(stdout);}
  if (need_to_remove_renamed_file)
  { err = remove(rename_file);
    if (err) {fprintf(stderr,"%s%s%s","Error: autofile failed to remove file ",rename_file,"\n"); fflush(stderr);}
  }
  if (rename_file) delete rename_file;
}

class World_Window : public TWindow
{ public:    World_Window(TWindow *parent) : TWindow(parent) {Attr.W=400;Attr.H=300;}
//             void SetupWindow();
//bool CanClose() {return true;}
  /*protected:*/ void EvLButtonDown(uint,TPoint &);      //Message response function
				 void EvRButtonDown(uint,TPoint &);      //Message response function
             void EvSize(UINT sizeType,TSize &size); //Message response function
             void Paint(TDC& cursor,BOOL,TRect &)    //A message response function
               {draw_world();} //  calls Paint
             int number_ann_windows_open;
  DECLARE_RESPONSE_TABLE(World_Window);
};

DEFINE_RESPONSE_TABLE1(World_Window, TWindow)
  EV_WM_SIZE,
  EV_WM_LBUTTONDOWN,
  EV_WM_RBUTTONDOWN,
END_RESPONSE_TABLE;

/////////////////////////////////////////////////////////////////

void World_Window::EvSize(UINT sizeType,TSize &size)
{ Invalidate();
  TWindow::EvSize(sizeType, size);
  world_window_x_step_size = size.cx / (world_length+2);
  world_window_y_step_size = (size.cy-GA_time_text_height) / (world_length+2);
  bxl=((size.cx - (world_length+1)*world_window_x_step_size)>>1);                   //border: x left
  byt=GA_time_text_height+((size.cy - GA_time_text_height
											- (world_length+1)*world_window_y_step_size)>>1); //border: y top
  bxr=bxl+(world_length+1)*world_window_x_step_size;                               //border: x right
  byb=byt+(world_length+1)*world_window_y_step_size;                               //border: y bottom
  if (!world_window_pen) return; //world_window_pen = new TClientDC(*this);
  world_window_pen->PatBlt(0,0,size.cx,size.cy,WHITENESS); //remove old content
  draw_world();
}

void World_Window::EvLButtonDown(uint,TPoint &point)
{     int previous_number_of_ann_windows_open = number_of_ann_windows_open;
      if (number_of_ann_windows_open == -1) number_of_ann_windows_open = 0; // unpause
		if (number_of_ann_windows_open == max_num_ann_windows) return;
      int xb = -1+ (point.x + (world_window_x_step_size>>1) - bxl) / world_window_x_step_size;
      int yb = -1+ (point.y + (world_window_y_step_size>>1) - byt) / world_window_y_step_size;
      Organism *best_org=0;
      double best_distance=world_window_x_step_size<world_window_y_step_size?
                           world_window_x_step_size/3:world_window_y_step_size/3;
      for (int x_search=(xb<=0?0:xb-1);x_search<(xb>=world_length?world_length:xb+1);x_search++)
        for (int y_search=(yb<=0?0:yb-1);y_search<(yb>=world_length?world_length:yb+1);y_search++)
        { Organism *org = world[x_search + y_search * world_length];
          if (!org) continue;
			 double delta_x = ((org->at_x_64 * world_window_x_step_size)>>6) + bxl //see output_individual
                           - (point.x - (world_window_x_step_size>>1));
			 double delta_y = ((org->at_y_64 * world_window_y_step_size)>>6) + byt //see output_individual
                           - (point.y - (world_window_y_step_size>>1));
          double distance = sqrt( (delta_x*delta_x) + (delta_y*delta_y) );
          if (distance < best_distance)
          { best_org = org;
				best_distance = distance;
          }
        }
      if (best_org)
      { (new Ann_Window(best_org,this))->Create();
        if (messages_on) {fprintf(stdout,"ORG: at_x=%d	at_y=%d	deg=%d	age=%d\n",best_org->at_x,best_org->at_y,best_org->direction_deg,best_org->stats__age); fflush(stdout);}
		}
      else if (advanced_on)
      { if (((point.y>=byt)&&(point.y<=byb)&&(point.x>=bxl)&&(point.x<=bxr))
            &&(number_of_ann_windows_open==0))
        { if (previous_number_of_ann_windows_open == 0) if (messages_on) {fprintf(stdout,"Paused\n"); fflush(stdout);}
          number_of_ann_windows_open = -1; //pause
        }
        else if (((point.y<byt)||(point.y>byb)||(point.x<bxl)||(point.x>bxr))
                 &&(previous_number_of_ann_windows_open==-1)) if (messages_on) {fprintf(stdout,"UnPaused\n"); fflush(stdout);}
      }
}

void World_Window::EvRButtonDown(uint,TPoint &point)
{ if (advanced_on)
  {     if ((point.y>=byt)&&(point.y<=byb)&&(point.x>=bxl)&&(point.x<=bxr))
        { int err = geb_file__write(geb_file__write__name);
          //note: rulestat components now included in snapshot if rulestat_on - writing called from within geb_file__write
			 if (!err && messages_on) {fprintf(stdout,"%s%s%s","Snapshot written to file ",geb_file__write__name,"\n"); fflush(stdout);}
          return;
		  }
/*
        //else:
        if (display_organisms)
		  { display_organisms = 0;
          if (messages_on) {fprintf(stdout,"Display (only) temporarily Halted by user (for faster running)\n"); fflush(stdout);}
        }
        else
        { if (!world_window_pen) return; //over-cautious but still
          display_organisms = 1;
          world_window_pen->PatBlt(0,0,Attr.W,Attr.H,WHITENESS); //remove old content
          draw_world();
          if (messages_on) {fprintf(stdout,"Display UnHalted by user\n"); fflush(stdout);}
        }
*/
  }
}

//#ifdef  detect_key_presses
//    case KeyPress:
//      KeySym tmp_keysym;
//      char tmp_text[5];
//      if (XLookupString(&(X_event.xkey),tmp_text,5,&tmp_keysym,0))
//        switch (tmp_text[0])
//        {
//#ifdef  display_delay_with_f_and_s_keys
//          case 'F':
//          case 'f': // faster
//            if (display_delay_level==0) break;
//            display_delay_level--;
//            if (messages_on) {fprintf(stdout,"Faster: display_delay_level now %d\n",display_delay_level); fflush(stdout);}
//            break;
//          case 'S':
//          case 's': // slower
//            if (display_delay_level==MAX_DISPLAY_DELAY_LEVEL) break;
//            display_delay_level++;
//            if (messages_on) {fprintf(stdout,"Slower: display_delay_level now %d\n",display_delay_level); fflush(stdout);}
//            break;
//#endif  //display_delay_with_f_and_s_keys
//        }
//      break;
//#endif  //detect_key_presses

class Geb_Application : public TApplication
{ public:   // Geb_Application() : TApplication() {}
				//~Geb_Application() {geb_terminate();}
//bool CanClose() {return true;}
  /*protected:*/
				 void InitMainWindow();
				 void InitInstance();
//void InitApplication() {};
bool IdleAction(long idleCount);
};

void Geb_Application::InitMainWindow()
{  TFrameWindow *fw = new TFrameWindow(0,"GEB: World Window",new World_Window(0),true);
	//fw->Attr.Style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
	fw->Attr.Style = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
	SetMainWindow(fw);
}

void Geb_Application::InitInstance()
{ TApplication::InitInstance();
  if (file_on)
  { int read_rnd_state = geb_seed?0:1; // read random number state from snapshot unless seed given on command line
	 int err = geb_file__ga_initialise(geb_file__ga_initialise__name,read_rnd_state);
	 //note: rulestat components now included in snapshot if rulestat_on - reading called from within geb_file__ga_initialise
	 if (err) exit(106); //else:
	 if (messages_on)
	 { if (!strcmp(geb_file__ga_initialise__name,"-")) fprintf(stdout,"%s","Snapshot read from standard input\n");
		else fprintf(stdout,"%s%s%s","Snapshot read from file ",geb_file__ga_initialise__name,"\n");
		if (!read_rnd_state) printf("Seed used for this restart: %d\n",geb_seed);
		fflush(stdout);
	 }
  }
  else
  { ga_initialise();
	 if (messages_on) {printf("Seed used for this run: %d\n",geb_seed); fflush(stdout);}
	 if (rulestat_on) geb_rulestat_initialise();
  }
  number_of_ann_windows_open = 0;
  world_window_pen = new TClientDC(*MainWindow);
  world_window_pen->SelectObject(TBrush(TColor::Black));
  world_window_pen->SelectObject(TPen(TColor::Black));
  world_window_pen_green = new TClientDC(*MainWindow);
  world_window_pen_green->SelectObject(TBrush(TColor::LtGreen));
  world_window_pen_green->SelectObject(TPen(TColor::LtGreen));
  world_window_pen_blue = new TClientDC(*MainWindow);
  world_window_pen_blue->SelectObject(TBrush(TColor::LtBlue));
  world_window_pen_blue->SelectObject(TPen(TColor::LtBlue));
  world_window_pen_red = new TClientDC(*MainWindow);
  world_window_pen_red->SelectObject(TBrush(TColor::LtRed));
  world_window_pen_red->SelectObject(TPen(TColor::LtRed));
  world_window_clear_pen = new TClientDC(*MainWindow);
  world_window_clear_pen->SelectObject(TBrush(TColor::White));
  world_window_clear_pen->SelectObject(TPen(TColor::White));
  draw_world();
}

bool Geb_Application::IdleAction(long idleCount)
{	 if (number_of_ann_windows_open == 0)
	 { old_GA_time = GA_time;
//while (GA_time == old_GA_time)
		ga_main_loop();
		if (GA_time != old_GA_time)
		{ show_GA_time();
		  old_GA_time = GA_time;
		  if (grid_on) show_grid();
		  if (stats_on && (!(GA_time%stats_every))) geb_stats();
		  if (rulestat_on && (!(GA_time%rulestat_every))) geb_rulestat();
		  if (autofile_on && (!(GA_time%autofile_every)))
		  { geb_autofile();
#ifdef  memory_defragmentation_by_rereading_on_autofile
			 if (messages_on) {printf("+++ Memory defragmentation starting +++\n"); fflush(stdout);}
			 //old (now in snapshot): if (messages_on) {printf("Saving random number state.\n"); fflush(stdout);}
			 //old (now in snapshot): rnd_state s;      // save and restore rnd_state for backwards compatibility (reproducibility) with no-defrag versions
			 //old (now in snapshot): get_rnd_state(s); // save and restore rnd_state for backwards compatibility (reproducibility) with no-defrag versions
			 if (messages_on) {printf("Terminating current run."); fflush(stdout);}
			 geb_terminate(0,0);
			 if (messages_on) {printf("\nRestarting from snapshot:\n"); fflush(stdout);}
			 int err = geb_file__ga_initialise(geb_file__write__name); //nb. the file name used here is the write name
			 //note: rulestat components now included in snapshot if rulestat_on - reading called from within geb_file__ga_initialise
			 if (err) exit(107); //else:
			 //old (now in snapshot): if (messages_on) {printf("Restoring random number state.\n"); fflush(stdout);}
			 //old (now in snapshot): set_rnd_state(s); // save and restore rnd_state for backwards compatibility (reproducibility) with no-defrag versions
			 if (messages_on) {printf("--- Memory defragmentation finished ---\n"); fflush(stdout);}
#endif  //memory_defragmentation_by_rereading_on_autofile
		  }
//#ifdef  display_delay_with_f_and_s_keys
//        if (display && display_delay_level)
//        { struct timespec delay  = {0,MIN_DISPLAY_DELAY_NANOSEC<<(display_delay_level-1)}; // in nanoseconds
//          nanosleep(&delay,NULL);
//        }
//#endif  //display_delay_with_f_and_s_keys
		  if (memjog_on)
		  { if (GA_time%2) for (int s=0;s<100;s++) delete [] memjog_array[s];
			 else           for (int s=0;s<100;s++) memjog_array[s] = new int [s];
		  }
//        if (stopat && (GA_time==stopat)) not_done=0;
		  if (stopat && (GA_time==stopat)) {geb_terminate(1,1);BreakMessageLoop=true;}
//if (!(GA_time%100) && world_window_pen)
//{ world_window_pen->PatBlt(0,0,Attr.W,Attr.H,WHITENESS); //remove old content
//  draw_world(); //when using pen widths < 2, rubbish left behind - these 4 lines clear it up
//}
		}
	 }
	 MSG msg;
	 if (PeekMessage(&msg, 0, 0, 0, PM_NOYIELD | PM_NOREMOVE)) PumpWaitingMessages();
	 return true;
}

int OwlMain(int argc,char *argv[])
{ _InitEasyWin();
  fprintf(stderr,"Geb version %s. Copyright (C) Alastair Channon.\n",geb_version);
  usage(argc,argv);
  try {return Geb_Application().Run();}
  catch (xmsg& x) {::MessageBox(0,x.why().c_str(),"Exception",MB_OK);}
  return -1;
}

