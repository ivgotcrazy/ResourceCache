/*#############################################################################
 * 文件名   : rcs_config.hpp
 * 创建人   : teck_zhou	
 * 创建时间 : 2013年8月21日
 * 文件描述 : 此文件包含了对RCS整个工程生效的配置选项，包括:
 *            1) 配置宏，配置常量，不适合放到配置文件中进行配置
 *            2) 编译宏，控制编译
 *            3) 全局宏或const常量
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#ifndef HEADER_RCS_CONFIG
#define HEADER_RCS_CONFIG

namespace BroadCache
{

// 配置文件路径
#define GLOBAL_CONFIG_PATH 	 "../config/rcs.conf"
#define BT_CONFIG_PATH   	 "../config/rcs-bt.conf"
#define XL_CONFIG_PATH   	 "../config/rcs-xl.conf"
#define PPS_CONFIG_PATH   	 "../config/rcs-pps.conf"
#define PPL_CONFIG_PATH   	 "../config/rcs-ppl.conf"
#define IP_FILTER_PATH       "../config/ipfilter.conf"

// IO读/写缓存相关配置
#define MAX_READ_CACHE_NUM	1024
#define MAX_WRITE_CACHE_NUM 1024

// 内存池相关配置 - MB: memory buffer
#define MB_DATA_BLOCK_SIZE	16 * 1024
#define MB_MSG_COMMON_SIZE	16 * 1024
#define MB_MSG_DATA_SIZE	17 * 1024
#define MB_MAX_NUM			 8 * 1024

// IO工作队列

// BLOCK大小，理论上BLOCK大小是与协议相关的，这里先从简处理，
// 认为所有协议定义的BLOCK大小都是相同的，单位是Byte
#define BT_BLOCK_SIZE		16 * 1024
#define DEFAULT_FRAGMENT_SIZE	16 * 1024  //调试PPS,暂时去掉这项

#define PPS_FRAGMENT_SIZE	1 * 1024

#define PPS_BLOCK_SIZE		16 * 1024

#define UDP_MAX_PACKET_LEN  (2 * 1024) 

// 断言开关
#ifndef BC_DEBUG
#define BC_DEBUG
#endif

// 每次从PiecePicker申请下载的block数量
#define APPLY_DOWNLOAD_BLOCK_NUM	1

// 分布式下载报文消息魔术字
#define DISTRIBUTED_DOWNLOAD_MAGIC_NUMBER 0xFFFFC593


}

#endif
