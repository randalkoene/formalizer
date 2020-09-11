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
CORECOMPDIRS += $(COREPATH)/
CORECOMPDIRS += $(COREPATH)/
TOOLSCOMPDIRS =
COMPILABLES = $(LIBCOMPDIRS) $(CORECOMPDIRS) $(TOOLSCOMPDIRS)
# +----- end  : Select Formalizer compilables -----+

# +----- begin: Select Formalizer executables -----+
EXECUTABLES = $(COREPATH)/fzquerypq/fzquerypq
EXECUTABLES += $(COREPATH)/fzserverpq/fzserverpq
EXECUTABLES += $(COREPATH)/fzsetup/fzsetup.py
EXECUTABLES += $(COREPATH)/fzguide.system/fzguide.system

EXECUTABLES += $(TOOLSPATH)/dev/boilerplate/boilerplate
EXECUTABLES += $(TOOLSPATH)/compat/dil2al-polldaemon.sh
EXECUTABLES += $(TOOLSPATH)/graph2dil/graph2dil
EXECUTABLES += $(TOOLSPATH)/requestmanual/requestmanual.py
EXECUTABLES += $(TOOLSPATH)/dil2graph/dil2graph
EXECUTABLES += $(TOOLSPATH)/earlywiz/earlywiz.py
EXECUTABLES += $(TOOLSPATH)/nodeboard/nodeboard

CGIEXE = $(COREPATH)/fzquerypq/fzquerypq
CGIEXE += $(TOOLSPATH)/logentry-form/logentry-form.py
# +----- end  : Select Formalizer executables -----+

# See https://www.gnu.org/software/make/manual/html_node/Force-Targets.html
.PHONY: FORCE

all: init executables doxygen

init: FORCE
	@echo '-------------------------------------------------------------------'
	@echo 'Making Formalizer executables available'
	@echo 'Target directories: $(EXEDIR), $(CGIEXE)'
	@echo '-------------------------------------------------------------------'


executables: $(EXECUTABLES)
	mkdir -p $(EXEDIR)
	ln -f -s $(EXECUTABLES) $(EXEDIR)/
	sudo ln -f -s $(CGIEXE) $(CGIDIR)/

doxygen: FORCE
	doxygen Doxyfile

clean:
	rm -f $(EXEDIR)/*

#doc++:
#	rm -r -f html
#	doc++ -d html spiker.dxx


