#
# Makefile for fzdashboard
# (by Randal A. Koene, randalk@minduploading.org)
#

SHELL=/usr/bin/bash

# +----- begin: Output preconfigurations -----+
STANDARD_STREAMS = -DFORMALIZER_BASE_OUT_OSTREAM_PTR=\&std::cout
STANDARD_STREAMS += -DFORMALIZER_BASE_ERR_OSTREAM_PTR=\&std::cerr
# +----- end  : Output preconfigurations -----+

# +----- begin: Output preconfigurations -----+

# +----- end  : Output preconfigurations -----+

# +----- begin: Compilers and machine specific tuning -----+
CC=gcc
CCPP=g++ 

# MACHSPEC=
# GCCTUNE=$(if $(shell gcc -v 2>&1 | grep "^gcc version[^0-9]*[4-9][.]"),-mtune=pentium4)
# SSEOPT= -mfpmath=sse
MACHOPT= $(GCCTUNE) $(SSEOPT)
# +----- end  : Compilers and machine specific tuning -----+

# +----- begin: Formalizer source directory structure -----+
FORMALIZERPATH=$(HOME)/src/formalizer
COREINCLUDEPATH=$(FORMALIZERPATH)/core/include
CORELIBPATH=$(FORMALIZERPATH)/core/lib
FORMALIZERPATH_THIS=$(FORMALIZERPATH)/tools/interface/fzdashboard

TEMPLATEDIR = -DDEFAULT_TEMPLATE_DIR=\"$(FORMALIZERPATH_THIS)\" 

INC=$(COREINCLUDEPATH)
OBJ=./obj
TEST=./test
# +----- end  : Formalizer source directory structure -----+

# Notes:
# - to find the postgresql header files (e.g libpq-fe.h), do this: pg_config --includedir
# - to find the postgresql library files fo this: pg_config --libdir
POSTGRESINC = /usr/include/postgresql
POSTGRESLIB = -L/usr/lib/x86_64-linux-gnu -lpq

# To find the Boost libraries
# See the note on POSIX systems in Building Boost.Interprocess at https://www.boost.org/doc/libs/1_38_0/doc/html/interprocess.html#interprocess.intro.introduction_building_interprocess.
BOOSTINTERPROCESSLIB_REQS = -lrt -lpthread
LOCALINC = $(HOME)/local/include
LOCALLIB = $(BOOSTINTERPROCESSLIB_REQS)

# +----- begin: Essential include and library paths for this program -----+
INCLUDES := -I$(CPATH) -I$(COREINCLUDEPATH)
INCLUDES += -I$(POSTGRESINC)
INCLUDES += -I$(LOCALINC)
LIB_PATH = $(POSTGRESLIB)
LIB_PATH += $(LOCALLIB)
# +----- begin: Essential include and library paths for this program -----+

# +----- begin: In case not included from CPATH -----+
# INCLUDES= $(INCLUDE)-I$(HOME)/src/include
# +----- end  : In case not included from CPATH -----+

# +----- being: Compiler Build options -----+
CPPMATH = -mieee-fp -ffast-math
# Note: Some Formalizer code uses concepts new to C++20.
CPP2020 = -std=gnu++2a
CPPXTRA := $(CPP2020) $(MACHOPT) -pedantic -Wall -Werror 
# +----- end  : Compiler Build options -----+

# +----- begin: Optimization options -----+
CPPOPT = -felide-constructors
## optimized (USUALLY USE THIS ONE!):
COMPOPT = -O3
OPTSTR = optimized
## optimized + debugging info (as per GDB recommendations 2019-02-01)
#COMPOPT= -g -O3
#OPTSTR=production_optimized_with_dbg_symbols
## debugging information (updates 2019-02-01):
#COMPOPT= -g -Og
#OPTSTR=editing_optimized_for_debugging
## generate profile information for use with gprof:
#COMPOPT= -g -O6
#OPTSTR=profiling
# +----- end  : Optimization options -----+

# +----- begin: Compile-time Macro definitions -----+
SPECIFYDEFAULTHOMEDIRDEF = -DDEFAULTHOMEDIR=\"$(HOME)\" 
COMPILETIMEMACRODEFS += $(SPECIFYDEFAULTHOMEDIRDEF)
CPPXTRA += $(COMPILETIMEMACRODEFS)
# +----- end  : Compile-time Macro definitions -----+

# +----- begin: Putting together the C flags -----+
# CFLAGS = $(COMPOPT) $(MACHSPEC) $(CFXTRA)
# RXCFLAGS = $(COMPOPT) $(MACHSPEC) $(RXCFXTRA)
# +----- end  : Putting together the C flags -----+

# +----- begin: Putting together the C++ flags -----+
CPPFLAGS = $(COMPOPT) $(CPPOPT) $(MACHSPEC) $(CPPXTRA) $(INCLUDES)
# +----- end  : Putting together the C++ flags -----+

# +----- begin: Select Formalizer core library objects -----+
CORE_OBJS = 
CORE_OBJS += $(CORELIBPATH)/obj/error.o
CORE_OBJS += $(CORELIBPATH)/obj/config.o
CORE_OBJS += $(CORELIBPATH)/obj/stringio.o
CORE_OBJS += $(CORELIBPATH)/obj/jsonlite.o
# CORE_OBJS += $(CORELIBPATH)/obj/html.o
CORE_OBJS += $(CORELIBPATH)/obj/standard.o
CORE_OBJS += $(CORELIBPATH)/obj/general.o
CORE_OBJS += $(CORELIBPATH)/obj/templater.o
CORE_OBJS += $(CORELIBPATH)/obj/fzpostgres.o
CORE_OBJS += $(CORELIBPATH)/obj/TimeStamp.o
# CORE_OBJS += $(CORELIBPATH)/obj/Graphaccess.o
# CORE_OBJS += $(CORELIBPATH)/obj/ReferenceTime.o
CORE_OBJS += $(CORELIBPATH)/obj/Graphbase.o
CORE_OBJS += $(CORELIBPATH)/obj/Graphtypes.o
# CORE_OBJS += $(CORELIBPATH)/obj/LogtypesID.o
# CORE_OBJS += $(CORELIBPATH)/obj/Logtypes.o
# CORE_OBJS += $(CORELIBPATH)/obj/Graphpostgres.o
# CORE_OBJS += $(CORELIBPATH)/obj/Logpostgres.o
# CORE_OBJS += $(CORELIBPATH)/obj/Guidepostgres.o
CORE_OBJS += $(CORELIBPATH)/obj/html.o
CORE_OBJS += $(CORELIBPATH)/obj/utf8.o
# CORE_OBJS += $(CORELIBPATH)/obj/dilaccess.o
# +----- end  : Select Formalizer core library objects -----+

.PHONY: FORCE

all: init fzdashboard

init: FORCE
	@echo '-------------------------------------------------------------------'
	@echo 'Compilation options: $(MACHSTR), $(OPTSTR), $(REGEXSTR)'
	@echo 'Includes: $(CPPFLAGS)'
	@echo '-------------------------------------------------------------------'

$(OBJ)/render.o: render.cpp render.hpp
	$(CCPP) $(CPPFLAGS) $(COLORS) $(STANDARD_STREAMS) $(FORMALIZERROOT) $(TEMPLATEDIR) -c render.cpp -o $(OBJ)/render.o


$(OBJ)/fzdashboard.o: fzdashboard.cpp fzdashboard.hpp version.hpp
	$(CCPP) $(CPPFLAGS) $(COLORS) $(STANDARD_STREAMS) $(TEMPLATEDIR) -c fzdashboard.cpp -o $(OBJ)/fzdashboard.o

fzdashboard: $(OBJ)/fzdashboard.o $(OBJ)/render.o $(CORE_OBJS)
	$(CCPP) $(CPPFLAGS) $(STANDARD_STREAMS)  $(TEMPLATEDIR) $^ -o fzdashboard $(LIB_PATH)

clean:
	cp -f fzdashboard fzdashboard.bak 2>/dev/null || :
	rm -f $(OBJ)/*.o fzdashboard

#doc++:
#	rm -r -f html
#	doc++ -d html spiker.dxx
