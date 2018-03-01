/*#############################################################################
 * 文件名   : mem_buf_pool.hpp
 * 创建人   : teck_zhou	
 * 创建时间 : 2013年11月14日
 * 文件描述 : 内存池声明
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#ifndef HEADER_MEM_BUF_POOL
#define HEADER_MEM_BUF_POOL

#include <boost/noncopyable.hpp>
#include <boost/pool/pool.hpp>
#include "bc_typedef.hpp"

namespace BroadCache
{

/******************************************************************************
 * 描述: 内存池
 * 作者：teck_zhou
 * 时间：2013/11/14
 *****************************************************************************/
class MemBufPool : public boost::noncopyable
{
public:
	static MemBufPool& GetInstance();

	char* AllocPktBuf();
	void FreePktBuf(const char* buf);

private:
	MemBufPool();

private:
	boost::mutex pkt_mutex_;
	boost::pool<> pkt_buf_pool_;
};

}

#endif