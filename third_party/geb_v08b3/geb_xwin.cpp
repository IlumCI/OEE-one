/* Copyright (c) 2021 Alastair Channon
* 
* Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
* 
* The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
* 
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

/* geb_xwin.cpp */
#include "geb_rs.h"
#include "geb_file.h"
#include "geb_core.h"
#include "ann_xwin.h"
#include "win_xwin.h"
#include <X11/Xutil.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include "geb_xver.h"

//#define disable_signals    //in case of difficulties with signals
#define disable_exceptions   //in case of difficulties with exceptions

#define memory_defragmentation_by_rereading_on_autofile
#ifndef memory_defragmentation_by_rereading_on_autofile
#warning: NOT Using memory_defragmentation_by_rereading_on_autofile
// memory_defragmentation_by_rereading_on_autofile can save a significant amount of
// both memory and run-time on long runs, especially when rulestats are turned on.
#endif  //memory_defragmentation_by_rereading_on_autofile

#define display_delay_with_f_and_s_keys
#ifdef  display_delay_with_f_and_s_keys
#define detect_key_presses
#include <time.h>
int display_delay_level=0;
const long MAX_DISPLAY_DELAY_LEVEL=10;
const long MIN_DISPLAY_DELAY_NANOSEC=999999999L>>(MAX_DISPLAY_DELAY_LEVEL-1);
#endif  //display_delay_with_f_and_s_keys

//#define gprofile       //used in code development
//#define gprofile_long  //for profiling over a long time

#ifdef  gprofile
#warning: Compiling for gprofiling (compile with -pg)
#endif  //gprofile

#ifdef  gprofile_long
#ifndef gprofile
#define gprofile
#endif  //gprofile
#warning: Compiling for long gprofiling (compile with -pg)
#endif  //gprofile_long

//#define memprof        //used in code development (I used Gnome memprof 0.4.0)

#ifdef  memprof
#warning: Compiling for memory profiling and leak detection (compile with -g)
#define disable_signals
#define detect_key_presses
#include <time.h>
const int memprof__pause_in_seconds = 10;
#endif  //memprof

#ifndef disable_signals
#include <signal.h>
#include <sys/types.h>
//unistd.h now needed for autofile (file rename)
//#include <unistd.h>
#ifdef  gebHPUX
#include <sys/procset.h>
#endif  //gebHPUX
#endif  //disable_signals

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

// A note for Linux users
// ----------------------
// _LINUX_SIGNAL_H or __linux__ or gebLinux should have been #defined
// either in one of the o/s system header files #included above,
// or through a compiler macro definition (perhaps in makefilestart.Linux).  
// Geb uses this to choose between sigset&sigsend [non-Linux] and
// sigaction&kill [Linux].  

#ifdef  __linux__
#define _LINUX_SIGNAL_H
#endif  //__linux__

#ifdef  gebLinux
#define _LINUX_SIGNAL_H
#endif  //gebLinux

#ifndef disable_signals
#else   //disable_signals
char *halted_display;
#endif  //disable_signals

char geb_file__write__name[] = "snapshot.geb";
char *geb_file__ga_initialise__name;

const char *display_name_file_start = "/tmp/.geb_display.";

#ifndef memprof
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
#else   //memprof
int messages_on = 1;
int stats_on = 1;
const int stats_every = 100;
int rulestat_on = 1;
int advanced_on = 1;
int file_on = 0;
int autofile_on = 1;
/*const*/ int autofile_every = 100;
//const char *autofile_every_string = "100";
int grid_on = 1;
int memjog_on = 0;
const int memprof__override__rulestat_every = 100;
int stopat = 0;
#endif  //memprof

int* memjog_array [100];

Display *X_main_display;
Window *X_world_window;
GC X_world_window_pen;
GC X_world_window_pen_blue;
GC X_world_window_pen_red;
GC X_world_window_pen_green;
GC X_world_window_clear_pen;
int bxl,byt,bxr,byb; // border

//long int GA_time initialised and incremented in geb_core
long int old_GA_time;
char GA_time_string[17];
const int GA_time_text_height = 10;
void show_GA_time()
{ if (!display) return;
  sprintf(GA_time_string,"%ld",GA_time);
  XDrawImageString(X_main_display,*X_world_window,X_world_window_pen,100,GA_time_text_height,
      GA_time_string,strlen(GA_time_string));
}

int world_window_x_step_size, world_window_y_step_size;

void show_grid()
{ if (!display) return;
  int _bxl = bxl + (world_window_x_step_size>>1); int _bxr = bxr - (world_window_x_step_size>>1);
  int _byt = byt + (world_window_y_step_size>>1); int _byb = byb - (world_window_y_step_size>>1);
  for (int n=0;n<=world_length;n++) 
  { XDrawLine(X_main_display,*X_world_window,X_world_window_pen,_bxl,_byt+n*world_window_y_step_size,_bxr,_byt+n*world_window_y_step_size);
    XDrawLine(X_main_display,*X_world_window,X_world_window_pen,_bxl+n*world_window_x_step_size,_byt,_bxl+n*world_window_x_step_size,_byb);
  }
}

void output_individual(int at_x_64,int at_y_64,int direction_deg,GC X_pen)
{ if (!display) return;
  int x = ((at_x_64 * world_window_x_step_size)>>6) + bxl;
  int y = ((at_y_64 * world_window_y_step_size)>>6) + byt;
  x += ((world_window_x_step_size>>2)*sindeg64[direction_deg])/(1<<6);
  y -= ((world_window_y_step_size>>2)*cosdeg64[direction_deg])/(1<<6);
  XFillArc(X_main_display,*X_world_window,X_pen,x,y,
           world_window_x_step_size,world_window_y_step_size,
           (260-direction_deg)<<6,1280);
   //270-10=260         (10*2)<<6=1280  [1<<6=64]
}

void show_individual(int at_x_64,int at_y_64,int direction_deg,int pen_colour)
{ GC X_pen;
  switch (pen_colour)
  { case 1 : X_pen = X_world_window_pen_blue;  break;
    case 2 : X_pen = X_world_window_pen_red;   break;
    case 3 : X_pen = X_world_window_pen_green; break;
    default: X_pen = X_world_window_pen;       break;
  }
  output_individual(at_x_64,at_y_64,direction_deg,X_pen);
}

void unshow_individual(int at_x_64,int at_y_64,int direction_deg)
{ output_individual(at_x_64,at_y_64,direction_deg,X_world_window_clear_pen);
}

void draw_world()
{ if (!display) return;
  XDrawImageString(X_main_display,*X_world_window,X_world_window_pen,0,10,
      "Time:",5);
  show_GA_time();
  XDrawLine(X_main_display,*X_world_window,X_world_window_pen,bxl,byt,bxr,byt);
  XDrawLine(X_main_display,*X_world_window,X_world_window_pen,bxr,byt,bxr,byb);
  XDrawLine(X_main_display,*X_world_window,X_world_window_pen,bxr,byb,bxl,byb);
  XDrawLine(X_main_display,*X_world_window,X_world_window_pen,bxl,byb,bxl,byt);
  for (int at=0;at<world_size;at++)
    if (world[at])
      show_individual(world[at]->at_x_64,world[at]->at_y_64,
                      world[at]->direction_deg,world[at]->pen_colour);
  if (grid_on) show_grid();
}

int create_world_window(int argc,const char *argv1,char *argv2) //[-display [DISPLAY]]
{ number_of_ann_windows_open = 0;//always world_window created on own
  X_main_display = 0;
  X_world_window = 0;
  int err=X_create_window(X_main_display,X_world_window,X_world_window,
                          X_world_window_pen,
                          "GEB: World Window","geb",100,100,400,300,
#ifdef  detect_key_presses
                          KeyPressMask|
#endif  //detect_key_presses
                          ButtonPressMask|ExposureMask|StructureNotifyMask,
                          argc,argv1,argv2);
  if (err) display=0;
  //if (err)
  //{ fprintf(stderr,"No display - exiting\n");
  //  fprintf(stderr,"Try specifying a display using the -display [DISPLAY] option\n");
  //  fprintf(stderr,"Or you can set the environment variable DISPLAY (to eg. :0)\n"); fflush(stderr);
  //  exit(103);
  //}
  //if (err) {fprintf(stderr,"Error %d when creating main window\n",err); fflush(stderr);}
  else
  { X_world_window_clear_pen = XCreateGC(X_main_display,*X_world_window,0,0);
    XGCValues temp_XGCValues;
    //XGetGCValues(X_main_display,X_world_window_pen,GCBackground,&temp_XGCValues);
    XGetGCValues(X_main_display,X_world_window_pen,GCForeground|GCBackground,&temp_XGCValues);
    unsigned long X_background = temp_XGCValues.background;
    XSetForeground(X_main_display,X_world_window_clear_pen,X_background);
    XSetBackground(X_main_display,X_world_window_clear_pen,X_background);
    X_world_window_pen_blue  = XCreateGC(X_main_display,*X_world_window,0,0);
    X_world_window_pen_red   = XCreateGC(X_main_display,*X_world_window,0,0);
    X_world_window_pen_green = XCreateGC(X_main_display,*X_world_window,0,0);
    XSetBackground(X_main_display,X_world_window_pen_blue, X_background);
    XSetBackground(X_main_display,X_world_window_pen_red,  X_background);
    XSetBackground(X_main_display,X_world_window_pen_green,X_background);
    Colormap X_cmap = DefaultColormap(X_main_display,DefaultScreen(X_main_display));
    XColor X_blueF;  X_blueF.red =10000; X_blueF.green =10000; X_blueF.blue =65535;
    XColor X_redF;   X_redF.red  =65535; X_redF.green  =10000; X_redF.blue  =10000;
    XColor X_greenF; X_greenF.red=10000; X_greenF.green=65535; X_greenF.blue=10000;
    if (XAllocColor(X_main_display,X_cmap,&X_blueF))
      XSetForeground(X_main_display,X_world_window_pen_blue,X_blueF.pixel);
    else XSetForeground(X_main_display,X_world_window_pen_blue,temp_XGCValues.foreground);
    if (XAllocColor(X_main_display,X_cmap,&X_redF))
      XSetForeground(X_main_display,X_world_window_pen_red,X_redF.pixel);
    else XSetForeground(X_main_display,X_world_window_pen_red,temp_XGCValues.foreground);
    if (XAllocColor(X_main_display,X_cmap,&X_greenF))
      XSetForeground(X_main_display,X_world_window_pen_green,X_greenF.pixel);
    else XSetForeground(X_main_display,X_world_window_pen_green,temp_XGCValues.foreground);
  }
  return err;
}

#ifndef disable_signals
int new_display_interrupt_received = 0;//not yet

void new_display_interrupt(int sig)
{ new_display_interrupt_received = 1;
}

void new_display()
{ new_display_interrupt_received = 0;//about to process it
  char* display_name_file = new char[1+strlen(display_name_file_start)+10]; char pid[10];
  strcpy(display_name_file,display_name_file_start);
  sprintf(pid,"%ld",(unsigned long)getpid());
  strncat(display_name_file,pid,79-strlen(pid));
  char *new_display;
  char use_display[81];
  FILE *fp;
  if (  (!(fp = fopen(display_name_file,"r")))
      ||(!fgets(use_display,80,fp))
      ||(fclose(fp)))
  { fprintf(stderr,"Error reading %s\n",display_name_file); fflush(stderr);
    delete [] display_name_file; return;
  }
  else remove(display_name_file);
  if (*use_display == ' ')
  { if (messages_on) {fprintf(stdout,"\nResetting to use no display\n"); fflush(stdout);}
    new_display = 0;
    number_of_ann_windows_open = 0;//no ann_windows without world window
  }
  else
  { if (messages_on) {fprintf(stdout,"\nResetting to use display %s\n",use_display); fflush(stdout);}
    new_display = use_display;
  }
  if (X_main_display)
  { if (X_world_window) XDestroyWindow(X_main_display,*X_world_window);
    XCloseDisplay(X_main_display);
  }
  X_main_display = 0;
  X_world_window = 0;
  int err = (new_display && !strcmp(new_display,"$DISPLAY"))
          ? create_world_window(1,0,0) : create_world_window(3,"-display",new_display);
  if (err && messages_on) {fprintf(stderr,"\nError when trying to reset display\n"); fflush(stderr);}
  delete [] display_name_file;
}

void recognise_new_display_interrupt()
{
#ifndef _LINUX_SIGNAL_H
  sigset(SIGUSR1,new_display_interrupt);
#else   //_LINUX_SIGNAL_H
  struct sigaction action;
  action.sa_handler = new_display_interrupt;
  sigset_t dummy_set; sigemptyset(&dummy_set); action.sa_mask = dummy_set;
  action.sa_flags = SA_RESTART;
  action.sa_restorer = NULL;
  sigaction(SIGUSR1,&action,NULL);
#endif  //_LINUX_SIGNAL_H
}

void silentTerminate_signal_interrupt(int sig)
{ fprintf(stderr,"Exiting because signal %d caught\n",sig); fflush(stderr);
  ga_terminate();
  exit(128+sig);
}

void recognise_silentTerminate_signals()
{ // These are the signals that by default terminate a process without dumping core:
  // except SIGKILL (which, like SIGSTOP, cannot be caught), SIGUSR1 (used) and SIGLOST (absent)
#ifndef _LINUX_SIGNAL_H
  const int num_silentTerminate_signals = 11;
  int silentTerminate_signals[num_silentTerminate_signals]
    = { SIGHUP, SIGINT, SIGUSR2, SIGPIPE, SIGALRM, SIGTERM, SIGVTALRM, SIGPROF, SIGIO, SIGPWR, SIGPOLL}; //may not be complete
#else   //_LINUX_SIGNAL_H
  const int num_silentTerminate_signals = 12; // 13;
  int silentTerminate_signals[num_silentTerminate_signals]
    = { SIGHUP, SIGINT, SIGUSR2, SIGPIPE, SIGALRM, SIGTERM, SIGVTALRM, SIGPROF, SIGIO, SIGPWR, SIGPOLL, SIGSTKFLT}; // , SIGUNUSED};
#endif  //_LINUX_SIGNAL_H
  int silentTerminate_signal_num;
  for (silentTerminate_signal_num=0;silentTerminate_signal_num<num_silentTerminate_signals;silentTerminate_signal_num++)
  { int sig = silentTerminate_signals[silentTerminate_signal_num];
#ifndef _LINUX_SIGNAL_H
    sigset(sig,silentTerminate_signal_interrupt);
#else   //_LINUX_SIGNAL_H
    struct sigaction action;
    action.sa_handler = silentTerminate_signal_interrupt;
    sigset_t dummy_set; sigemptyset(&dummy_set); action.sa_mask = dummy_set;
    action.sa_flags = SA_RESTART;
    action.sa_restorer = NULL;
    sigaction(sig,&action,NULL);
#endif  //_LINUX_SIGNAL_H
  }
}

void send_new_display_interrupt(char *pid,const char *new_display)
{ char* display_name_file = new char[1+strlen(display_name_file_start)+10];
  strcpy(display_name_file,display_name_file_start);
  strncat(display_name_file,pid,79-strlen(pid));
  FILE *fp;
  if (!new_display) new_display = " ";
  if (  (!(fp = fopen(display_name_file,"w")))
      ||(!fputs(new_display,fp))
      ||(fclose(fp))
     )
  { fprintf(stderr,"Error writing %s\n",display_name_file); fflush(stderr);
    exit(104);
  }
#ifndef _LINUX_SIGNAL_H
  int err = sigsend(P_PID,(id_t)atol(pid),SIGUSR1);
#else   //_LINUX_SIGNAL_H
  int err = kill((pid_t)atol(pid),SIGUSR1);
#endif  //_LINUX_SIGNAL_H
  if (err)
  { fprintf(stderr,"Error in sending new display request signal\n"); fflush(stderr);
    exit(105);
  }
  delete [] display_name_file;
}
#endif  //disable_signals

void geb_terminate(int graphics_need_terminating = 1,int doexit=1)
{ ga_terminate();
  if (rulestat_on) geb_rulestat_terminate(doexit);
  if (graphics_need_terminating)
  { XFreeGC(X_main_display,X_world_window_pen);
    XDestroyWindow(X_main_display,*X_world_window);
    XCloseDisplay(X_main_display);
    delete X_world_window;
  }
  if (!doexit) return;
  // if (messages_on) {fprintf(stdout,"\nProcess Terminated\n"); fflush(stdout);}
  fprintf(stderr,"\nProcess Terminated\n"); fflush(stderr);
  exit(100);
}

//int X_ioerrorhandler(Display *X_error_display) {geb_terminate(0);}
//So as to avoid error such as "non-void function X_ioerrorhandler should return a value"
int X_ioerrorhandler(Display *X_error_display) {geb_terminate(0);return 0;}

void output_simple_functionality_message()
{ fprintf(stdout,"%s","\nSimple functionality:\n");
  fprintf(stdout,"%s","=====================\n");
#ifdef  display_delay_with_f_and_s_keys
  fprintf(stdout,     "With World Window selected: S-key = Slower , F-key = Faster\n");
#endif  //display_delay_with_f_and_s_keys
  fprintf(stdout,"%s","Click on an organism to display its network and pause the program\n");
  fprintf(stdout,"%s","Then: Left-click on a network's nodes to drag and drop them\n");
  fprintf(stdout,"%s","      Middle/Right-click on a network window to remove it\n"); fflush(stdout);
}

void output_advanced_functionality_message()
{ fprintf(stdout,"%s","\nAdvanced functionality:\n");
  fprintf(stdout,"%s","=======================\n\n");
  fprintf(stdout,"%s","In the World Window:\n");
#ifdef  display_delay_with_f_and_s_keys
  fprintf(stdout,     "    S-key = Slower , F-key = Faster. (The World Window must be selected.)\n");
#endif  //display_delay_with_f_and_s_keys
  fprintf(stdout,"%s","    Left-click within  the world box pauses the program.  If the (mouse)\n");
  fprintf(stdout,"%s","               pointer is over an organism, its network will be displayed.\n");
  fprintf(stdout,"%s","               If not, it should now be easy to click on an organism.\n");
  fprintf(stdout,"%s","    Left-click outside the world box will continue the program.  \n");
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
  fprintf(stdout,"%s","In a Network Window: As with simple functionality:\n");
  fprintf(stdout,"%s","    Left-click on a network's nodes to drag and drop them\n");
  fprintf(stdout,"%s","    Middle/Right-click on a network window to remove it\n"); fflush(stdout);
}

void usage(int argc,char *argv[])
{ if (argc==1)
  { output_simple_functionality_message();
    fprintf(stdout,"%s%s%s","Try ",argv[0]," -help for information on arguments\n"); fflush(stdout);
  }
  int display_argc=1; char *display_argv1=0; char *display_argv2=0; //[-display [DISPLAY]]
  char *pid=0;
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
    else if (!strcmp(argv[arg],"-display"))
         { if (((arg+1)==argc) || (argv[arg+1][0] == '-'))
           { display_argc=2; display_argv1=argv[arg]; display_argv2=0;
             arg++;
           }
           else
           { display_argc=3; display_argv1=argv[arg]; display_argv2=argv[arg+1];
             if (!strcmp(display_argv2,"blank"))
             { display_argc=2; display_argv2=0;
             }
             arg+=2;
           }
         }
    else if (!strcmp(argv[arg],"-pid") && ((arg+1)<argc) && atol(argv[arg+1]))
         { pid = argv[arg+1];
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
      fprintf(stderr,"%s"    ," -display  DISPLAY      = X display (default is $DISPLAY).  Can be blank\n");
      fprintf(stderr,"%s"    ," -pid      PID          = process id.  For display redirection during a run\n");
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
      fprintf(stderr,"%s%s%s","Hint: use ",argv[0]," [arguments] &      ,to run\n");
      fprintf(stderr,"%s%s%s","          ",argv[0]," -pid PID -display  ,before logging out\n");
      fprintf(stderr,"%s%s%s","          ",argv[0]," -pid PID           ,after  logging back in\n"); fflush(stderr);
      exit(0);
    }
  }
#ifndef disable_signals
  if (pid)
  { if (display_argv1) send_new_display_interrupt(pid,display_argv2);
    else               send_new_display_interrupt(pid,"$DISPLAY");
    exit(0);
  }
#endif  //disable_signals
  XSetIOErrorHandler(X_ioerrorhandler);
  /* int err = */ create_world_window(display_argc,display_argv1,display_argv2);
}

void initialise(int argc,char *argv[])
{ fprintf(stderr,"Geb version %s. Copyright (C) Alastair Channon.\n",geb_version);
  usage(argc,argv);
#ifndef disable_signals
  recognise_new_display_interrupt();
  recognise_silentTerminate_signals();
#else   //disable_signals
  halted_display = 0;
#endif  //disable_signals
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
  draw_world();
}

int process_geb_xwin_events(XEvent &X_event)
{ int not_done = 1;
  switch (X_event.type)
  { case Expose:
      if (X_event.xexpose.count == 0) draw_world();
      break; 
    case ConfigureNotify:
      XClearWindow(X_main_display,*X_world_window);
      world_window_x_step_size = X_event.xconfigure.width / (world_length+2);
      world_window_y_step_size =
          (X_event.xconfigure.height-GA_time_text_height) / (world_length+2);
      bxl=((X_event.xconfigure.width - (world_length+1)*world_window_x_step_size)>>1); //border: x left
      byt=GA_time_text_height+((X_event.xconfigure.height - GA_time_text_height
                                     - (world_length+1)*world_window_y_step_size)>>1); //border: y top
      bxr=bxl+(world_length+1)*world_window_x_step_size;                               //border: x right
      byb=byt+(world_length+1)*world_window_y_step_size;                               //border: y bottom
      draw_world();
      break;
    case MappingNotify:
      XRefreshKeyboardMapping(&(X_event.xmapping));
      break;
    case ButtonPress:
      if (advanced_on && (X_event.xbutton.button != 1))
      { if ((X_event.xbutton.y>=byt)&&(X_event.xbutton.y<=byb)&&(X_event.xbutton.x>=bxl)&&(X_event.xbutton.x<=bxr))
        { int err = geb_file__write(geb_file__write__name);
          //note: rulestat components now included in snapshot if rulestat_on - writing called from within geb_file__write
          if (!err && messages_on) {fprintf(stdout,"%s%s%s","Snapshot written to file ",geb_file__write__name,"\n"); fflush(stdout);}
          break;
        }
        //else:
#ifndef gprofile
#else   //gprofile
        not_done = 0; //set to terminate program
#endif  //gprofile
#ifndef disable_signals
        char pid[20];
        sprintf(pid,"%ld",(long)getpid());
        send_new_display_interrupt(pid," ");
#else   //disable_signals
        if (display)
        { halted_display = display;
          display = 0;
          if (messages_on) {fprintf(stdout,"Display (only) temporarily Halted by user (for faster running)\n"); fflush(stdout);}
        }
        else
        { display = halted_display;
          halted_display = 0;
          XClearWindow(X_main_display,*X_world_window);
          draw_world();
          if (messages_on) {fprintf(stdout,"Display UnHalted by user\n"); fflush(stdout);}
        }
#endif  //disable_signals
        break;
      }
      { int previous_number_of_ann_windows_open = number_of_ann_windows_open;
        if (number_of_ann_windows_open == -1) number_of_ann_windows_open = 0; // unpause
        if (number_of_ann_windows_open == max_num_ann_windows) break;
        //int xb = (X_event.xbutton.x - (world_window_x_step_size>>1) - bxl) / world_window_x_step_size;
        //int yb = (X_event.xbutton.y - (world_window_y_step_size>>1) - byt) / world_window_y_step_size;
        //changed because want eg. -0.5 to round down to -1, not up to 0:
        int xb = -1+ (X_event.xbutton.x + (world_window_x_step_size>>1) - bxl) / world_window_x_step_size;
        int yb = -1+ (X_event.xbutton.y + (world_window_y_step_size>>1) - byt) / world_window_y_step_size;
        Organism *best_org=0;
        double best_distance=world_window_x_step_size<world_window_y_step_size?
                             world_window_x_step_size/3:world_window_y_step_size/3;
        for (int x_search=(xb<=0?0:xb-1);x_search<(xb>=world_length?world_length:xb+1);x_search++)
          for (int y_search=(yb<=0?0:yb-1);y_search<(yb>=world_length?world_length:yb+1);y_search++)
          { Organism *org = world[x_search + y_search * world_length];
            if (!org) continue;
            double delta_x = ((org->at_x_64 * world_window_x_step_size)>>6) + bxl //see output_individual
                             - (X_event.xbutton.x - (world_window_x_step_size>>1));
            double delta_y = ((org->at_y_64 * world_window_y_step_size)>>6) + byt //see output_individual
                             - (X_event.xbutton.y - (world_window_y_step_size>>1));
            double distance = sqrt( (delta_x*delta_x) + (delta_y*delta_y) );
            if (distance < best_distance)
            { best_org = org;
              best_distance = distance;
            }
          }
        if (best_org)
        { /* Ann_Window *a_new_Ann_Window = */ new Ann_Window(best_org,X_main_display,X_world_window);
          if (messages_on) {fprintf(stdout,"ORG: at_x=%d	at_y=%d	deg=%d	age=%d\n",best_org->at_x,best_org->at_y,best_org->direction_deg,best_org->stats__age); fflush(stdout);}
        }
        else if (advanced_on)
        { if (((X_event.xbutton.y>=byt)&&(X_event.xbutton.y<=byb)&&(X_event.xbutton.x>=bxl)&&(X_event.xbutton.x<=bxr))
              &&(number_of_ann_windows_open==0))
          { if (previous_number_of_ann_windows_open == 0) if (messages_on) {fprintf(stdout,"Paused\n"); fflush(stdout);}
            number_of_ann_windows_open = -1; //pause
          }
          else if (((X_event.xbutton.y<byt)||(X_event.xbutton.y>byb)||(X_event.xbutton.x<bxl)||(X_event.xbutton.x>bxr))
                   &&(previous_number_of_ann_windows_open==-1)) if (messages_on) {fprintf(stdout,"UnPaused\n"); fflush(stdout);}
        }
      }
      break;
#ifdef  detect_key_presses
    case KeyPress:
      KeySym tmp_keysym;
      char tmp_text[5];
      if (XLookupString(&(X_event.xkey),tmp_text,5,&tmp_keysym,0))
        switch (tmp_text[0])
        {
#ifdef  display_delay_with_f_and_s_keys
          case 'F':
          case 'f': // faster
            if (display_delay_level==0) break;
            display_delay_level--;
            if (messages_on) {fprintf(stdout,"Faster: display_delay_level now %d\n",display_delay_level); fflush(stdout);}
            break;
          case 'S':
          case 's': // slower
            if (display_delay_level==MAX_DISPLAY_DELAY_LEVEL) break;
            display_delay_level++;
            if (messages_on) {fprintf(stdout,"Slower: display_delay_level now %d\n",display_delay_level); fflush(stdout);}
            break;
#ifdef  memprof
          case 'P':
          case 'p': // pause long enough for Gnome memprof user to press "Profile" and "Leaks" buttons
            struct timespec delay  = {0,999999999L}; // one second
            for (int t=0;t<memprof__pause_in_seconds;t++) nanosleep(&delay,NULL);
            break;
#endif  //memprof
#endif  //display_delay_with_f_and_s_keys
        }
      break;
#endif  //detect_key_presses
  }
  return not_done;
}

void geb_stats();    //forward declaration
void geb_autofile(); //forward declaration

int main(int argc,char *argv[])
{ initialise(argc,argv);
  //
  XEvent X_event;
  int not_done = 1;
  while (not_done)
  { 
#ifndef disable_signals
    if (new_display_interrupt_received) new_display();
    if (   (   !display
            ||
#else   //disable_signals
    if (   (
#endif  //disable_signals
#ifndef gprofile_long
               !XEventsQueued(X_main_display,QueuedAfterFlush)
#else   //gprofile_long
               !XEventsQueued(X_main_display,QueuedAlready)
               // QueuedAfterReading and QueuedAfterFlush cause pauses when compiled with -pg
#endif  //gprofile_long
           )
        && (number_of_ann_windows_open == 0)
       )
    { old_GA_time = GA_time;
      ga_main_loop();
      if (GA_time != old_GA_time)
      { show_GA_time();
        old_GA_time = GA_time;
        if (grid_on) show_grid();
        if (display) XFlush(X_main_display);
        if (stats_on && (!(GA_time%stats_every))) geb_stats();
#ifndef memprof
        if (rulestat_on && (!(GA_time%rulestat_every))) geb_rulestat();
#else   //memprof
        if (!(GA_time%memprof__override__rulestat_every)) geb_rulestat();
#endif  //memprof
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
#ifdef  display_delay_with_f_and_s_keys
        if (display && display_delay_level)
        { struct timespec delay  = {0,MIN_DISPLAY_DELAY_NANOSEC<<(display_delay_level-1)}; // in nanoseconds
          nanosleep(&delay,NULL);
        }
#endif  //display_delay_with_f_and_s_keys
        if (memjog_on)
        { if (GA_time%2) for (int s=0;s<100;s++) delete [] memjog_array[s];
          else           for (int s=0;s<100;s++) memjog_array[s] = new int [s];
        }
        if (stopat && (GA_time==stopat)) not_done=0;
      }
    }
    else
    { XNextEvent(X_main_display,&X_event);
      if (!number_of_ann_windows_open || (X_event.xany.window == *X_world_window))
      { if (grid_on) show_grid();
        not_done = process_geb_xwin_events(X_event);
      }
      else process_ann_xwin_events(X_event);
    }
  }
  //
  if (display) geb_terminate(1);
  else         geb_terminate(0);
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
  fprintf(stdout,"time: T%ldT	average_age: A%fA	oldest: O%dO	youngest: Y%dY number: N%dN	number of reproductions: R%ldR	blacK%dK	B%dBlue	reD%dD	G%dGreen\n",GA_time,(double)age_total/num_individs,oldest,youngest,num_individs,stats__num_reproductions,pen_colours_total[0],pen_colours_total[2],pen_colours_total[3],pen_colours_total[4]); fflush(stdout);
  fprintf(stdout,"time: S%ldS	SHADOWstats__num_individuals %d\n",GA_time,SHADOWstats__num_individuals); fflush(stdout);
  fprintf(stdout,"time: L%ldL	mean_genotype_length %d SHADOWmean_genotype_length %d\n",GA_time,total_genome_lengths/num_individs,SHADOWtotal_genome_lengths/SHADOWstats__num_individuals); fflush(stdout);
  if (num_individs<2) {printf("Terminating because number of individuals = %d\n",num_individs); fflush(stdout); exit(108);}
}

void geb_autofile()
{ int err;
  int need_to_remove_renamed_file = 0;
  char *rename_file=0;
  FILE *fp;
  if ((fp = fopen(geb_file__write__name,"r")))
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
