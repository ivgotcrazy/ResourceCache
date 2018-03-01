
BIN = ut
LIB = -lgtest -lboost_system

# main
OBJ += main.o

#header file search path
CPPFLAGS = -I../pub/inc \
		   -I../rcs/src/frame/inc \
		   -std=c++0x \

# pub
pub_ut_dir = ./pub
pub_source_dir = ../pub

OBJ += $(pub_ut_dir)/file/file_test.o 
OBJ += $(pub_ut_dir)/serialize/serialize_test.o
OBJ += $(pub_ut_dir)/msg_factory/msg_factory_test.o
OBJ += $(pub_ut_dir)/msg_dispatcher/msg_dispatcher_test.o
OBJ += $(pub_ut_dir)/bencode/bencode_test.o
OBJ += $(pub_source_dir)/msg.o
OBJ += $(pub_source_dir)/msg_factory.o
OBJ += $(pub_source_dir)/msg_dispatcher.o
OBJ += $(pub_source_dir)/bencode.o
OBJ += $(pub_source_dir)/bencode_entry.o

# frame
frame_ut_dir = ./frame
frame_source_dir = ../rcs/src/frame

OBJ += $(frame_ut_dir)/piece_priority_strategy/piece_priority_strategy_test.o
OBJ += $(frame_ut_dir)/piece_picker/piece_picker_test.o
OBJ += $(frame_source_dir)/piece_priority_strategy.o
OBJ += $(frame_source_dir)/piece_picker.o

# bt
bt_ut_dir = ./bt
bt_source_dir = ../src/bt

OBJ +=


# build
$(BIN) : $(OBJ)
	g++ -o $@ $(OBJ) $(LIB) $(CPPFLAGS)

clean :
	@rm -f $(BIN) $(OBJ)
