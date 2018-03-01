/*#############################################################################
 * 文件名   : rcs_util.cpp
 * 创建人   : teck_zhou	
 * 创建时间 : 2013年11月13日
 * 文件描述 : 全局公共使用函数
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#include "rcs_util.hpp"
#include "disk_io_job.hpp"

namespace BroadCache
{

/*------------------------------------------------------------------------------
 * 描  述: 转换PeerRequest
 * 参  数: [in] read_job ReadDataJob
 * 返回值: PeerRequest
 * 修  改:
 *   时间 2013年11月04日
 *   作者 teck_zhou
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
PeerRequest ToPeerRequest(const ReadDataJob& read_job)
{
	PeerRequest request;

	request.piece = read_job.piece;
	request.start = read_job.offset;
	request.length = read_job.len;

	return request;
}

/*------------------------------------------------------------------------------
 * 描  述: 转换PeerRequest
 * 参  数: [in] write_job WriteDataJob
 * 返回值: PeerRequest
 * 修  改:
 *   时间 2013年11月07日
 *   作者 teck_zhou
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
PeerRequest ToPeerRequest(const WriteDataJob& write_job)
{
	PeerRequest request;

	request.piece = write_job.piece;
	request.start = write_job.offset;
	request.length = write_job.len;

	return request;
}

}
