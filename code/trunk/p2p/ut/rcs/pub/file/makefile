# this makefile is just for running local unit test
# you should just copy this to your local ut directory
# make sure ut_dir and source files are correct 

# path to locate main.cpp
ut_dir = ../..

# path to locate source file
source_dir = ../../../src/pub

BIN = test
LIB = -lgtest -lboost_system
OBJ = $(ut_dir)/main.o $(source_dir)/file.o file_test.o

$(BIN) : $(OBJ)
	g++ -o $@ $(OBJ) $(LIB)

clean :
	@rm -f $(BIN) $(OBJ)
