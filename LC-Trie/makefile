#  The code presented in this file has been tested with care but is
#  not guaranteed for any purpose. The writer does not offer any
#  warranties nor does he accept any liabilities with respect to
#  the code.
#
#  Stefan Nilsson, 8 jan 1997.
#
#  Laboratory of Information Processing Science
#  Helsinki University of Technology
#  Stefan.Nilsson@hut.fi

# The name of our program:
PROGRAM= trietest

# These are our C source files:
SRCS= qsort.c clock.c trie.c trietest.c Good_32bit_Rand.c

# The name of the C-compiler we want to use:
CC= gcc

# The flags (options) that the C-compiler needs:
CFLAGS = -O4            # optimization
#CFLAGS = -g -Wall      # debugging, warnings
#CFLAGS = -a           # tcov, profiling, works only with cc
#CFLAGS = -xpg          # gprof, works only with cc

# Names of libraries we need to link with our program:
LDLIBS= -lm

# Object files are derived from the sources, with .c replaced by .o
OBJECTS = ${SRCS:%.c=%.o}

# The first target, the name of our program:
${PROGRAM}: $(OBJECTS)
	$(LINK.c) -o $@ $(OBJECTS) $(LDLIBS)

# To remove generated object files:
clean:
	-rm -f ${OBJECTS}
