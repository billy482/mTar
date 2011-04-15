MAKEFLAGS 		+= -rR --no-print-directory

# commands
CC				:= ccache ${TARGET}gcc
CSCOPE			:= cscope
CTAGS			:= ctags
GDB				:= gdb
OBJCOPY			:= ${TARGET}objcopy
STRIP			:= ${TARGET}strip

# variable
NAME			:= mtar
BIN				:= bin/${NAME}
DIR_NAME		:= $(lastword $(subst /, , $(realpath .)))
#VERSION			:= $(shell git describe)
VERSION			:= 0.0.1

BUILD_DIR		:= build
DEPEND_DIR		:= depend

SRC_FILES		:= $(sort $(shell test -d src && find src -name '*.c'))
HEAD_FILES		:= $(sort $(shell test -d src && find src -name '*.h'))

OBJ_FILES		:= $(sort $(patsubst src/%.c,${BUILD_DIR}/%.o,${SRC_FILES}))
DEP_FILES		:= $(sort $(shell test -d ${DEPEND_DIR} && find ${DEPEND_DIR} -name '*.d'))

BIN_DIRS		:= $(sort $(dir ${BIN}))
OBJ_DIRS		:= $(sort $(dir ${OBJ_FILES}))
DEP_DIRS		:= $(patsubst ${BUILD_DIR}/%,${DEPEND_DIR}/%,${OBJ_DIRS})

# compilation flags
CFLAGS			:= -std=gnu99 -pipe -O0 -ggdb3 -Wall -Wextra -pedantic -Wabi -Werror-implicit-function-declaration -Wmissing-prototypes -DMTAR_VERSION=\"${VERSION}\" -I include/
LIBS			:=

CSCOPE_OPT		:= -b -I include -R -s src -U
CTAGS_OPT		:= -R src


# phony target
.PHONY: all binary clean cscope ctags debug distclean prepare realclean run stat stat-extra TAGS tar

all: binary cscope ctags

binary: prepare ${BIN}

check:
	@echo 'Checking source files...'
	@${CC} -fsyntax-only ${CFLAGS} ${SRC_FILES}

clean:
	@echo ' RM       -Rf ${BUILD_DIR}'
	@rm -Rf ${BUILD_DIR}

cscope: cscope.out

ctags TAGS: tags

debug: ${BIN}
	@echo ' GDB      ${BIN}'
	@${GDB} ${BIN}

distclean realclean: clean
	@echo ' RM       -Rf ${BIN} ${BIN}.debug cscope.out ${DEPEND_DIR} tags'
	@rm -Rf ${BIN_DIRS} cscope.out ${DEPEND_DIR} tags

prepare: ${BIN_DIRS} ${DEP_DIRS} ${OBJ_DIRS}

rebuild: clean all

run: binary
	@echo ' ${BIN}'
	@./${BIN}

stat:
	@wc $(sort ${SRC_FILES} ${HEAD_FILES})
	@git diff --stat=${COLUMNS} --cached -M

stat-extra:
	@c_count -w 48 $(sort ${HEAD_FILES} ${SRC_FILES})

tar: ${NAME}.tar.bz2


# real target
${BIN} : ${OBJ_FILES}
	@echo " LD       $@"
	@${CC} -o ${BIN} ${OBJ_FILES} ${LIBS}
	@echo " OBJCOPY  --only-keep-debug $@ $@.debug"
	@${OBJCOPY} --only-keep-debug ${BIN} ${BIN}.debug
	@echo " STRIP    $@"
	@${STRIP} ${BIN}
	@echo " OBJCOPY  --add-gnu-debuglink=$@.debug $@"
	@${OBJCOPY} --add-gnu-debuglink=${BIN}.debug ${BIN}
	@echo " CHMOD    -x $@.debug"
	@chmod -x ${BIN}.debug

${BIN_DIRS} ${DEP_DIRS} ${OBJ_DIRS}:
	@echo " MKDIR    $@"
	@mkdir -p $@


${NAME}.tar.bz2:
	git archive --format=tar --prefix=${DIR_NAME}/ master | bzip2 -9c > $@

cscope.out: ${SRC_FILES} ${HEAD_FILES}
	@echo " CSCOPE"
	@${CSCOPE} ${CSCOPE_OPT}

tags: ${SRC_FILES} ${HEAD_FILES}
	@echo " CTAGS"
	@${CTAGS} ${CTAGS_OPT}


# implicit target
${BUILD_DIR}/%.o: src/%.c
	@echo " CC       $@"
	@${CC} -c ${CFLAGS} -Wp,-MD,${DEPEND_DIR}/$*.d,-MT,$@ -o $@ $<

ifneq (${DEP_FILES},)
include ${DEP_FILES}
endif

