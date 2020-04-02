# # # # # # #
# Makefile for project 1
#
# changed from Makefile made by Matt Farrugia
#

CC     = gcc
CFLAGS = -Wall -std=c99
EXE    = image_tagger
OBJ    = http-server.o linklist.o handle_list.o get_post_page.o

# top (default) target
all: $(EXE)

# link executable
$(EXE): $(OBJ)
	$(CC) $(CFLAGS) -o $(EXE) $(OBJ)

# dependencies
http-server.o: linklist.h handle_list.h get_post_page.h
linklist.o: linklist.h
handle_list.o: handle_list.h linklist.h
get_post_page.o: get_post_page.h linklist.h

# phony targets
.PHONY: clean cleanly all CLEAN
# `make clean` to remove all object files
# `make CLEAN` to remove all object and executable files
# `make cleanly` to `make` then immediately remove object files
clean:
	rm -f $(OBJ)
CLEAN: clean
	rm -f $(EXE)
cleanly: all clean
