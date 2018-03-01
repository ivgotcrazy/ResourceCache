#
# path define
#
#prefix = ../../../../../src
prefix = ../../../src

#
# build flags
#
CC = g++
BIN = main
LIB = -lboost_thread -lboost_system  -lboost_program_options  -lglog -lgtest
CCOPT = -O2
CFLAGS = $(CCOPT)

all_src = test_config_parser.cpp  $(prefix)/pub/config_parser.cpp

#
# all object files
#
OBJ = $(all_src:.cpp=.o)

#
# build excutable bin file
#
$(BIN) : $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ) $(LIB)

#
# generate dependency objects
#
$(OBJ) : %.o : %.cpp
	$(CC) -c -std=c++0x -I$(prefix) $< -o $@

clean :
	@rm -fr $(OBJ) $(BIN)
