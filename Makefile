#
# Makefile for Formalizer environment
# (by Randal A. Koene, randalk@minduploading.org)
#

SHELL=/usr/bin/bash

# +----- begin: Formalizer source directory structure -----+
FORMALIZERPATH=$(HOME)/src/formalizer
COREINCLUDEPATH=$(FORMALIZERPATH)/core/include
COREPATH=$(FORMALIZERPATH)/core
TOOLSPATH=$(FORMALIZERPATH)/tools
MEDIAPATH=$(FORMALIZERPATH)/media
FORMALIZERPATH_THIS=$(FORMALIZERPATH)
FZCONFIGSRCPATH=$(FORMALIZERPATH)/config
FZCONFIGPATH=$(HOME)/.formalizer
LOCALBINDIR=$(HOME)/local/bin

#TEMPLATEDIR = -DDEFAULT_TEMPLATE_DIR=\"$(FORMALIZERPATH_THIS)\" 

EXEDIR=$(HOME)/.formalizer/bin
CGIDIR=/usr/lib/cgi-bin
W3MCGIDIR=/usr/lib/w3m/cgi-bin
WEBBASEDIR=/var/www/html
WEBINTERFACESDIR=$(WEBBASEDIR)/formalizer

INC=$(COREINCLUDEPATH)
OBJ=./obj
TEST=./test
# +----- end  : Formalizer source directory structure -----+

# +----- begin: Select Formalizer compilables -----+
LIBCOMPDIRS = $(COREPATH)/lib

CORECOMPDIRS = $(COREPATH)/fzguide.system
CORECOMPDIRS = $(COREPATH)/fzlog
CORECOMPDIRS = $(COREPATH)/fzgraph
CORECOMPDIRS += $(COREPATH)/fzquerypq
CORECOMPDIRS += $(COREPATH)/fzserverpq

TOOLSCOMPDIRS = $(TOOLSPATH)/conversion/dil2graph
TOOLSCOMPDIRS += $(TOOLSPATH)/conversion/graph2dil
TOOLSCOMPDIRS += $(TOOLSPATH)/dev/boilerplate
TOOLSCOMPDIRS += $(TOOLSPATH)/interface/fzloghtml
TOOLSCOMPDIRS += $(TOOLSPATH)/interface/fzgraphhtml
TOOLSCOMPDIRS += $(TOOLSPATH)/interface/nodeboard

COMPILABLES = $(LIBCOMPDIRS) $(CORECOMPDIRS) $(TOOLSCOMPDIRS)
# +----- end  : Select Formalizer compilables -----+

# +----- begin: Select Formalizer executables -----+
EXECUTABLES =
EXECUTABLES += $(COREPATH)/fzbackup/fzbackup.py
EXECUTABLES += $(COREPATH)/fzbackup/fzbackup-mirror-to-github.sh
EXECUTABLES += $(COREPATH)/fzedit/fzedit
EXECUTABLES += $(COREPATH)/fzgraph/fzgraph
EXECUTABLES += $(COREPATH)/fzgraphsearch/fzgraphsearch
EXECUTABLES += $(COREPATH)/fzguide.system/fzguide.system
EXECUTABLES += $(COREPATH)/fzinfo/fzinfo.py
EXECUTABLES += $(COREPATH)/fzbackup/fzlist_backups.sh
EXECUTABLES += $(COREPATH)/fzlog/fzlog
EXECUTABLES += $(COREPATH)/fzquerypq/fzquerypq
EXECUTABLES += $(COREPATH)/fzbackup/fzrestore.sh
EXECUTABLES += $(COREPATH)/fzserverpq/fzserverpq
EXECUTABLES += $(COREPATH)/fzserverpq/fzserverpqd.sh
# EXECUTABLES += $(COREPATH)/fzserverpq/fzserverpq-graph
# EXECUTABLES += $(COREPATH)/fzserverpq/fzserverpq-log
# EXECUTABLES += $(COREPATH)/fzserverpq/fzserverpq-metrics
EXECUTABLES += $(COREPATH)/fzsetup/fzsetup.py
# EXECUTABLES += $(COREPATH)/fzshift/fzshift
EXECUTABLES += $(COREPATH)/fztask/fztask.py
EXECUTABLES += $(COREPATH)/fztask-server/fztask-server.py
EXECUTABLES += $(COREPATH)/fztask-server/fztask-serverd.sh
EXECUTABLES += $(COREPATH)/fzupdate/fzupdate

# EXECUTABLES += $(TOOLSPATH)/addnode/addnode.py
EXECUTABLES += $(TOOLSPATH)/compat/dil2al-polldaemon.sh
EXECUTABLES += $(TOOLSPATH)/compat/categories_a2c-NNLs-init.sh
EXECUTABLES += $(TOOLSPATH)/compat/categories_hourly-NNLs-init.sh
EXECUTABLES += $(TOOLSPATH)/compat/frequent-init.sh
EXECUTABLES += $(TOOLSPATH)/compat/v1xv2x-refresh.sh
EXECUTABLES += $(TOOLSPATH)/conversion/dil2graph/dil2graph
EXECUTABLES += $(TOOLSPATH)/conversion/graph2dil/graph2dil
EXECUTABLES += $(TOOLSPATH)/conversion/graph2dil/graph2dil-diff.sh
EXECUTABLES += $(TOOLSPATH)/conversion/graph2dil/graph2dil-integrity.py
EXECUTABLES += $(TOOLSPATH)/dev/boilerplate/boilerplate
EXECUTABLES += $(TOOLSPATH)/dev/fzbuild/fzbuild.py
EXECUTABLES += $(TOOLSPATH)/glue/fzdaily.sh
EXECUTABLES += $(TOOLSPATH)/glue/graph-resident.py
EXECUTABLES += $(TOOLSPATH)/glue/graph-topics.sh
# EXECUTABLES += $(TOOLSPATH)/glue/calendarsync/calendarsync
# EXECUTABLES += $(TOOLSPATH)/glue/exact2calendar/exact2calendar
EXECUTABLES += $(TOOLSPATH)/interface/addnode/addnode.py
EXECUTABLES += $(TOOLSPATH)/interface/fzdashboard/fzdashboard
EXECUTABLES += $(TOOLSPATH)/interface/fzcatchup/fzcatchup.py
EXECUTABLES += $(TOOLSPATH)/interface/fzgraphhtml/fzgraphhtml
EXECUTABLES += $(TOOLSPATH)/interface/fzgraphhtml/fzgraphhtml-term.sh
EXECUTABLES += $(TOOLSPATH)/interface/fzloghtml/fzloghtml
EXECUTABLES += $(TOOLSPATH)/interface/fzloghtml/fzloghtml-term.sh
EXECUTABLES += $(TOOLSPATH)/interface/fzloghtml/fzlog-mostrecent.sh
EXECUTABLES += $(TOOLSPATH)/interface/fzloghtml/get_log_entry.sh
EXECUTABLES += $(TOOLSPATH)/interface/fzlogmap/fzlogmap
EXECUTABLES += $(TOOLSPATH)/interface/fzlogtime/fzlogtime
EXECUTABLES += $(TOOLSPATH)/interface/fzlogtime/fzlogtime-term.sh
EXECUTABLES += $(TOOLSPATH)/interface/fzserver-info/fzserver-info
# EXECUTABLES += $(TOOLSPATH)/interface/fzvizgraph/fzvizgraph
# EXECUTABLES += $(TOOLSPATH)/interface/fzvizmilestones/fzvizmilestones
EXECUTABLES += $(TOOLSPATH)/interface/logentry/logentry.py
EXECUTABLES += $(TOOLSPATH)/interface/nodeboard/nodeboard
EXECUTABLES += $(TOOLSPATH)/interface/panes/panes-term.sh
EXECUTABLES += $(TOOLSPATH)/system/earlywiz/earlywiz.py
# EXECUTABLES += $(TOOLSPATH)/system/fzcatchup/fzcatchup
# EXECUTABLES += $(TOOLSPATH)/system/fzpassed/fzpassed
# EXECUTABLES += $(TOOLSPATH)/system/metrics/sysmetrics/sysmetrics
EXECUTABLES += $(TOOLSPATH)/system/metrics/sysmet-extract/sysmet-extract-show.sh
EXECUTABLES += $(TOOLSPATH)/system/metrics/sysmet-extract/sysmet-extract-term.sh
EXECUTABLES += $(TOOLSPATH)/system/requestmanual/requestmanual.py
# EXECUTABLES += $(TOOLSPATH)/system/schedule/checkmissing/checkmissing
# EXECUTABLES += $(TOOLSPATH)/system/schedule/checkrequired/checkrequired
# EXECUTABLES += $(TOOLSPATH)/system/schedule/favorvariable/favorvariable
# EXECUTABLES += $(TOOLSPATH)/system/syswizard/syswizard
EXECUTABLES += $(TOOLSPATH)/system/top/index-term.sh

# configuration files needed by CGI scripts
CONFIGEXPOSE =
CONFIGEXPOSE += $(FZCONFIGPATH)/server_address

# local/bin programs needed by CGI scripts
LOCALBINSYM =
LOCALBINSYM += $(LOCALBINDIR)/aha

# symbolic links to executables for CGI scripts and configuration files to call upon
SYMBIN = 
SYMBIN += $(COREPATH)/fzedit/fzedit
SYMBIN += $(COREPATH)/fzgraph/fzgraph
SYMBIN += $(COREPATH)/fzgraphsearch/fzgraphsearch
SYMBIN += $(COREPATH)/fzlog/fzlog
SYMBIN += $(COREPATH)/fzquerypq/fzquerypq
SYMBIN += $(COREPATH)/fztask/fztask.py
SYMBIN += $(COREPATH)/fzupdate/fzupdate
SYMBIN += $(COREPATH)/include/ansicolorcodes.py
SYMBIN += $(COREPATH)/include/error.py
SYMBIN += $(COREPATH)/include/fzcmdcalls.py
SYMBIN += $(COREPATH)/include/fzmodbase.py
SYMBIN += $(COREPATH)/include/Graphaccess.py
SYMBIN += $(COREPATH)/include/proclock.py
SYMBIN += $(COREPATH)/include/tcpclient.py
SYMBIN += $(COREPATH)/include/TimeStamp.py
SYMBIN += $(TOOLSPATH)/interface/fzdashboard/fzdashboard
SYMBIN += $(TOOLSPATH)/interface/fzgraphhtml/fzgraphhtml
SYMBIN += $(TOOLSPATH)/interface/fzloghtml/fzloghtml
SYMBIN += $(TOOLSPATH)/interface/fzloghtml/get_log_entry.sh
SYMBIN += $(TOOLSPATH)/interface/fzlogmap/fzlogmap
SYMBIN += $(TOOLSPATH)/interface/fzlogtime/fzlogtime
SYMBIN += $(TOOLSPATH)/interface/fzserver-info/fzserver-info
SYMBIN += $(TOOLSPATH)/system/metrics/sysmet-extract/categories_a2c.json
SYMBIN += $(COREPATH)/fzguide.system/fzguide.system
SYMBIN += $(CONFIGEXPOSE)
SYMBIN += $(LOCALBINSYM)

# CGI scripts that need to be copied to $(CGIDIR)
CGIEXE =
CGIEXE += $(COREPATH)/fzbackup/fzbackup-cgi.py
CGIEXE += $(COREPATH)/fzedit/fzedit-cgi.py
CGIEXE += $(COREPATH)/fzgraph/fzgraph-cgi.py
CGIEXE += $(COREPATH)/fzgraphsearch/fzgraphsearch-cgi.py
CGIEXE += $(COREPATH)/fzguide.system/fzguide.system-cgi.py
CGIEXE += $(COREPATH)/fzlog/fzlog-cgi.py
CGIEXE += $(COREPATH)/fzquerypq/fzquerypq-cgi.py
CGIEXE += $(COREPATH)/fztask/fztask-cgi.py
CGIEXE += $(COREPATH)/fzupdate/fzupdate-cgi.py
CGIEXE += $(TOOLSPATH)/interface/logentry-form/logentry-form.py
CGIEXE += $(TOOLSPATH)/interface/fzgraphhtml/fzgraphhtml-cgi.py
CGIEXE += $(TOOLSPATH)/interface/fzloghtml/fzloghtml-cgi.py
CGIEXE += $(TOOLSPATH)/interface/fzlink/fzlink.py
CGIEXE += $(TOOLSPATH)/interface/fzlogtime/fzlogtime.cgi
CGIEXE += $(TOOLSPATH)/interface/fzserver-info/fzserver-info-cgi.py
CGIEXE += $(TOOLSPATH)/interface/fzuistate/fzuistate.py
CGIEXE += $(TOOLSPATH)/system/metrics/sysmet-extract/sysmet-extract-cgi.py

# CGI scripts for machine-local use with w3m, which can launch programs as the user
LOCALCGI =
LOCALCGI += $(COREPATH)/fzedit/fzedit
LOCALCGI += $(COREPATH)/fzedit/fzedit-cgi.py
LOCALCGI += $(COREPATH)/fzgraphsearch/fzgraphsearch-cgi.py
LOCALCGI += $(COREPATH)/fzquerypq/fzquerypq-cgi.py
LOCALCGI += $(COREPATH)/fztask/fztask-cgi.py
LOCALCGI += $(COREPATH)/fzupdate/fzupdate-cgi.py
LOCALCGI += $(TOOLSPATH)/interface/fzlogtime/fzlogtime
LOCALCGI += $(TOOLSPATH)/interface/fzlogtime/fzlogtime.cgi

# Web interface files to copy to web servable Formalizer directory
WEBINTERFACES = 
WEBINTERFACES += $(COREPATH)/fzgraph/add_node.html
WEBINTERFACES += $(COREPATH)/fzgraphsearch/fzgraphsearch-form.html
WEBINTERFACES += $(COREPATH)/fzguide.system/fzguide.system-form.html
# WEBINTERFACES += $(TOOLSPATH)/interface/fzgraphhtml/fzgraphhtml-form.html
WEBINTERFACES += $(TOOLSPATH)/interface/logentry-form/templates/logentry-form_fullpage.template.html
WEBINTERFACES += $(TOOLSPATH)/interface/fzloghtml/fzloghtml-form.html
WEBINTERFACES += $(TOOLSPATH)/interface/timer/timer.html
WEBINTERFACES += $(TOOLSPATH)/system/doc/doc.html
WEBINTERFACES += $(TOOLSPATH)/system/doc/help
WEBINTERFACES += $(TOOLSPATH)/system/doc/system-help.html
WEBINTERFACES += $(TOOLSPATH)/system/earlywiz/earlywiz.html
WEBINTERFACES += $(TOOLSPATH)/system/latewiz/latewiz.html
WEBINTERFACES += $(TOOLSPATH)/system/latewiz/templates/prep-morning-exercise.html
WEBINTERFACES += $(TOOLSPATH)/system/metrics/sysmet-add/sysmet-add-form.html
WEBINTERFACES += $(TOOLSPATH)/system/nutrition/day-nutrition.html
WEBINTERFACES += $(TOOLSPATH)/system/planning/decisions/rttdecision/rttdecision-template.html

# Data files to make available for reading to web interfaces via symbolic links
SYMWEB =
SYMWEB += $(FZCONFIGPATH)/.fzchunkmarks

# Necessary symlinks into the core/include directory
SYMINCLUDE =
SYMINCLUDE = $(COREPATH)/fztask-server/fztaskAPI.py

# Web interface files and web resources to copy to the root web servable directory
TOPLEVEL =
# The following two are now done via fzdashboard
#TOPLEVEL += $(TOOLSPATH)/system/top/index.html
#TOPLEVEL += $(TOOLSPATH)/system/top/index-static.html
TOPLEVEL += $(TOOLSPATH)/system/top/granular-controls.html
TOPLEVEL += $(TOOLSPATH)/system/top/select.html
TOPLEVEL += $(TOOLSPATH)/system/top/integrity.html
TOPLEVEL += $(TOOLSPATH)/system/top/integrity-static.html
TOPLEVEL += $(TOOLSPATH)/interface/fzuistate/fzuistate.css
TOPLEVEL += $(TOOLSPATH)/interface/fzuistate/fzuistate.js
TOPLEVEL += $(TOOLSPATH)/interface/resources/clock.css
TOPLEVEL += $(TOOLSPATH)/interface/resources/clock.js
TOPLEVEL += $(TOOLSPATH)/interface/resources/fz.css
TOPLEVEL += $(TOOLSPATH)/interface/resources/fz-cards.css
TOPLEVEL += $(TOOLSPATH)/interface/resources/fzclosing_window.js
TOPLEVEL += $(TOOLSPATH)/interface/resources/w3.css
TOPLEVEL += $(TOOLSPATH)/interface/resources/bluetable.css
TOPLEVEL += $(MEDIAPATH)/favicon-192x192.png
TOPLEVEL += $(MEDIAPATH)/favicon-32x32.png
TOPLEVEL += $(MEDIAPATH)/favicon-16x16.png
TOPLEVEL += $(MEDIAPATH)/favicon.ico
TOPLEVEL += $(MEDIAPATH)/favicon-nodes-32x32.png
TOPLEVEL += $(FZCONFIGSRCPATH)/logentry.py/favicon-logentry-32x32.png


# Test files to copy to web servable Formalizer directory
TESTS = 
TESTS += $(TOOLSPATH)/misc/development-tests-INDEX.html
TESTS += $(TOOLSPATH)/misc/test_python_cgiformget.html
TESTS += $(TOOLSPATH)/misc/test_python_cgianalysis.html
TESTS += $(TOOLSPATH)/misc/test_visual_counters.html
TESTS += $(TOOLSPATH)/misc/test_add_node.html
TESTS += $(TOOLSPATH)/misc/test_task_chunk_countdown_bar.html
TESTS += $(TOOLSPATH)/misc/test_webfileread.html
TESTS += $(TOOLSPATH)/misc/test_timed_close.html
# +----- end  : Select Formalizer executables -----+

# See https://www.gnu.org/software/make/manual/html_node/Force-Targets.html
.PHONY: FORCE build $(COMPILABLES)

# simply calling `make` refreshes executables and code documentation
all: init executables doxygen tests

init: FORCE
	@echo '-------------------------------------------------------------------'
	@echo 'Making Formalizer executables available'
	@echo 'Target directories: $(EXEDIR), $(CGIEXE)'
	@echo '-------------------------------------------------------------------'

# call `make build` to build recursively
build: $(COMPILABLES)

$(COMPILABLES):
	$(MAKE) -C $@

# cannot build before building LIBCOMPDIRS
$(CORECOMPDIRS): $(LIBCOMPDIRS)

# cannot build before building LIBCOMPDIRS
$(TOOLSCOMPDIRS): $(LIBCOMPDIRS)

# call `make executables` to refresh links to executables
executables: $(EXECUTABLES)
	mkdir -p $(EXEDIR)
	ln -f -s $(EXECUTABLES) $(EXEDIR)/
	sudo ln -f -s $(SYMBIN) $(CGIDIR)/
	sudo cp -f $(CGIEXE) $(CGIDIR)/
	sudo ln -f -s $(LOCALCGI) $(W3MCGIDIR)/
	cp -r -f $(WEBINTERFACES) $(WEBINTERFACESDIR)/
	sudo ln -f -s $(SYMWEB) $(WEBINTERFACESDIR)/
	ln -f -s $(SYMINCLUDE) $(COREINCLUDEPATH)/
	sudo cp -f $(TOPLEVEL) $(WEBBASEDIR)/
	./pycmdlinks.sh $(EXEDIR)

# call `make tests` to make test files available
tests: $(TESTS)
	sudo cp -f $(TESTS) $(WEBINTERFACESDIR)/

# call `make doxygen` to refresh code documentation
doxygen: FORCE
	doxygen Doxyfile

# call `make clean` to remove links to executables
clean:
	rm -f $(EXEDIR)/*
