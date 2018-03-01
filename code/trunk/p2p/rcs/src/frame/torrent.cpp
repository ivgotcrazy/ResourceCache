/*#############################################################################
 * �ļ���   : torrent.cpp
 * ������   : teck_zhou	
 * ����ʱ�� : 2013��8��8��
 * �ļ����� : Torrent��ʵ��
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
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
 * ��  ��: ���캯��
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��08��21��
 *   ���� teck_zhou
 *   ���� ����
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
 * ��  ��: ���캯��
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��08��21��
 *   ���� teck_zhou
 *   ���� ����
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
 * ��  ��: ���캯�����Դ�FastResume�ļ�����ʼ��Torrent�������ȡ�ļ�ʧ�������
 *         Э��ģ��ӿ�ȥ��ȡmetadata
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��08��21��
 *   ���� teck_zhou
 *   ���� ����
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

	Initialize(); // Э��ģ�鴦��

	return true;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ����������ʱ��
 * ��  ��: 
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��09��
 *   ���� teck_zhou
 *   ���� ����
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
 * ��  ��: �ж�torrent�Ƿ��ڷǻ�Ծ״̬
 * ��  ��: 
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��09��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool Torrent::IsInactive() const
{
	// ����proxy���͵�torrent��ֻҪ��������Ϊ0��ֱ��ɾ��
	if (is_proxy_ && policy_->GetProxyPeerConnNum() == 0) return true;

	return alive_time_without_conn_ >= max_alive_time_without_conn_;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��ʱ��������
 * ��  ��: 
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��08��21��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void Torrent::OnTick()
{
	policy_->OnTick();

	UpdateAliveTimeWithoutConn();

	TickProc();
	
	alive_time_++;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��ȡpiece�Ĵ�С��ע�⣬��Torrent��ȡ��Metadataǰ�����޷������
 * ��  ��: [in] index piece����
 * ����ֵ: piece��С, �ⲿ����У�飬��Ϊ��Զ��Ч
 * ��  ��:
 *   ʱ�� 2013��09��12��
 *   ���� teck_zhou
 *   ���� ����
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
 * ��  ��: ��ȡblock�Ĵ�С��ע�⣬��Torrent��ȡ��Metadataǰ�����޷������
 * ��  ��: [in] piece piece����
 *         [in] block block����
 * ����ֵ: block��С, �ⲿ����У�飬��Ϊ��Զ��Ч
 * ��  ��:
 *   ʱ�� 2013��09��12��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
uint64 Torrent::GetBlockSize(uint64 piece, uint64 block) const
{
	BC_ASSERT(is_ready_);
	BC_ASSERT(piece < torrent_info_.piece_map.size());

	size_t piece_size = GetPieceSize(piece);
	size_t block_size = torrent_info_.block_size;

	BC_ASSERT(block < CALC_FULL_MULT(piece_size, block_size));

	// ��piece�������ٸ�block
	size_t num_blocks = CALC_FULL_MULT(piece_size, block_size);

	// �����һ��block����piece�ĳ�����block���ȵ�������
	if ((block + 1 < num_blocks) || (piece_size % block_size == 0))
	{
		return block_size;
	}
	else // ���һ��block
	{
		return (piece_size % block_size);
	}
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��ȡpiece������
 * ��  ��:
 * ����ֵ: piece�������ⲿ����У�飬��Ϊ��Զ��Ч
 * ��  ��:
 *   ʱ�� 2013��09��12��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
uint64 Torrent::GetPieceNum() const
{
	BC_ASSERT(is_ready_);
	return torrent_info_.piece_map.size();
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��ȡpiece��block��
 * ��  ��: [in] piece piece����
 * ����ֵ: block����, �ⲿ����У�飬��Ϊ��Զ��Ч
 * ��  ��:
 *   ʱ�� 2013��09��12��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
uint64 Torrent::GetBlockNum(uint64 piece) const
{
	BC_ASSERT(is_ready_);
	BC_ASSERT(piece <= torrent_info_.piece_map.size());

	return CALC_FULL_MULT(GetPieceSize(piece), GetCommonBlockSize());
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��ȡ����piece��С
 * ��  ��: 
 * ����ֵ: piece��С, �ⲿ����У�飬��Ϊ��Զ��Ч
 * ��  ��:
 *   ʱ�� 2013��09��12��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
uint64 Torrent::GetCommonPieceSize() const
{
	BC_ASSERT(is_ready_);
	return torrent_info_.piece_size;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��ȡ����block��С
 * ��  ��: 
 * ����ֵ: block��С, �ⲿ����У�飬��Ϊ��Զ��Ч
 * ��  ��:
 *   ʱ�� 2013��09��12��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
uint64 Torrent::GetCommonBlockSize() const
{
	BC_ASSERT(is_ready_);
	return torrent_info_.block_size;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��ȡ���һ��piece��С
 * ��  ��: 
 * ����ֵ: piece��С, �ⲿ����У�飬��Ϊ��Զ��Ч
 * ��  ��:
 *   ʱ�� 2013��09��12��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
uint64 Torrent::GetLastPieceSize() const
{
	BC_ASSERT(is_ready_);

	int last_piece = GetPieceNum() - 1;

	BC_ASSERT(last_piece >= 0);

	return GetPieceSize(last_piece);
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��ȡpiece�����һ��block�Ĵ�С
 * ��  ��: 
 * ����ֵ: piece��С, �ⲿ����У�飬��Ϊ��Զ��Ч
 * ��  ��:
 *   ʱ�� 2013��09��12��
 *   ���� teck_zhou
 *   ���� ����
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
 * ��  ��: ����torrent������Ϣ
 * ��  ��: [in] torrent_info ��������
 * ����ֵ: 
 * ��  ��:
 *   ʱ�� 2013��10��14��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void Torrent::SetTorrentInfo(const TorrentInfo& torrent_info)
{
	BC_ASSERT(!is_ready_);

	torrent_info_ = torrent_info; // �����Ѿ�У��

	is_ready_ = true; // torrent׼��OK�����Խ����������ϴ������ˡ�

	// ���ڿ��Դ���PiecePicker��
	piece_picker_.reset(new PiecePicker(shared_from_this(), torrent_info_.piece_map));

	// ����ڻ�ȡmetadata֮ǰ���Ѿ����зֲ�ʽ����ͨ�ţ��˴���Ҫ����metadata��
	// ��ȡ��ʽ����ˢ����proxy��־�����metadata�Ǵ�fast-resume��ȡ�ģ���˵��
	// ����Դ֮ǰ�Ѿ����棬�����Ƿ��д������ӣ�torrent�����Ǵ�����Դ��
	if (is_proxy_ && torrent_info_.retrive_type != FROM_METADATA)
	{
		is_proxy_ = false;
	}
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��ʼ����
 * ��  ��: 
 * ����ֵ: 
 * ��  ��:
 *   ʱ�� 2013��10��14��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void Torrent::SetSeed()
{
    is_seed_ = true;

	BC_ASSERT(policy_);
	
	policy_->SetSeed();
}

/*-----------------------------------------------------------------------------
 * ��  ��: ����Ϊ�������
 * ��  ��: 
 * ����ֵ: 
 * ��  ��:
 *   ʱ�� 2013��11��20��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void Torrent::SetProxy()
{
	LOG(INFO) << "Set torrent's proxy flag | " << info_hash();

	BC_ASSERT(!is_proxy_);

	// �����ʱ��û�л�ȡ��metadata���������ñ�־���Ȼ�ȡmetadata����ˢ��
	if (!is_ready_)
	{
		is_proxy_ = true;
		return;
	}

	// �Ѿ���ȡ��metadata�����Ǵ������ȡ�ģ���˵��֮ǰδ�������Դ���Ǵ���Դ
	// �ض�Ϊproxy��Դ�����������Ҫ����Դ�Ӵ���ɾ����
	if (torrent_info_.retrive_type == FROM_METADATA)
	{
		is_proxy_ = true;
	}
}

/*-----------------------------------------------------------------------------
 * ��  ��: ������piece����
 * ��  ��: 
 * ����ֵ: 
 * ��  ��:
 *   ʱ�� 2013��11��12��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void Torrent::HavePiece(uint64 piece)
{
	policy_->HavePiece(piece);

	HavePieceProc(piece);
}

/*-----------------------------------------------------------------------------
 * ��  ��: ֹͣ��������  ������Դ��ؿ��� 
 * ��  ��: block  true������ false��ָ�
 * ����ֵ: 
 * ��  ��:
 *   ʱ�� 2013��11��2��
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
void Torrent::StopLaunchNewConnection()
{
	policy_->set_stop_new_connection_flag(true);
}

/*-----------------------------------------------------------------------------
 * ��  ��: �ָ���������  ������Դ��ؿ��� 
 * ��  ��: block  true������ false��ָ�
 * ����ֵ: 
 * ��  ��:
 *   ʱ�� 2013��11��2��
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
void Torrent::ResumeLaunchNewConnection()
{
	policy_->set_stop_new_connection_flag(false);
}


}

