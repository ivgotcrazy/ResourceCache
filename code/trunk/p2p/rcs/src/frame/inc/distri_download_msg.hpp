/*#############################################################################
 * 文件名   : distri_download_msg.hpp
 * 创建人   : teck_zhou	
 * 创建时间 : 2013年11月20日
 * 文件描述 : 分布式下载消息相关声明
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#ifndef HEADER_DISTRI_DOWNLOAD_MSG
#define HEADER_DISTRI_DOWNLOAD_MSG

namespace BroadCache
{

enum DistriDownloadMsgType
{
	DISTRI_DOWNLOAD_MSG_INVALID = 0,
	DISTRI_DOWNLOAD_MSG_ASSIGN_PIECE = 1,
	DISTRI_DOWNLOAD_MSG_ACCEPT_PIECE = 2,
	DISTRI_DOWNLOAD_MSG_REJECT_PIECE = 3
};

// 分布式下载消息通用格式
// <magic-number><length><type><data>      

// 1.assign-piece                                                                                                                            
//				4Bytes                    2Bytes       1Byte             4Bytes               
//|--------------------------------|----------------|--------|-------------------------------|
//|           0xFFFFC593           |        5       |    1   |         piece-index           |
//|--------------------------------|----------------|--------|-------------------------------|


// 2.accept-piece                                                                                                                                                                                     
//				4Bytes                    2Bytes       1Byte             4Bytes                       
//|--------------------------------|----------------|--------|-------------------------------|        
//|           0xFFFFC593           |        5       |    2   |         piece-index           |        
//|--------------------------------|----------------|--------|-------------------------------|        


// 3.reject-piece                                                                                         
//				4Bytes                    2Bytes       1Byte             4Bytes               
//|--------------------------------|----------------|--------|-------------------------------|
//|           0xFFFFC593           |        5       |    3   |         piece-index           |
//|--------------------------------|----------------|--------|-------------------------------|


}

#endif