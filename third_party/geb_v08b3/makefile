#makefile for Geb

#without -ffloat-store, two runs of the same binary on the same machine with the same (random number generator) seed can produce different results
CC  = g++ -O3 -ffloat-store -Wall -Wno-uninitialized -Wno-unused
#CC  = g++ -O3 -ffloat-store -Wall -Wno-uninitialized -Wno-unused -pg
#CC  = g++ -O3 -ffloat-store -Wall -Wno-uninitialized -Wno-unused -g
#flag -mieee-fp not necessary (?): CC  = g++ -ffloat-store -mieee-fp ...

#MACHINETYPE = ${HOSTTYPE}
MACHINETYPE = `uname -m`

include makefilestart

CCc = $(CCpath)$(CC) $(CCdefs) $(CCc_include_dir) -c
#CCl = $(CCpath)$(CC) $(CCl_lib_dir) -lX11 -lm
CCl = $(CCpath)$(CC) $(CCl_lib_dir)
CCl_tail_options += -lX11 -lm

o = o.`uname`.$(MACHINETYPE)
runbin_resets = geb_$(code_version).resets.$(OS).$(MACHINETYPE).bin
runbin_no_resets = geb_$(code_version).no_resets.$(OS).$(MACHINETYPE).bin
stats_xmgr_parameters_file = xmgr_geb.par

#

run_in_o:	
	@	if [ ! -d $(o) ] ;then mkdir $(o) ;fi
	@	cd $(o) ; \
			if [ ! -f makefilestart ] ;then \
			  if [ -f ../makefilestart.`uname` ] ;then \
			    ln -s ../makefilestart.`uname` makefilestart ; \
			  else \
			    ln -s ../makefilestart makefilestart ; \
			  fi; \
			fi; \
			if [ ! -f makefile_rs ] ;then \
			  ln -s ../makefile_rs makefile_rs ; \
			fi; \
			make -f ../makefile makelist

makelist	:	geb_resets geb_no_resets link_geb_binary_resets link_geb_binary_no_resets
			

and		:	run_in_o
		

viewstats	:
		fgrep 00T geb.out | awk -F\[A-Z\] \
			'{print $$2" "$$4" "$$6" "$$8" "$$10" "$$12" "$$14" "$$16" "$$18" "$$20" "$$22}' \
			| xmgr -eps -nxy -source stdin -param $(stats_xmgr_parameters_file)
#old:		egrep ^time: ......

#================================================================================

##Either (default) define empty 'cleanallrulestats' and don't include makefile_rs, for compatibility: 
cleanallrulestats	:	
			@
##Or include makefile_rs, for ease of use (removing need to type 'make -f makefile_rs ...'):
#include makefile_rs

#================================================================================

clean	:
		if [ -d $(o) ] ;then rm -rf $(o) ;fi

cleanall	: cleanallrulestats
		if [ -d $(o) ] ;then rm -rf $(o) ;fi
#		if [ -f $(runbin) ] ;then rm -f $(runbin) ;fi
#		if [ -f geb_v*\.bin ] ;then rm -f geb_v*\.bin ;fi
		rm -f geb_v*\.bin

#

link_geb_binary_resets	:	geb_resets
		@	runbin=$(runbin_resets); if [ `fgrep 'const char *geb_version = ' ../geb_xver.h | \
				awk -F\" '{print $$2}' | awk -F\( '{print "v"$$1}'` != \
				`echo $$runbin | awk -F_ '{print $$2}' | \
				awk -F\. '{print $$1}'` ] ;then \
				echo Warning: Version numbers do not match ;fi
		@	if [ -f ../$(runbin_resets) ] ;then rm -f ../$(runbin_resets) ;fi
			mv -f geb_resets ../$(runbin_resets) ; cp -p ../$(runbin_resets) geb_resets
#got fed up of ln mucking up
#			ln ../$(runbin_resets) geb_resets

link_geb_binary_no_resets	:	geb_no_resets
		@	runbin=$(runbin_no_resets); if [ `fgrep 'const char *geb_version = ' ../geb_xver.h | \
				awk -F\" '{print $$2}' | awk -F\( '{print "v"$$1}'` != \
				`echo $$runbin | awk -F_ '{print $$2}' | \
				awk -F\. '{print $$1}'` ] ;then \
				echo Warning: Version numbers do not match ;fi
		@	if [ -f ../$(runbin_no_resets) ] ;then rm -f ../$(runbin_no_resets) ;fi
			mv -f geb_no_resets ../$(runbin_no_resets) ; cp -p ../$(runbin_no_resets) geb_no_resets
#got fed up of ln mucking up
#			ln ../$(runbin_no_resets) geb_no_resets

#

geb_resets	:	rnd_core.o ann_core.o geb_core.o geb_file.o geb_rs_resets.o win_xwin.o ann_xwin.o geb_xwin.o
	$(CCl) -o geb_resets \
		rnd_core.o ann_core.o geb_core.o geb_file.o geb_rs_resets.o win_xwin.o ann_xwin.o geb_xwin.o \
		$(CCl_tail_options)

geb_no_resets	:	rnd_core.o ann_core.o geb_core.o geb_file.o geb_rs_no_resets.o win_xwin.o ann_xwin.o geb_xwin.o
	$(CCl) -o geb_no_resets \
		rnd_core.o ann_core.o geb_core.o geb_file.o geb_rs_no_resets.o win_xwin.o ann_xwin.o geb_xwin.o \
		$(CCl_tail_options)

geb_xwin.o	:	../geb_xwin.cpp ../geb_xver.h ../win_xwin.h ../ann_xwin.h ../geb_core.h ../geb_file.h ../geb_rs.h
			$(CCc) ../geb_xwin.cpp

ann_xwin.o	:	../ann_xwin.cpp ../win_xwin.h ../ann_xwin.h
			$(CCc) ../ann_xwin.cpp

../ann_xwin.h	:	../ann_core.h
			touch ../ann_xwin.h

win_xwin.o	:	../win_xwin.cpp ../win_xwin.h
			$(CCc) ../win_xwin.cpp

geb_file.o	:	../geb_file.cpp ../geb_file.h ../geb_core.h
			$(CCc) ../geb_file.cpp

geb_rs_resets.o	:	../geb_rs.cpp ../geb_rs.h ../geb_core.h
			$(CCc) \
			-Dreset_shadow_to_real_for_each_pass ../geb_rs.cpp -o geb_rs_resets.o

geb_rs_no_resets.o	:	../geb_rs.cpp ../geb_rs.h ../geb_core.h
			$(CCc) \
			-Ddont_reset_shadow_to_real_for_each_pass ../geb_rs.cpp -o geb_rs_no_resets.o

geb_core.o	:	../geb_core.cpp ../geb_core.h
			$(CCc) ../geb_core.cpp

../geb_core.h	:	../ann_core.h
			touch ../geb_core.h

ann_core.o	:	../ann_core.cpp ../ann_core.h
			$(CCc) ../ann_core.cpp

../ann_core.h	:	../rnd_core.h
			touch ../ann_core.h

rnd_core.o	:	../rnd_core.cpp ../rnd_core.h
			$(CCc) ../rnd_core.cpp
