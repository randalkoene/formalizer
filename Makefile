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
FORMALIZERPATH_THIS=$(FORMALIZERPATH)

#TEMPLATEDIR = -DDEFAULT_TEMPLATE_DIR=\"$(FORMALIZERPATH_THIS)\" 

EXEDIR=$(HOME)/.formalizer/bin
CGIDIR=/usr/lib/cgi-bin

INC=$(COREINCLUDEPATH)
OBJ=./obj
TEST=./test
# +----- end  : Formalizer source directory structure -----+

# +----- begin: Select Formalizer compilables -----+
LIBCOMPDIRS = $(COREPATH)/lib

CORECOMPDIRS = $(COREPATH)/fzguide.system
CORECOMPDIRS += $(COREPATH)/fzquerypq
CORECOMPDIRS += $(COREPATH)/fzserverpq

TOOLSCOMPDIRS = $(TOOLSPATH)/conversion/dil2graph
TOOLSCOMPDIRS += $(TOOLSPATH)/conversion/graph2dil
TOOLSCOMPDIRS += $(TOOLSPATH)/dev/boilerplate
TOOLSCOMPDIRS += $(TOOLSPATH)/interface/nodeboard

COMPILABLES = $(LIBCOMPDIRS) $(CORECOMPDIRS) $(TOOLSCOMPDIRS)
# +----- end  : Select Formalizer compilables -----+

# +----- begin: Select Formalizer executables -----+
EXECUTABLES =
EXECUTABLES += $(COREPATH)/include/coreversion.py
# EXECUTABLES = $(COREPATH)/fzaddnode/fzaddnode
# EXECUTABLES = $(COREPATH)/fzedit/fzedit
EXECUTABLES += $(COREPATH)/fzguide.system/fzguide.system
# EXECUTABLES = $(COREPATH)/fzlog/fzlog
EXECUTABLES += $(COREPATH)/fzquerypq/fzquerypq
EXECUTABLES += $(COREPATH)/fzserverpq/fzserverpq
# EXECUTABLES += $(COREPATH)/fzserverpq/fzserverpq-graph
# EXECUTABLES += $(COREPATH)/fzserverpq/fzserverpq-log
# EXECUTABLES += $(COREPATH)/fzserverpq/fzserverpq-metrics
EXECUTABLES += $(COREPATH)/fzsetup/fzsetup.py
# EXECUTABLES = $(COREPATH)/fzshift/fzshift
# EXECUTABLES = $(COREPATH)/fztask/fztask
# EXECUTABLES = $(COREPATH)/fzupdate/fzupdate

EXECUTABLES += $(TOOLSPATH)/compat/dil2al-polldaemon.sh
EXECUTABLES += $(TOOLSPATH)/dev/boilerplate/boilerplate
EXECUTABLES += $(TOOLSPATH)/conversion/dil2graph/dil2graph
EXECUTABLES += $(TOOLSPATH)/conversion/graph2dil/graph2dil
EXECUTABLES += $(TOOLSPATH)/dev/boilerplate/boilerplate
# EXECUTABLES += $(TOOLSPATH)/glue/calendarsync/calendarsync
# EXECUTABLES += $(TOOLSPATH)/glue/exact2calendar/exact2calendar
# EXECUTABLES += $(TOOLSPATH)/interface/fzgraphhtml/fzgraphhtml
# EXECUTABLES += $(TOOLSPATH)/interface/fzloghtml/fzloghtml
# EXECUTABLES += $(TOOLSPATH)/interface/fzvizgraph/fzvizgraph
# EXECUTABLES += $(TOOLSPATH)/interface/fzvizmilestones/fzvizmilestones
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

CGIEXE = $(TOOLSPATH)/interface/logentry-form/logentry-form.py
# CGIEXE += $(TOOLSPATH)/interface/fzlink/fzlink
CGIEXE += $(COREPATH)/fzquerypq/fzquerypq
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
	sudo ln -f -s $(CGIEXE) $(CGIDIR)/

# call `make doxygen` to refresh code documentation
doxygen: FORCE
	doxygen Doxyfile

# call `make clean` to remove links to executables
clean:
	rm -f $(EXEDIR)/*