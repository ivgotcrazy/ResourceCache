/*#############################################################################
 * 文件名   : torrent.cpp
 * 创建人   : teck_zhou	
 * 创建时间 : 2013年8月8日
 * 文件描述 : Torrent类实现
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#include "torrent.hpp"
#include <glog/logging.h>
#include <boost/bind.hpp>
#include "session.hpp"
#include "policy.hpp"
#include "io_oper_manager.hpp"
#include "storage.hpp"
#include "info_hash.hpp"
#include "bc_assert.hpp"
#include "piece_picker.hpp"
#include "rcs_config.hpp"
#include "timer.hpp"
#include "rcs_config_parser.hpp"
#include "rcs_util.hpp"

namespace BroadCache
{

/*-----------------------------------------------------------------------------
 * 描  述: 构造函数
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年08月21日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
Torrent::Torrent(Session& session, const InfoHashSP& hash, const fs::path& save_path) 
	: session_(session)
	, save_path_(save_path)
	, alive_time_(0)
	, is_ready_(false)
	, is_seed_(false)
	, is_proxy_(false)
	, alive_time_without_conn_(0)	
{
	torrent_info_.info_hash = hash;

	GET_RCS_CONFIG_INT("global.torrent.max-alive-time-without-connection", max_alive_time_without_conn_);
}

/*-----------------------------------------------------------------------------
 * 描  述: 构造函数
 * 参  数:
 * 返回值:
 * 修  改:
 *   时间 2013年08月21日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
Torrent::~Torrent()
{
	LOG(INFO) << ">>> Destructing torrent | " << info_hash();

	session_.io_thread().ClearWriteCache(this);

	if (is_proxy_)
	{
		io_oper_manager_->RemoveAllFiles();

		LOG(INFO) << "Remove all files of torrent | " << info_hash();
	}
}

/*-----------------------------------------------------------------------------
 * 描  述: 构造函数尝试从FastResume文件来初始化Torrent，如果读取文件失败则调用
 *         协议模块接口去获取metadata
 * 返回值:
 * 修  改:
 *   时间 2013年08月21日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool Torrent::Init()
{
	io_oper_manager_.reset(new IoOperManager(shared_from_this(), session_.io_thread()));
	if (!io_oper_manager_->Init())
	{
		LOG(ERROR) << "Fail to init io operation manager";
		return false;
	}

	policy_.reset(new Policy(shared_from_this()));

	timer_.reset(new Timer(session_.timer_ios(), boost::bind(&Torrent::OnTick, this), 1));
	timer_->Start();

	Initialize(); // 协议模块处理

	return true;
}

/*-----------------------------------------------------------------------------
 * 描  述: 更新无连接时间
 * 参  数: 
 * 返回值:
 * 修  改:
 *   时间 2013年11月09日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void Torrent::UpdateAliveTimeWithoutConn()
{
	if (policy_->GetPeerConnNum() == 0)
	{
		alive_time_without_conn_++;
	}
	else
	{
		alive_time_without_conn_ = 0;
	}
}

/*-----------------------------------------------------------------------------
 * 描  述: 判断torrent是否处于非活跃状态
 * 参  数: 
 * 返回值:
 * 修  改:
 *   时间 2013年11月09日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
bool Torrent::IsInactive() const
{
	// 对于proxy类型的torrent，只要代理连接为0就直接删除
	if (is_proxy_ && policy_->GetProxyPeerConnNum() == 0) return true;

	return alive_time_without_conn_ >= max_alive_time_without_conn_;
}

/*-----------------------------------------------------------------------------
 * 描  述: 定时器处理函数
 * 参  数: 
 * 返回值:
 * 修  改:
 *   时间 2013年08月21日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void Torrent::OnTick()
{
	policy_->OnTick();

	UpdateAliveTimeWithoutConn();

	TickProc();
	
	alive_time_++;
}

/*-----------------------------------------------------------------------------
 * 描  述: 获取piece的大小，注意，在Torrent获取到Metadata前，是无法计算的
 * 参  数: [in] index piece索引
 * 返回值: piece大小, 外部无需校验，认为永远有效
 * 修  改:
 *   时间 2013年09月12日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
uint64 Torrent::GetPieceSize(uint64 index) const
{
	BC_ASSERT(is_ready_);

	uint64 piece_num = CALC_FULL_MULT(torrent_info_.total_size, torrent_info_.piece_size);
	BC_ASSERT(index >= 0);
	BC_ASSERT(index < piece_num);

	if (index != piece_num - 1)
	{
		return torrent_info_.piece_size;
	}
	else
	{
		int diff = (torrent_info_.total_size -
				   (piece_num - 1) * torrent_info_.piece_size);
		BC_ASSERT(diff >= 0);
		return diff;
	}
}

/*-----------------------------------------------------------------------------
 * 描  述: 获取block的大小，注意，在Torrent获取到Metadata前，是无法计算的
 * 参  数: [in] piece piece索引
 *         [in] block block索引
 * 返回值: block大小, 外部无需校验，认为永远有效
 * 修  改:
 *   时间 2013年09月12日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
uint64 Torrent::GetBlockSize(uint64 piece, uint64 block) const
{
	BC_ASSERT(is_ready_);
	BC_ASSERT(piece < torrent_info_.piece_map.size());

	size_t piece_size = GetPieceSize(piece);
	size_t block_size = torrent_info_.block_size;

	BC_ASSERT(block < CALC_FULL_MULT(piece_size, block_size));

	// 此piece包含多少个block
	size_t num_blocks = CALC_FULL_MULT(piece_size, block_size);

	// 非最后一个block或者piece的长度是block长度的整数倍
	if ((block + 1 < num_blocks) || (piece_size % block_size == 0))
	{
		return block_size;
	}
	else // 最后一个block
	{
		return (piece_size % block_size);
	}
}

/*-----------------------------------------------------------------------------
 * 描  述: 获取piece总数量
 * 参  数:
 * 返回值: piece数量，外部无需校验，认为永远有效
 * 修  改:
 *   时间 2013年09月12日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
uint64 Torrent::GetPieceNum() const
{
	BC_ASSERT(is_ready_);
	return torrent_info_.piece_map.size();
}

/*-----------------------------------------------------------------------------
 * 描  述: 获取piece中block数
 * 参  数: [in] piece piece索引
 * 返回值: block数量, 外部无需校验，认为永远有效
 * 修  改:
 *   时间 2013年09月12日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
uint64 Torrent::GetBlockNum(uint64 piece) const
{
	BC_ASSERT(is_ready_);
	BC_ASSERT(piece <= torrent_info_.piece_map.size());

	return CALC_FULL_MULT(GetPieceSize(piece), GetCommonBlockSize());
}

/*-----------------------------------------------------------------------------
 * 描  述: 获取正常piece大小
 * 参  数: 
 * 返回值: piece大小, 外部无需校验，认为永远有效
 * 修  改:
 *   时间 2013年09月12日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
uint64 Torrent::GetCommonPieceSize() const
{
	BC_ASSERT(is_ready_);
	return torrent_info_.piece_size;
}

/*-----------------------------------------------------------------------------
 * 描  述: 获取正常block大小
 * 参  数: 
 * 返回值: block大小, 外部无需校验，认为永远有效
 * 修  改:
 *   时间 2013年09月12日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
uint64 Torrent::GetCommonBlockSize() const
{
	BC_ASSERT(is_ready_);
	return torrent_info_.block_size;
}

/*-----------------------------------------------------------------------------
 * 描  述: 获取最后一块piece大小
 * 参  数: 
 * 返回值: piece大小, 外部无需校验，认为永远有效
 * 修  改:
 *   时间 2013年09月12日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
uint64 Torrent::GetLastPieceSize() const
{
	BC_ASSERT(is_ready_);

	int last_piece = GetPieceNum() - 1;

	BC_ASSERT(last_piece >= 0);

	return GetPieceSize(last_piece);
}

/*-----------------------------------------------------------------------------
 * 描  述: 获取piece中最后一块block的大小
 * 参  数: 
 * 返回值: piece大小, 外部无需校验，认为永远有效
 * 修  改:
 *   时间 2013年09月12日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
uint64 Torrent::GetLastBlockSize(uint64 piece) const
{
	BC_ASSERT(is_ready_);
	BC_ASSERT(piece <= torrent_info_.piece_map.size());

	int last_block = GetBlockNum(piece);
	BC_ASSERT(last_block >= 0);

	return GetBlockSize(piece, last_block);
}

/*-----------------------------------------------------------------------------
 * 描  述: 设置torrent属性信息
 * 参  数: [in] torrent_info 属性数据
 * 返回值: 
 * 修  改:
 *   时间 2013年10月14日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void Torrent::SetTorrentInfo(const TorrentInfo& torrent_info)
{
	BC_ASSERT(!is_ready_);

	torrent_info_ = torrent_info; // 数据已经校验

	is_ready_ = true; // torrent准备OK，可以进行正常的上传下载了。

	// 终于可以创建PiecePicker了
	piece_picker_.reset(new PiecePicker(shared_from_this(), torrent_info_.piece_map));

	// 如果在获取metadata之前就已经进行分布式下载通信，此处需要根据metadata的
	// 获取方式重新刷新了proxy标志，如果metadata是从fast-resume获取的，则说明
	// 此资源之前已经缓存，不管是否有代理连接，torrent都不是代理资源。
	if (is_proxy_ && torrent_info_.retrive_type != FROM_METADATA)
	{
		is_proxy_ = false;
	}
}

/*-----------------------------------------------------------------------------
 * 描  述: 开始做种
 * 参  数: 
 * 返回值: 
 * 修  改:
 *   时间 2013年10月14日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void Torrent::SetSeed()
{
    is_seed_ = true;

	BC_ASSERT(policy_);
	
	policy_->SetSeed();
}

/*-----------------------------------------------------------------------------
 * 描  述: 设置为缓存代理
 * 参  数: 
 * 返回值: 
 * 修  改:
 *   时间 2013年11月20日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void Torrent::SetProxy()
{
	LOG(INFO) << "Set torrent's proxy flag | " << info_hash();

	BC_ASSERT(!is_proxy_);

	// 如果此时还没有获取到metadata，则先设置标志，等获取metadata后再刷新
	if (!is_ready_)
	{
		is_proxy_ = true;
		return;
	}

	// 已经获取到metadata，且是从网络获取的，则说明之前未缓存此资源，那此资源
	// 必定为proxy资源，代理完后，需要将资源从磁盘删除。
	if (torrent_info_.retrive_type == FROM_METADATA)
	{
		is_proxy_ = true;
	}
}

/*-----------------------------------------------------------------------------
 * 描  述: 下载完piece处理
 * 参  数: 
 * 返回值: 
 * 修  改:
 *   时间 2013年11月12日
 *   作者 teck_zhou
 *   描述 创建
 ----------------------------------------------------------------------------*/
void Torrent::HavePiece(uint64 piece)
{
	policy_->HavePiece(piece);

	HavePieceProc(piece);
}

/*-----------------------------------------------------------------------------
 * 描  述: 停止主动连接  用于资源监控控制 
 * 参  数: block  true表阻塞 false表恢复
 * 返回值: 
 * 修  改:
 *   时间 2013年11月2日
 *   作者 tom_liu
 *   描述 创建
 ----------------------------------------------------------------------------*/
void Torrent::StopLaunchNewConnection()
{
	policy_->set_stop_new_connection_flag(true);
}

/*-----------------------------------------------------------------------------
 * 描  述: 恢复主动连接  用于资源监控控制 
 * 参  数: block  true表阻塞 false表恢复
 * 返回值: 
 * 修  改:
 *   时间 2013年11月2日
 *   作者 tom_liu
 *   描述 创建
 ----------------------------------------------------------------------------*/
void Torrent::ResumeLaunchNewConnection()
{
	policy_->set_stop_new_connection_flag(false);
}


}

