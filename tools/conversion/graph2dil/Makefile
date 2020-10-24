#
# Makefile for graph2dil
# (by Randal A. Koene, randalk@minduploading.org)
#

SHELL=/usr/bin/bash

# +----- begin: Output preconfigurations -----+
STANDARD_STREAMS = -DFORMALIZER_BASE_OUT_OSTREAM_PTR=\&std::cout
STANDARD_STREAMS += -DFORMALIZER_BASE_ERR_OSTREAM_PTR=\&std::cerr
# +----- end  : Output preconfigurations -----+

# +----- begin: Output preconfigurations -----+
OUTPUTPRECONFIGS = -DGRAPH2DIL_OUTPUT_DIR=\"/var/www/html/graph2dil\"
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
FORMALIZERPATH_THIS=$(FORMALIZERPATH)/tools/graph2dil

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

# +----- begin: include to use dil2al object code -----+
# DIL2AL = $(HOME)/src/dil2al
# INCLUDES += $(DIL2AL)
# Note: dil2al code depends on some workarounds for gcc 4.x and gcc 7.1+
# CPPTEMPWORKAROUND = -fno-access-control -Wno-format-truncation
# Note: Built around a regex-gnu.h copy adapted for integration with C++
### GCC3FXTRA = $(if $(shell gcc -v 2>&1 | grep "^gcc version[^0-9]*[3-9][.]"),-Wno-unused-function)
# CPPXTRA += $(CPPTEMPWORKAROUND) $(GCC3FXTRA)
# ALT_REGEX = $(DIL2AL)/regex-gnu.o
# ALT_REGEX_H = \"regex-gnu.h\"
# DIL2ALDEF = -D__DIL2AL__
# STDCHEADS = -DSTDC_HEADERS
# CFXTRA = $(DIL2ALDEF) $(STDCHEADS) -pedantic -Wall -Werror -Wno-char-subscripts $(GCC3FXTRA)
# RXCFXTRA = $(DIL2ALDEF) $(STDCHEADS) -pedantic -Wall $(GCC3FXTRA)
# REGEXSTR = regex-gnu_for_C++
# SAFEREGEX = -D_BIGREGEX_SAFE_MATCHES
# ALTREGEXDEF = -D_ALT_REGEX_H=$(ALT_REGEX_H) -D_USE_ALT_REGEX -D_CPP_REGEX
# COMPILETIMEMACRODEFS += $(DIL2ALDEF) $(SAFEREGEX) $(ALTREGEXDEF)
# DIL2AL_OBJS = $(DIL2AL)/utilities.o $(DIL2AL)/interface.o $(DIL2AL)/testcode.o $(DIL2AL)/diladmin.o \
			$(DIL2AL)/tladmin.o $(DIL2AL)/cksum.o $(DIL2AL)/alcomp.o $(DIL2AL)/tlfilter.o \
			$(DIL2AL)/BigString.o $(DIL2AL)/BigRegex.o
### RXSPECIAL = -Wno-error
# +----- end  : include to use dil2al object code -----+

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
CORE_OBJS = $(CORELIBPATH)/obj/error.o
CORE_OBJS += $(CORELIBPATH)/obj/config.o
CORE_OBJS += $(CORELIBPATH)/obj/standard.o
CORE_OBJS += $(CORELIBPATH)/obj/general.o
CORE_OBJS += $(CORELIBPATH)/obj/stringio.o
CORE_OBJS += $(CORELIBPATH)/obj/jsonlite.o
CORE_OBJS += $(CORELIBPATH)/obj/utf8.o
CORE_OBJS += $(CORELIBPATH)/obj/templater.o
CORE_OBJS += $(CORELIBPATH)/obj/ReferenceTime.o
CORE_OBJS += $(CORELIBPATH)/obj/TimeStamp.o
CORE_OBJS += $(CORELIBPATH)/obj/Graphbase.o
CORE_OBJS += $(CORELIBPATH)/obj/Graphtypes.o
CORE_OBJS += $(CORELIBPATH)/obj/LogtypesID.o
CORE_OBJS += $(CORELIBPATH)/obj/Logtypes.o
CORE_OBJS += $(CORELIBPATH)/obj/fzpostgres.o
CORE_OBJS += $(CORELIBPATH)/obj/Graphaccess.o
CORE_OBJS += $(CORELIBPATH)/obj/Graphpostgres.o
CORE_OBJS += $(CORELIBPATH)/obj/Logpostgres.o
# CORE_OBJS += $(CORELIBPATH)/obj/dilaccess.o
# +----- end  : Select Formalizer core library objects -----+

.PHONY: FORCE

all: init graph2dil

init: FORCE
	@echo '-------------------------------------------------------------------'
	@echo 'Compilation options: $(MACHSTR), $(OPTSTR), $(REGEXSTR)'
	@echo 'Includes: $(CPPFLAGS)'
	@echo '-------------------------------------------------------------------'

$(OBJ)/log2tl.o: log2tl.cpp log2tl.hpp version.hpp
	$(CCPP) $(CPPFLAGS) $(COLORS) $(TEMPLATEDIR) $(STANDARD_STREAMS) -c log2tl.cpp -o $(OBJ)/log2tl.o

$(OBJ)/graph2dil.o: graph2dil.cpp graph2dil.hpp version.hpp
	$(CCPP) $(CPPFLAGS) $(COLORS) $(TEMPLATEDIR) $(STANDARD_STREAMS) -c graph2dil.cpp -o $(OBJ)/graph2dil.o

graph2dil: $(OBJ)/graph2dil.o $(OBJ)/log2tl.o $(CORE_OBJS)
	$(CCPP) $(CPPFLAGS) $(STANDARD_STREAMS) $(OUTPUTPRECONFIGS) $^ -o graph2dil $(LIB_PATH)

clean:
	cp -f graph2dil graph2dil.bak 2>/dev/null || :
	rm -f $(OBJ)/*.o graph2dil

#doc++:
#	rm -r -f html
#	doc++ -d html spiker.dxx


