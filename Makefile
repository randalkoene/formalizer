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
FZCONFIGPATH=$(HOME)/.formalizer

#TEMPLATEDIR = -DDEFAULT_TEMPLATE_DIR=\"$(FORMALIZERPATH_THIS)\" 

EXEDIR=$(HOME)/.formalizer/bin
CGIDIR=/usr/lib/cgi-bin
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
# EXECUTABLES += $(COREPATH)/fzedit/fzedit
EXECUTABLES += $(COREPATH)/fzbackup/fzbackup.py
EXECUTABLES += $(COREPATH)/fzinfo/fzinfo.py
EXECUTABLES += $(COREPATH)/fzguide.system/fzguide.system
EXECUTABLES += $(COREPATH)/fzlog/fzlog
EXECUTABLES += $(COREPATH)/fzquerypq/fzquerypq
EXECUTABLES += $(COREPATH)/fzserverpq/fzserverpq
# EXECUTABLES += $(COREPATH)/fzserverpq/fzserverpq-graph
# EXECUTABLES += $(COREPATH)/fzserverpq/fzserverpq-log
# EXECUTABLES += $(COREPATH)/fzserverpq/fzserverpq-metrics
EXECUTABLES += $(COREPATH)/fzsetup/fzsetup.py
# EXECUTABLES += $(COREPATH)/fzshift/fzshift
# EXECUTABLES += $(COREPATH)/fztask/fztask
# EXECUTABLES += $(COREPATH)/fzupdate/fzupdate

EXECUTABLES += $(TOOLSPATH)/compat/dil2al-polldaemon.sh
EXECUTABLES += $(TOOLSPATH)/compat/v1xv2x-refresh.sh
EXECUTABLES += $(TOOLSPATH)/conversion/dil2graph/dil2graph
EXECUTABLES += $(TOOLSPATH)/conversion/graph2dil/graph2dil
EXECUTABLES += $(TOOLSPATH)/dev/boilerplate/boilerplate
EXECUTABLES += $(TOOLSPATH)/dev/fzbuild/fzbuild.py
# EXECUTABLES += $(TOOLSPATH)/glue/calendarsync/calendarsync
# EXECUTABLES += $(TOOLSPATH)/glue/exact2calendar/exact2calendar
EXECUTABLES += $(TOOLSPATH)/interface/fzgraphhtml/fzgraphhtml
EXECUTABLES += $(TOOLSPATH)/interface/fzloghtml/fzloghtml
EXECUTABLES += $(TOOLSPATH)/interface/fzlogtime/fzlogtime
EXECUTABLES += $(TOOLSPATH)/interface/fzlogtime/fzlogtime-term.sh
EXECUTABLES += $(TOOLSPATH)/interface/fzserver-info/fzserver-info
# EXECUTABLES += $(TOOLSPATH)/interface/fzvizgraph/fzvizgraph
# EXECUTABLES += $(TOOLSPATH)/interface/fzvizmilestones/fzvizmilestones
EXECUTABLES += $(TOOLSPATH)/interface/logentry/logentry.py
EXECUTABLES += $(TOOLSPATH)/interface/nodeboard/nodeboard
EXECUTABLES += $(TOOLSPATH)/system/earlywiz/earlywiz.py
# EXECUTABLES += $(TOOLSPATH)/system/fzcatchup/fzcatchup
# EXECUTABLES += $(TOOLSPATH)/system/fzpassed/fzpassed
# EXECUTABLES += $(TOOLSPATH)/system/metrics/sysmetrics/sysmetrics
EXECUTABLES += $(TOOLSPATH)/system/requestmanual/requestmanual.py
# EXECUTABLES += $(TOOLSPATH)/system/schedule/checkmissing/checkmissing
# EXECUTABLES += $(TOOLSPATH)/system/schedule/checkrequired/checkrequired
# EXECUTABLES += $(TOOLSPATH)/system/schedule/favorvariable/favorvariable
# EXECUTABLES += $(TOOLSPATH)/system/syswizard/syswizard

# configuration files needed by CGI scripts
CONFIGEXPOSE =
CONFIGEXPOSE += $(FZCONFIGPATH)/server_address

# symbolic links to executables for CGI scripts and configuration files to call upon
SYMBIN = 
SYMBIN += $(COREPATH)/fzgraph/fzgraph
SYMBIN += $(COREPATH)/fzlog/fzlog
SYMBIN += $(COREPATH)/fzquerypq/fzquerypq
SYMBIN += $(TOOLSPATH)/interface/fzgraphhtml/fzgraphhtml
SYMBIN += $(TOOLSPATH)/interface/fzloghtml/fzloghtml
SYMBIN += $(TOOLSPATH)/interface/fzserver-info/fzserver-info
SYMBIN += $(COREPATH)/fzguide.system/fzguide.system
SYMBIN += $(CONFIGEXPOSE)

# CGI scripts that need to be copied to $(CGIDIR)
CGIEXE =
CGIEXE += $(COREPATH)/fzgraph/fzgraph-cgi.py
CGIEXE += $(TOOLSPATH)/interface/logentry-form/logentry-form.py
CGIEXE += $(TOOLSPATH)/interface/fzgraphhtml/fzgraphhtml-cgi.py
CGIEXE += $(TOOLSPATH)/interface/fzloghtml/fzloghtml-cgi.py
CGIEXE += $(TOOLSPATH)/interface/fzlink/fzlink.py
CGIEXE += $(TOOLSPATH)/interface/fzlogtime/fzlogtime
CGIEXE += $(TOOLSPATH)/interface/fzlogtime/fzlogtime.cgi
CGIEXE += $(TOOLSPATH)/interface/fzserver-info/fzserver-info-cgi.py
CGIEXE += $(COREPATH)/fzguide.system/fzguide.system-cgi.py

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
	cp -f $(WEBINTERFACES) $(WEBINTERFACESDIR)/
	sudo cp -f $(TOPLEVEL) $(WEBBASEDIR)/
	./pycmdlinks.sh $(EXEDIR)


# call `make doxygen` to refresh code documentation
doxygen: FORCE
	doxygen Doxyfile

# call `make clean` to remove links to executables
clean:
	rm -f $(EXEDIR)/*
