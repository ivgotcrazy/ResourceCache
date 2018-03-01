# path define
prefix = ../src
main_dir = $(prefix)
frame_dir = $(prefix)/frame
ugs_pub_dir = $(prefix)/pub
bt_dir = $(prefix)/protocol/bt
pps_dir = $(prefix)/protocol/pps
pub_dir = ../../pub
message_dir = ../../protobuf/ugs-rcs

# pre-compile header file
#GCH_SOURCE = $(pub_dir)/inc/depend.hpp
#GCH = $(GCH_SOURCE).gch

# header file search path
INC = -I../.. \
	  -I../../pub \
	  -I../../pub/inc \
	  -I../src/frame/inc \
	  -I../src/pub/inc \
      -I../src/protocol/bt/inc \
      -I../src/protocol/pps/inc \
	  -I$(message_dir) \

# build flags
CC = ccache g++
BIN = ugs
LIB = -lboost_thread -lboost_system  -lboost_program_options  -lboost_filesystem \
      -lboost_regex -lglog -lprotobuf -lprotoc -lpcap
FLAGS = -g -rdynamic --std=c++0x
CCOPT = -O2 $(FLAGS)
CPPFLAGS = -Wall -gstabs+ -Winvalid-pch $(FLAGS) $(INC)

# pub source files
pub_src = $(pub_dir)/bc_assert.cpp \
		  $(pub_dir)/bc_util.cpp \
 		  $(pub_dir)/sha1.cpp \
		  $(pub_dir)/timer.cpp \
		  $(pub_dir)/basic_config_parser.cpp \
		  $(pub_dir)/protobuf_msg_encode.cpp \
		  $(pub_dir)/bencode_entry.cpp \
		  $(pub_dir)/server_socket.cpp \
		  $(pub_dir)/message_recombiner.cpp \
		  
# frame source files
frame_src = $(frame_dir)/session_manager.cpp \
	  		$(frame_dir)/session.cpp \
	  		$(frame_dir)/pkt_capturer.cpp \
			$(frame_dir)/communicator.cpp \
			$(frame_dir)/service_node_detector.cpp \
		    $(frame_dir)/tracker_manager.cpp \
			$(frame_dir)/hot_value_calculator.cpp \
			$(frame_dir)/hot_resource_manager.cpp \
			$(frame_dir)/access_record_database.cpp \
			$(frame_dir)/peer_pool.cpp \
			$(frame_dir)/rcs_mapper.cpp \
		  	
# UGS pub source files
ugs_pub_src = $(ugs_pub_dir)/resource_dispatcher.cpp \
			  $(ugs_pub_dir)/ugs_config_parser.cpp \
			  $(ugs_pub_dir)/mem_buf_pool.cpp \
			  $(ugs_pub_dir)/raw_data.cpp \
		      $(ugs_pub_dir)/pkt_processor.cpp \

# main source files
main_src = $(main_dir)/main.cpp \

# bt source files
bt_src = $(bt_dir)/bt_session.cpp \
		 $(bt_dir)/bt_packet_statistics.cpp \
		 $(bt_dir)/bt_packet_processor.cpp \

# pps source files
pps_src = $(pps_dir)/pps_session.cpp \
		  $(pps_dir)/pps_pkt_processor.cpp \
		  $(pps_dir)/pps_pkt_statistics.cpp \

# message files
message_src = $(message_dir)/message.pb.cpp \

# all source files
all_src = $(main_src) $(pub_src) $(frame_src) $(ugs_pub_src) $(bt_src) $(pps_src) $(message_src)

# all object files
OBJ = $(all_src:.cpp=.o)

# build excutable bin file
$(BIN) : $(OBJ)
	$(CC) $(CCOPT) -o $@ $(OBJ) $(LIB)

# generate dependency objects
$(OBJ) : %.o : %.cpp $(PRE_COMPILE_GCH)
	$(CC) -c $(CPPFLAGS) $< -o $@

# define gch dependency
#$(OBJ) : $(GCH)

# use pre-compiled header
#$(GCH) : $(GCH_SOURCE) 
#	$(CC) -o $(GCH) -x c++-header $(FLAGS) -c $(GCH_SOURCE) 

# clear for rebuild
clean :
	@rm -fr $(OBJ) $(BIN) $(GCH)
