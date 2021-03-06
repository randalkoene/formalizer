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
EXECUTABLES += $(COREPATH)/fzgraph/fzgraph
EXECUTABLES += $(COREPATH)/fzedit/fzedit
EXECUTABLES += $(COREPATH)/fzbackup/fzbackup.py
EXECUTABLES += $(COREPATH)/fzbackup/fzlist_backups.sh
EXECUTABLES += $(COREPATH)/fzbackup/fzrestore.sh
EXECUTABLES += $(COREPATH)/fzinfo/fzinfo.py
EXECUTABLES += $(COREPATH)/fzguide.system/fzguide.system
EXECUTABLES += $(COREPATH)/fzlog/fzlog
EXECUTABLES += $(COREPATH)/fzquerypq/fzquerypq
EXECUTABLES += $(COREPATH)/fzserverpq/fzserverpq
EXECUTABLES += $(COREPATH)/fzserverpq/fzserverpqd.sh
# EXECUTABLES += $(COREPATH)/fzserverpq/fzserverpq-graph
# EXECUTABLES += $(COREPATH)/fzserverpq/fzserverpq-log
# EXECUTABLES += $(COREPATH)/fzserverpq/fzserverpq-metrics
EXECUTABLES += $(COREPATH)/fzsetup/fzsetup.py
# EXECUTABLES += $(COREPATH)/fzshift/fzshift
EXECUTABLES += $(COREPATH)/fztask/fztask.py
EXECUTABLES += $(COREPATH)/fzupdate/fzupdate

# EXECUTABLES += $(TOOLSPATH)/addnode/addnode.py
EXECUTABLES += $(TOOLSPATH)/compat/dil2al-polldaemon.sh
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
EXECUTABLES += $(TOOLSPATH)/interface/fzcatchup/fzcatchup.py
EXECUTABLES += $(TOOLSPATH)/interface/fzgraphhtml/fzgraphhtml
EXECUTABLES += $(TOOLSPATH)/interface/fzgraphhtml/fzgraphhtml-term.sh
EXECUTABLES += $(TOOLSPATH)/interface/fzloghtml/fzloghtml
EXECUTABLES += $(TOOLSPATH)/interface/fzloghtml/fzloghtml-term.sh
EXECUTABLES += $(TOOLSPATH)/interface/fzloghtml/fzlog-mostrecent.sh
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
EXECUTABLES += $(TOOLSPATH)/system/requestmanual/requestmanual.py
# EXECUTABLES += $(TOOLSPATH)/system/schedule/checkmissing/checkmissing
# EXECUTABLES += $(TOOLSPATH)/system/schedule/checkrequired/checkrequired
# EXECUTABLES += $(TOOLSPATH)/system/schedule/favorvariable/favorvariable
# EXECUTABLES += $(TOOLSPATH)/system/syswizard/syswizard
EXECUTABLES += $(TOOLSPATH)/system/top/index-term.sh

# configuration files needed by CGI scripts
CONFIGEXPOSE =
CONFIGEXPOSE += $(FZCONFIGPATH)/server_address

# symbolic links to executables for CGI scripts and configuration files to call upon
SYMBIN = 
SYMBIN += $(COREPATH)/fzedit/fzedit
SYMBIN += $(COREPATH)/fzgraph/fzgraph
SYMBIN += $(COREPATH)/fzlog/fzlog
SYMBIN += $(COREPATH)/fzquerypq/fzquerypq
SYMBIN += $(COREPATH)/fztask/fztask.py
SYMBIN += $(COREPATH)/fzupdate/fzupdate
SYMBIN += $(TOOLSPATH)/interface/fzgraphhtml/fzgraphhtml
SYMBIN += $(TOOLSPATH)/interface/fzloghtml/fzloghtml
SYMBIN += $(TOOLSPATH)/interface/fzserver-info/fzserver-info
SYMBIN += $(COREPATH)/fzguide.system/fzguide.system
SYMBIN += $(CONFIGEXPOSE)

# CGI scripts that need to be copied to $(CGIDIR)
CGIEXE =
CGIEXE += $(COREPATH)/fzedit/fzedit-cgi.py
CGIEXE += $(COREPATH)/fzgraph/fzgraph-cgi.py
CGIEXE += $(COREPATH)/fztask/fztask-cgi.py
CGIEXE += $(COREPATH)/fzupdate/fzupdate-cgi.py
CGIEXE += $(TOOLSPATH)/interface/logentry-form/logentry-form.py
CGIEXE += $(TOOLSPATH)/interface/fzgraphhtml/fzgraphhtml-cgi.py
CGIEXE += $(TOOLSPATH)/interface/fzloghtml/fzloghtml-cgi.py
CGIEXE += $(TOOLSPATH)/interface/fzlink/fzlink.py
CGIEXE += $(TOOLSPATH)/interface/fzlogtime/fzlogtime
CGIEXE += $(TOOLSPATH)/interface/fzlogtime/fzlogtime.cgi
CGIEXE += $(TOOLSPATH)/interface/fzserver-info/fzserver-info-cgi.py
CGIEXE += $(COREPATH)/fzguide.system/fzguide.system-cgi.py

# CGI scripts for machine-local use with w3m, which an launch programs as the user
LOCALCGI =
LOCALCGI += $(COREPATH)/fzedit/fzedit
LOCALCGI += $(COREPATH)/fzedit/fzedit-cgi.py
LOCALCGI += $(COREPATH)/fztask/fztask-cgi.py
LOCALCGI += $(COREPATH)/fzupdate/fzupdate-cgi.py
LOCALCGI += $(TOOLSPATH)/interface/fzlogtime/fzlogtime
LOCALCGI += $(TOOLSPATH)/interface/fzlogtime/fzlogtime.cgi

WEBINTERFACES = 
WEBINTERFACES += $(COREPATH)/fzgraph/add_node.html
# WEBINTERFACES += $(TOOLSPATH)/interface/fzgraphhtml/fzgraphhtml-form.html
WEBINTERFACES += $(TOOLSPATH)/interface/fzloghtml/fzloghtml-form.html
WEBINTERFACES += $(COREPATH)/fzguide.system/fzguide.system-form.html
WEBINTERFACES += $(TOOLSPATH)/system/metrics/sysmet-add/sysmet-add-form.html

TOPLEVEL =
TOPLEVEL += $(TOOLSPATH)/system/top/index.html
TOPLEVEL += $(TOOLSPATH)/system/top/index-static.html
TOPLEVEL += $(TOOLSPATH)/system/top/select.html
TOPLEVEL += $(TOOLSPATH)/system/top/integrity.html
TOPLEVEL += $(TOOLSPATH)/system/top/integrity-static.html
TOPLEVEL += $(TOOLSPATH)/interface/resources/fz.css
TOPLEVEL += $(TOOLSPATH)/interface/resources/fz-cards.css
TOPLEVEL += $(TOOLSPATH)/interface/resources/w3.css
TOPLEVEL += $(TOOLSPATH)/interface/resources/bluetable.css
TOPLEVEL += $(MEDIAPATH)/favicon-192x192.png
TOPLEVEL += $(MEDIAPATH)/favicon-32x32.png
TOPLEVEL += $(MEDIAPATH)/favicon-16x16.png
# +----- end  : Select Formalizer executables -----+

# See https://www.gnu.org/software/make/manual/html_node/Force-Targets.html
.PHONY: FORCE build $(COMPILABLES)

# simply calling `make` refreshes executables and code documentation
all: init executables doxygen

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
	cp -f $(WEBINTERFACES) $(WEBINTERFACESDIR)/
	sudo cp -f $(TOPLEVEL) $(WEBBASEDIR)/
	./pycmdlinks.sh $(EXEDIR)


# call `make doxygen` to refresh code documentation
doxygen: FORCE
	doxygen Doxyfile

# call `make clean` to remove links to executables
clean:
	rm -f $(EXEDIR)/*
