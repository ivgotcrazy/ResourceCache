cc = g++
bin = main

p2p = /home/rosan/broadcache/code/trunk/p2p
pub = $(p2p)/pub
rcs = $(p2p)/rcs
rcs_src = $(rcs)/src
bt = $(rcs_src)/bt
frame = $(rcs_src)/frame
 
inc = -I$(pub)/inc \
      -I$(bt)/inc \
	  -I$(frame)/inc \

lib = -lboost_system \
	  -lboost_thread \
	  -lboost_regex \

flags = -std=c++0x -Wall

cppflags = $(flags) $(inc)

ccopt = 

src = main.cpp \
	  bt_msg.cpp \
	  $(pub)/client_socket.cpp \
	  $(pub)/bc_assert.cpp \
	  $(pub)/bc_util.cpp \
	  $(pub)/sha1.cpp \
	  $(bt)/bt_msg_recombiner.cpp \

obj = $(src:.cpp=.o)

$(bin) : $(obj)
	$(cc) -o $@ $(obj) $(ccopt) $(lib)

$(obj) : %.o : %.cpp
	$(cc) -o $@ -c $< $(cppflags)

clean:
	@rm -rf $(bin) $(obj)
