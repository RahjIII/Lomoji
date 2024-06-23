# lomoji - Last Outpost Emoji Translator
# Created: Wed May 29 10:35:37 AM EDT 2024 malakai 
# Copyright © 2024 Jeffrika Heavy Industries 
# $Id: makefile,v 1.7 2024/06/23 13:37:12 malakai Exp $

#PREFIX is the install directory.
PREFIX=/usr/local

# $(PROJECT) is the base name of the project.
PROJECT = lomoji
DESCRIPTION = "Last Outpost Emoji Translator"
LOMOJI_VERSION = 0.1

#SHARE_PREFIX = /usr/share
SHARE_PREFIX = $(PREFIX)/share

# List the .c files here.  Order doesn't matter.  Dont worry about header file
# dependencies, this makefile will figure them out automatically. 

PROJECT_CFILES = lomoji.c lomoji-demo.c
LIB_EXCLUDE_OBJS = lomoji-demo.o
LIB_INCLUDES = lomoji.h

# The list of HFILES, (required for making the ctags database) is generated
# automatically from the PROJECT_CFILES list.  However, it is possible that not
# everything in PROJECT_CFILES has a corresponding .h file.  MISSING_HFILES
# lists the .h's that aren't expected to exist.  ADDITIONAL_HFILES lists the
# .h's for which no .c exists.
ADDITIONAL_HFILES = 
MISSING_HFILES = 

# CDEBUG, use -g for gdb symbols.  For gprof, add -pg and -no-pie.
# CDEBUG = -g
# CDEBUG = -g -fsanitize=address
CDEBUG = -O2

# #### Flags and linklibs definitions ####

CDEFINES = -D'PREFIX="$(PREFIX)"' -D'LOMOJI_VERSION="$(LOMOJI_VERSION)"' -D'SHARE_PREFIX="$(SHARE_PREFIX)"'

#CFLAGS = -Wunused -Wimplicit-function-declaration -Wno-unused-but-set-variable -Wno-format-overflow -Wno-format-truncation `pkg-config --cflags glib-2.0`
CFLAGS = -Wall -fpic `pkg-config --cflags glib-2.0`

LDFLAGS =
#LINKLIBS = -lresolv -lssl -lcrypto `pkg-config --libs glib-2.0` -lpcre2-8
LINKLIBS = `pkg-config --libs glib-2.0`

# The project.pc file is built directly from the makefile.
define PKG_CONFIG
prefix=$(PREFIX)
exec_prefix=$${prefix}/bin
includedir=$${prefix}/include
libdir=$${exec_prefix}/lib

Name: $(PROJECT)
Description: $(DESCRIPTION)
Version: $(LOMOJI_VERSION)
Requires.private: glib-2.0
Cflags: -I$${includedir} 
Libs: -L$${libdir} -l$(PROJECT)
endef


# #### ############################################# ###
# ####         Makefile magic begins here.           ###
# #### Very little needs to change beyond this line! ###
# #### ############################################# ###

LIB_SO = lib$(PROJECT).so
LIB_PROJECT = $(LIB_SO).$(LOMOJI_VERSION)

CC=gcc
BUILD = ./build

CFILES = $(PROJECT_CFILES)

# HFILES generated automatically from CFILES, with additions and exclusions
HFILES := $(ADDITIONAL_HFILES)
HFILES += $(addsuffix .h, $(basename $(CFILES)))
HFILES := $(filter-out $(MISSING_HFILES), $(HFILES))

PROJECT_OFILES = $(PROJECT_CFILES:%.c=$(BUILD)/%.o)
LIB_PROJECT_OFILES := $(filter-out $(BUILD)/$(LIB_EXCLUDE_OBJS), $(PROJECT_OFILES))

PROJECT_DFILES = $(PROJECT_CFILES:%.c=$(BUILD)/%.d)

RUN = .

# #### Recipies Start Here ####

$(info ---------- START OF COMPILATION -----------)
all : $(BUILD) run lib tags $(BUILD)/$(PROJECT).pc

# Copying the binaries...
#.PHONY: $(PROJECT)
$(PROJECT) : $(BUILD) $(BUILD)/$(PROJECT)

.PHONY: run
run : $(PROJECT) $(RUN)/$(PROJECT)

.PHONY: lib
lib : $(PROJECT) $(BUILD)/$(LIB_PROJECT)

# The mv command is to prevent 'text file busy' error, and the 2>/ is some bash
# bullshit to keep the command from failing the make if the source file doesn't
# exist.
$(RUN)/$(PROJECT) : $(BUILD)/$(PROJECT)
	-mv -f $(RUN)/$(PROJECT) $(RUN)/$(PROJECT).prev 2>/dev/null || true
	cp $(BUILD)/$(PROJECT) $(RUN)/$(PROJECT)

# Create build directory...
$(BUILD) : 
	mkdir -p $(BUILD)

# Create run directory...
$(RUN) : 
	mkdir -p $(RUN)

# Linking the PROJECT binary...
$(BUILD)/$(PROJECT) : $(PROJECT_OFILES)
	$(CC) $(CDEBUG) $(LDFLAGS) $^ -o $(@) $(LINKLIBS)

# Linking the LIB_PROJECT library...
$(BUILD)/$(LIB_PROJECT) : $(LIB_PROJECT_OFILES)
	$(CC) $(CDEBUG) -shared $(LDFLAGS) $^ -o $(@) -Wl,-soname,$(LIB_PROJECT) $(LINKLIBS)

# check the .h dependency rules in the .d files made by gcc
-include $(PROJECT_DFILES)

# Build the .o's from the .c files, building .d's as you go.
$(BUILD)/%.o : %.c
	$(CC) $(CDEBUG) $(CDEFINES) $(CFLAGS) -MMD -c $< -o $(@)

# Updating the tags file...
tags : $(HFILES) $(CFILES)
	ctags $(HFILES) $(CFILES)

# Cleaning up...
# .PHONY just means 'not really a filename to check for'
.PHONY: clean
clean : 
	-rm $(BUILD)/$(PROJECT) $(PROJECT) $(OFILES) $(DFILES) tags
	-rm $(RUN)/$(PROJECT).prev
	-rm -rI $(BUILD)

.PHONY: wall-summary
wall-summary: 
	$(info ** Enable these errors in the CFLAGS section to track them down. )
	$(info ** Run this after a make clean for full effect! )
	make "CFLAGS=-Wall" 2>&1 | egrep -o "\[-W.*\]" | sort | uniq -c | sort -n

.PHONY: dist
dist: $(BUILD) $(BUILD)/$(PROJECT) FILES
	$(eval DISTDIR := $(PROJECT)-$(LOMOJI_VERSION))
	$(eval DISTFILES := $(shell xargs -a FILES))
	$(info --- Creating Distribution File in $(BUILD)/$(DISTDIR).tgz ----) 
	-mkdir $(BUILD)/$(DISTDIR)
	-cp $(DISTFILES) $(BUILD)/$(DISTDIR)
	(cd $(BUILD); tar -cvzf $(DISTDIR).tgz $(DISTDIR))

$(BUILD)/$(PROJECT).pc:
	$(info ---- Creating $@ ----) 
	$(file > $@,$(PKG_CONFIG))

.PHONY: install
install: $(BUILD)/$(PROJECT) $(BUILD)/$(LIB_PROJECT) $(LIB_INCLUDES) $(BUILD)/$(PROJECT).pc
	$(info ---- Install dir is $(PREFIX) ----)
	install -D $(BUILD)/$(PROJECT) $(PREFIX)/bin/
	install -D  $(LIB_INCLUDES) $(PREFIX)/include/
	install -D  -m 644 $(BUILD)/$(LIB_PROJECT) $(PREFIX)/lib/
	install -d  $(PREFIX)/lib/pkgconfig/
	install -D  $(BUILD)/$(PROJECT).pc  $(PREFIX)/lib/pkgconfig/
	ln -sf $(LIB_PROJECT) $(PREFIX)/lib/$(LIB_SO)
	install -d  $(PREFIX)/share/$(PROJECT)/
	ln -sf /usr/share/unicode/cldr/common/annotations/en.xml $(SHARE_PREFIX)/lomoji/default.xml
	ln -sf /usr/share/unicode/cldr/common/annotationsDerived/en.xml $(SHARE_PREFIX)/lomoji/derived.xml
	install -D  example_extra.xml $(SHARE_PREFIX)/lomoji/example_extra.xml
	ln -sf  $(SHARE_PREFIX)/lomoji/example_extra.xml $(SHARE_PREFIX)/lomoji/extra.xml
	ldconfig

