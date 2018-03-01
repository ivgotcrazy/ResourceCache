/*#############################################################################
 * 文件名   : depend.hpp
 * 创建人   : teck_zhou	
 * 创建时间 : 2013-08-24
 * 文件描述 : 预编译头文件，公共头文件包含都应该放在此处，能够加快编译速度
 * ##########################################################################*/

#ifndef HEADER_DEPEND
#define HEADER_DEPEND

// glog
#include <glog/logging.h>

//protobuf
#include <google/protobuf/message.h>

// std
#include <map>
#include <set>
#include <list>
#include <queue>
#include <string>
#include <vector>
#include <utility>


// boost
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/type.hpp>
#include <boost/regex.hpp>
#include <boost/cstdint.hpp>
#include <boost/foreach.hpp>
#include <boost/function.hpp>
#include <boost/pool/pool.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/filesystem.hpp>
#include <boost/noncopyable.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/shared_array.hpp>
#include <boost/scoped_array.hpp>
#include <boost/intrusive_ptr.hpp>
#include <boost/thread/thread.hpp>
#include <boost/unordered_set.hpp>
#include <boost/unordered_map.hpp>
#include <boost/typeof/typeof.hpp>
#include <boost/dynamic_bitset.hpp>
#include <boost/program_options.hpp>
#include <boost/system/error_code.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/date_time/posix_time/ptime.hpp>



// linux
//#include <sys/uio.h>

#endif
