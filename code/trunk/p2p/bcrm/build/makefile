# path define
prefix = ../src
protobuf_dir = ../../protobuf/bcrm
pub_dir = ../../pub
# header file search path
INC = -I../src \
	  -I../../pub \
	  -I../../pub/inc\
	  -I../../protobuf/bcrm\
	  
# build flags
CC = g++
BIN = bcrm
LIB = -lboost_thread -lboost_system  -lboost_program_options  -lboost_filesystem -lboost_regex -lglog -lprotobuf -lprotoc
FLAGS = -g -rdynamic --std=c++0x
CCOPT = -O2 $(FLAGS)
CPPFLAGS = -Wall -Winvalid-pch $(FLAGS) $(INC)

#pub source files      
pub_src = $(pub_dir)/bc_util.cpp\
		  $(pub_dir)/bc_assert.cpp\
		  $(pub_dir)/sha1.cpp\
		  $(pub_dir)/protobuf_msg_encode.cpp \
# source files
src = $(prefix)/bcrm_client.cpp \
      $(protobuf_dir)/bcrm_message.pb.cpp \

# main source files
main_src = $(prefix)/main.cpp

# all source files
all_src = $(main_src) $(src) $(pub_src) 

# all object files
OBJ = $(all_src:.cpp=.o)

# build excutable bin file
$(BIN) : $(OBJ)
	$(CC) $(CCOPT) -o $@ $(OBJ) $(LIB)

# generate dependency objects
$(OBJ) : %.o : %.cpp 
	$(CC) -c $(CPPFLAGS) $< -o $@

# clear for rebuild
clean :
	@rm -fr $(OBJ) $(BIN) 
