/*#############################################################################
 * �ļ���   : disk_io_job.cpp
 * ������   : teck_zhou	
 * ����ʱ�� : 2013��8��22��
 * �ļ����� : DiskIoJobʵ��
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#include "disk_io_job.hpp"
#include "disk_io_thread.hpp"
#include "io_oper_manager.hpp"
#include "torrent.hpp"

namespace BroadCache
{

/*-----------------------------------------------------------------------------
 * ��  ��: ���캯��
 * ��  ��: �ṹ�幹���������
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��08��21��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
DiskIoJob::DiskIoJob(io_service& ios, const TorrentSP& t, const IoJobHandler& handler) 
	: work_ios(ios), torrent(t), callback(handler)
{
}

//=============================================================================

/*-----------------------------------------------------------------------------
 * ��  ��: ���캯��
 * ��  ��: �ṹ�幹���������
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��08��21��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
ReadDataJob::ReadDataJob(io_service& ios, const TorrentSP& t, const IoJobHandler& handler, 
	char* buffer, uint64 piece, uint64 offset, uint64 length)
	: DiskIoJob(ios, t, handler)
	, piece(piece)
	, offset(offset)
	, len(length)
	, buf(buffer)
{
}

/*-----------------------------------------------------------------------------
 * ��  ��: ����ִ�ж����ݴ���
 * ��  ��: [in] io_thread ��Ҫ����DiskIoThread�ṩ�Ľӿ�
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��08��21��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void ReadDataJob::Do(DiskIoThread &io_thread)
{
	ReadDataJobSP read_data = shared_from_this();
	bool result = io_thread.ReadData(read_data);
	if (!result)
	{
		LOG(WARNING) << "Fail to read data frome disk";
	}
	
	// �ص�����
	work_ios.post(boost::bind(callback, result, shared_from_this()));
}

//=============================================================================

/*-----------------------------------------------------------------------------
 * ��  ��: ���캯��
 * ��  ��: �ṹ�幹���������
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��08��21��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
WriteDataJob::WriteDataJob(io_service& ios, const TorrentSP& t, const IoJobHandler& handler, 
	char* buffer, uint64 piece, uint64 offset, uint64 length)
	: DiskIoJob(ios, t, handler)
	, piece(piece)
	, offset(offset)
	, len(length)
	, buf(buffer)
{
}

/*-----------------------------------------------------------------------------
 * ��  ��: ����ִ��д�����ݴ���
 * ��  ��: [in] io_thread ��Ҫ����DiskIoThread�ṩ�Ľӿ�
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��08��21��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void WriteDataJob::Do(DiskIoThread &io_thread)
{
	bool result = io_thread.WriteData(shared_from_this());
	if (!result)
	{
		LOG(WARNING) << "Fail to write data to disk";
	}
	
	// �ص�����
	work_ios.post(boost::bind(callback, result, shared_from_this()));
}

//=============================================================================

/*-----------------------------------------------------------------------------
 * ��  ��: WriteResumeJob���캯��
 * ��  ��: [in]pm Piece����ָ��
 *         [in]handler ��job��ɺ�Ļص����� 
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.09.17
 *   ���� rosan
 *   ���� ����
 ----------------------------------------------------------------------------*/
WriteResumeJob::WriteResumeJob(io_service& ios, 
	const TorrentSP& t, const IoJobHandler& handler, const std::string& data)
    : DiskIoJob(ios, t, handler), fast_resume(data)
{
}

/*-----------------------------------------------------------------------------
 * ��  ��: дfast resume��Ϣ
 * ��  ��: [in]DiskIoThread δʹ��
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.09.17
 *   ���� rosan
 *   ���� ����
 ----------------------------------------------------------------------------*/
void WriteResumeJob::Do(DiskIoThread&)
{
    //��fast-resume��Ϣд���ļ���
    bool result = torrent->io_oper_manager()->WriteFastResume(fast_resume);
    if (!result) //д��ʧ��
	{
		LOG(WARNING) << "Fail to write fast-resume";
	}
	
	// �ص�����
	work_ios.post(boost::bind(callback, result, shared_from_this()));
}

//=============================================================================

/*-----------------------------------------------------------------------------
 * ��  ��: ReadResumeJob���캯��
 * ��  ��: [in]pm piece������
 *         [out]handler ��job��ɺ�Ļص�����
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.09.17
 *   ���� rosan
 *   ���� ����
 ----------------------------------------------------------------------------*/
ReadResumeJob::ReadResumeJob(io_service& ios, const TorrentSP& t, const IoJobHandler& handler)
    : DiskIoJob(ios, t, handler)
{
}

/*-----------------------------------------------------------------------------
 * ��  ��: ���ļ���ȡfast resume��Ϣ 
 * ��  ��: [in]DiskIoThread δʹ��
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.09.17
 *   ���� rosan
 *   ���� ����
 ----------------------------------------------------------------------------*/
void ReadResumeJob::Do(DiskIoThread&)
{
    //���ļ��ж�ȡfast-resume��bencode����������
    bool result = torrent->io_oper_manager()->ReadFastResume(fast_resume);
    if (!result) //��ȡʧ��
    { 
		LOG(WARNING) << "Fail to read fast-resume";
    }
	
	// �ص�����
	work_ios.post(boost::bind(callback, result, shared_from_this()));
}

//=============================================================================

/*-----------------------------------------------------------------------------
 * ��  ��: WriteMetadataJob���캯��
 * ��  ��: [in] pm piece������
 *         [in] handler ��job��ɺ�Ļص�����
 *         [in] buffer ��ȡ���ݻ�����
 *         [in] length ��ȡ�����ݳ���
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.09.17
 *   ���� rosan
 *   ���� ����
 ----------------------------------------------------------------------------*/
WriteMetadataJob::WriteMetadataJob(io_service& ios, const TorrentSP& t, 
	const IoJobHandler& handler, const std::string& data)
    : DiskIoJob(ios, t, handler)
	, metadata(data)
{
}

/*-----------------------------------------------------------------------------
 * ��  ��: ���ļ���ȡfast resume��Ϣ 
 * ��  ��: [in] DiskIoThread δʹ��
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.09.17
 *   ���� rosan
 *   ���� ����
 ----------------------------------------------------------------------------*/
void WriteMetadataJob::Do(DiskIoThread&)
{
    //���ļ��ж�ȡfast-resume��bencode����������
    bool result = torrent->io_oper_manager()->WriteMetadata(metadata);
    if (!result)
    { 
		LOG(WARNING) << "Fail to write metadata";
    }
	
	// �ص�����
	work_ios.post(boost::bind(callback, result, shared_from_this()));
}

//=============================================================================

/*-----------------------------------------------------------------------------
 * ��  ��: ReadMetadataJob���캯��
 * ��  ��: [in] pm piece������
 *         [in] handler ��job��ɺ�Ļص�����
 *         [in] buffer ��ȡ���ݻ�����
 *         [in] length ��ȡ�����ݳ���
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.09.17
 *   ���� rosan
 *   ���� ����
 ----------------------------------------------------------------------------*/
ReadMetadataJob::ReadMetadataJob(io_service& ios, const TorrentSP& t, 
	const IoJobHandler& handler)
    : DiskIoJob(ios, t, handler)
{
}

/*-----------------------------------------------------------------------------
 * ��  ��: ���ļ���ȡfast resume��Ϣ 
 * ��  ��: [in]DiskIoThread δʹ��
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.09.17
 *   ���� rosan
 *   ���� ����
 ----------------------------------------------------------------------------*/
void ReadMetadataJob::Do(DiskIoThread&)
{
    //���ļ��ж�ȡfast-resume��bencode����������
    bool result = torrent->io_oper_manager()->ReadMetadata(metadata);
    if (!result) //��ȡʧ��
    { 
		LOG(WARNING) << "Fail to read metadata";
    }
	
	// �ص�����
	work_ios.post(boost::bind(callback, result, shared_from_this()));
}

//=============================================================================

/*-----------------------------------------------------------------------------
 * ��  ��: ���캯��
 * ��  ��: [in] pm piece������
 *         [in] handler ��job��ɺ�Ļص�����
 *         [in] piece piece����
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��6��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
VerifyPieceInfoHashJob::VerifyPieceInfoHashJob(io_service& ios, const TorrentSP& t, 
	const IoJobHandler& handler, uint64 piece)
    : DiskIoJob(ios, t, handler)
	, piece_index(piece)
{
}

/*-----------------------------------------------------------------------------
 * ��  ��: �첽У��piece��info-hash
 * ��  ��: [in] io_thread DiskIoThread
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��6��
 *   ���� teck_zhou
 *   ���� ����
 ----------------------------------------------------------------------------*/
void VerifyPieceInfoHashJob::Do(DiskIoThread& io_thread)
{
    bool result = io_thread.VerifyPieceInfoHash(torrent, piece_index);
	if (!result)
	{
		LOG(ERROR) << "Fail to verify piece info-hash | " << piece_index;
	}
	
	// �ص�����
	work_ios.post(boost::bind(callback, result, shared_from_this()));
}

}
