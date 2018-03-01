# path define
prefix = ../src
main_dir = $(prefix)
frame_dir = $(prefix)/frame
rcs_pub_dir = $(prefix)/pub
bt_dir = $(prefix)/protocol/bt
pps_dir = $(prefix)/protocol/pps
pub_dir = ../../pub
pb_msg_dir = ../../protobuf/ugs-rcs
bcrm_pb_msg_dir = ../../protobuf/bcrm

# pre-compile header file
GCH_SOURCE = $(pub_dir)/inc/depend.hpp
GCH = $(GCH_SOURCE).gch

# header file search path
INC = -I../.. \
	  -I$(pub_dir)/inc \
	  -I$(frame_dir)/inc \
	  -I$(bt_dir)/inc \
      -I$(pps_dir)/inc \
	  -I$(rcs_pub_dir)/inc \
	  -I$(pb_msg_dir) \
	  -I$(bcrm_pb_msg_dir)\

# build flags
CC = ccache g++
BIN = rcs
LIB = -lboost_thread -lboost_system  -lboost_program_options  -lboost_filesystem -lboost_regex -lglog -lprotobuf -lprotoc
FLAGS = -g  -rdynamic --std=c++0x
CCOPT =  -O2  $(FLAGS)
CPPFLAGS =  -Wall -gstabs+ -Winvalid-pch $(FLAGS) $(INC)

# pub source files
pub_src = $(pub_dir)/file.cpp \
		  $(pub_dir)/client_socket.cpp \
		  $(pub_dir)/server_socket.cpp \
		  $(pub_dir)/bc_assert.cpp \
		  $(pub_dir)/bencode_entry.cpp \
		  $(pub_dir)/sha1.cpp \
          $(pub_dir)/hasher.cpp \
		  $(pub_dir)/bc_util.cpp \
		  $(pub_dir)/lazy_entry.cpp \
		  $(pub_dir)/bencode.cpp \
		  $(pub_dir)/socket_connection.cpp \
		  $(pub_dir)/socket_server.cpp \
		  $(pub_dir)/basic_monitor.cpp \
		  $(pub_dir)/timer.cpp \
		  $(pub_dir)/message_recombiner.cpp \
		  $(pub_dir)/protobuf_msg_encode.cpp \
		  $(pub_dir)/basic_config_parser.cpp \
		  $(pub_dir)/pkt_recombiner.cpp \
		  $(pub_dir)/client_socket_ex.cpp \
		  
#protobuf source files
protobuf_src = $(bcrm_pb_msg_dir)/bcrm_message.pb.cpp
		  
# frame source files
frame_src = $(frame_dir)/session_manager.cpp \
	  		$(frame_dir)/session.cpp \
	  		$(frame_dir)/disk_io_thread.cpp \
			$(frame_dir)/storage.cpp \
			$(frame_dir)/peer_connection.cpp \
			$(frame_dir)/info_hash.cpp \
			$(frame_dir)/p2p_fsm.cpp \
			$(frame_dir)/torrent.cpp \
		    $(frame_dir)/piece_picker.cpp \
			$(frame_dir)/io_oper_manager.cpp \
			$(frame_dir)/tracker_request_manager.cpp \
			$(frame_dir)/tracker_request.cpp \
			$(frame_dir)/peer.cpp \
			$(frame_dir)/policy.cpp \
			$(frame_dir)/disk_io_job.cpp \
			$(frame_dir)/communicator.cpp \
			$(frame_dir)/mem_buffer_pool.cpp \
			$(frame_dir)/piece_priority_strategy.cpp \
			$(frame_dir)/traffic_controller.cpp \
			$(frame_dir)/monitor_manager.cpp \
			$(frame_dir)/bcrm_server.cpp \
			$(frame_dir)/get_bcrm_info.cpp \
			$(frame_dir)/distri_download_mgr.cpp \
			$(frame_dir)/distri_download_msg_recognizer.cpp \
		 
# rcs pub source files 
rcs_pub_src = $(rcs_pub_dir)/rcs_config_parser.cpp \
			  $(rcs_pub_dir)/ip_filter.cpp \
			  $(rcs_pub_dir)/rcs_util.cpp
	
# main source files
main_src = $(main_dir)/main.cpp

# bt source files
bt_src = $(bt_dir)/bt_tracker_request.cpp \
		 $(bt_dir)/bt_session.cpp \
		 $(bt_dir)/bt_torrent.cpp \
		 $(bt_dir)/bt_peer_connection.cpp \
		 $(bt_dir)/bt_fsm.cpp \
		 $(bt_dir)/bt_metadata_retriver.cpp \
		 $(bt_dir)/bt_msg_recognizer.cpp \
		 $(bt_dir)/bt_udp_tracker_request.cpp \
		 
# pps source files
pps_src = $(pps_dir)/pps_tracker_request.cpp \
		 $(pps_dir)/pps_session.cpp \
		 $(pps_dir)/pps_pub.cpp \
		 $(pps_dir)/pps_torrent.cpp \
		 $(pps_dir)/pps_peer_connection.cpp \
		 $(pps_dir)/pps_fsm.cpp \
		 $(pps_dir)/pps_metadata_retriver.cpp \
		 $(pps_dir)/pps_msg_recognizer.cpp \
		 $(pps_dir)/pps_baseinfo_tracker_request.cpp \
		 $(pps_dir)/pps_baseinfo_retriver.cpp \

		 
#protobuf message files
pb_msg_src = $(pb_msg_dir)/message.pb.cpp \

# all source files
all_src = $(main_src) $(pub_src) $(frame_src) $(rcs_pub_src) $(bt_src) $(pps_src) $(pb_msg_src)$(protobuf_src)

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
