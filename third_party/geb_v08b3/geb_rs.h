/* Copyright (c) 2001 Alastair Channon
* 
* Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
* 
* The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
* 
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

/* geb_rs.h */
#ifndef geb_rs_h
#define geb_rs_h
#include <stdio.h>

#define rulestat_every_1000
#ifndef rulestat_every_1000
#warning NOT USING rulestat_every_1000
#else   //rulestat_every_1000
//geb_v06__default: #warning USING rulestat_every_1000
#endif  //rulestat_every_1000

extern int rulestat_on;
#ifndef rulestat_every_1000
const int rulestat_every = 100;
#else   //rulestat_every_1000
const int rulestat_every = 1000;
#endif  //rulestat_every_1000

void geb_rulestat_initialise();
void geb_rulestat_terminate(int close_rulestat_files = 1);
void geb_rulestat();

void geb_rulestat__write_snapshot(FILE *file);
void geb_rulestat__read_snapshot_and_initialise(FILE *file);

#endif
