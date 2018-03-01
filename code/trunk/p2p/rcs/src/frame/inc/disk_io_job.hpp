/*#############################################################################
 * �ļ���   : disk_io_job.hpp
 * ������   : teck_zhou	
 * ����ʱ�� : 2013��9��2��
 * �ļ����� : DiskIoJob����
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#ifndef HEADER_DISK_IO_JOB
#define HEADER_DISK_IO_JOB

#include "bc_typedef.hpp"
#include "rcs_typedef.hpp"

namespace BroadCache
{

class DiskIoThread;
class PieceManager;

// Job����ص�������Job�����߸���ʵ��
typedef boost::function<void(bool, const DiskIoJobSP&)> IoJobHandler;

/******************************************************************************
 * ����������IO�����ӿ�
 * ���ߣ�teck_zhou
 * ʱ�䣺2013/09/15
 *****************************************************************************/
INTERFACE DiskIoJob 
{
	DiskIoJob(io_service& ios, const TorrentSP& t, const IoJobHandler& handler);

	virtual void Do(DiskIoThread& io_thread) = 0;
	virtual ~DiskIoJob() {}

	io_service&  work_ios;
	TorrentSP	 torrent;
	IoJobHandler callback;
};

/******************************************************************************
 * ����: �Ӵ��̶�ȡTorrent��Դ����
 * ���ߣ�teck_zhou
 * ʱ�䣺2013/09/15
 *****************************************************************************/
struct ReadDataJob : public DiskIoJob,
	                 public boost::enable_shared_from_this<ReadDataJob>
{
	ReadDataJob(io_service& ios, 
		        const TorrentSP& t, 
				const IoJobHandler& handler, 
				char* buffer, 
				uint64 piece, 
				uint64 offset, 
				uint64 length);

	void Do(DiskIoThread& io_thread);
	
	uint64 piece;	// ��ȡ���ݵ�piece����
	uint64 offset;	// ��ȡ����λ��ƫ��
	uint64 len;		// ��ȡ���ݵĳ���
	char*  buf;		// ��ȡ���ݻ�����
};

template<class Stream>
inline Stream& operator<<(Stream& stream, const ReadDataJob& job)
{
	stream << "[" << job.piece << ":" << job.offset << ":" << job.len << "]";
	return stream;
}

/******************************************************************************
 * ����: ��Torrent��Դ����д�����
 * ���ߣ�teck_zhou
 * ʱ�䣺2013/09/15
 *****************************************************************************/
struct WriteDataJob : public DiskIoJob,
	                  public boost::enable_shared_from_this<WriteDataJob>
{
	WriteDataJob(io_service& ios, 
		         const TorrentSP& t, 
				 const IoJobHandler& handler, 
				 char* buffer, 
				 uint64 piece, 
				 uint64 offset, 
				 uint64 length);

	void Do(DiskIoThread& io_thread);

	uint64	piece;	// д�����ݵ�piece����
	uint64	offset;	// д������λ��ƫ��
	uint64	len;	// д�����ݳ���
	char*	buf;	// д�����ݻ�����
};

template<class Stream>
inline Stream& operator<<(Stream& stream, const WriteDataJob& job)
{
	stream << "[" << job.piece << ":" << job.offset << ":" << job.len << "]";
	return stream;
}

/******************************************************************************
 * ����: �Ӵ��̶�ȡFastResume����
 * ���ߣ�teck_zhou
 * ʱ�䣺2013/09/15
 *****************************************************************************/
struct WriteResumeJob : public DiskIoJob,
	                    public boost::enable_shared_from_this<WriteResumeJob>
{
	WriteResumeJob(io_service& ios, 
		           const TorrentSP& t, 
				   const IoJobHandler& handler, 
				   const std::string& data);

	virtual void Do(DiskIoThread& io_thread) override;

	std::string fast_resume;
};

/******************************************************************************
 * ����: ��FastResume����д�����
 * ���ߣ�teck_zhou
 * ʱ�䣺2013/09/15
 *****************************************************************************/
struct ReadResumeJob : public DiskIoJob,
	                   public boost::enable_shared_from_this<ReadResumeJob>
{
	ReadResumeJob(io_service& ios, const TorrentSP& t, const IoJobHandler& handler);

	virtual void Do(DiskIoThread& io_thread) override;

	std::string fast_resume;
};

/******************************************************************************
 * ����: �Ӵ��̶�ȡmetadata����
 * ���ߣ�teck_zhou
 * ʱ�䣺2013/10/18
 *****************************************************************************/
struct WriteMetadataJob : public DiskIoJob,
	                      public boost::enable_shared_from_this<WriteMetadataJob>
{
	WriteMetadataJob(io_service& ios, 
		             const TorrentSP& t, 
					 const IoJobHandler& handler, 
					 const std::string& data);

	virtual void Do(DiskIoThread& io_thread) override;

	std::string metadata;
};

/******************************************************************************
* ����: �����д��metadata����
* ���ߣ�teck_zhou
* ʱ�䣺2013/10/18
 *****************************************************************************/
struct ReadMetadataJob : public DiskIoJob,
	                     public boost::enable_shared_from_this<ReadMetadataJob>
{
	ReadMetadataJob(io_service& ios, const TorrentSP& t, const IoJobHandler& handler);

	virtual void Do(DiskIoThread& io_thread) override;

	std::string metadata;
};

/******************************************************************************
* ����: У��piece��hash
* ���ߣ�teck_zhou
* ʱ�䣺2013/11/06
 *****************************************************************************/
struct VerifyPieceInfoHashJob : public DiskIoJob,
	public boost::enable_shared_from_this<VerifyPieceInfoHashJob>
{
	VerifyPieceInfoHashJob(io_service& ios, 
		                   const TorrentSP& t, 
						   const IoJobHandler& handler, 
						   uint64 piece);

	virtual void Do(DiskIoThread& io_thread) override;

	uint64 piece_index;
};

}

#endif
