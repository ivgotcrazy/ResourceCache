/*#############################################################################
 * 文件名   : pkt_recombiner.hpp
 * 创建人   : teck_zhou	
 * 创建时间 : 2013年11月19日
 * 文件描述 : PktRecombiner声明
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#ifndef HEADER_PKT_RECOMBINER
#define HEADER_PKT_RECOMBINER

#include <vector>
#include <boost/noncopyable.hpp>
#include "bc_typedef.hpp"
#include "rcs_typedef.hpp"

namespace BroadCache
{

class MsgRecognizer;

/******************************************************************************
 * 描述: 报文重组器
 * 作者：teck_zhou
 * 时间：2013/11/19
 *****************************************************************************/
class PktRecombiner : public boost::noncopyable
{
public:
	typedef boost::function<void(const char*, uint32)> MsgProcessor;

public:
	PktRecombiner();

	void RegisterMsgRecognizer(MsgRecognizerSP recognizer, MsgProcessor processor);

	void AppendData(const char* buf, uint32 len);

	void AppendUdpData(const char* buf, uint32 len);

private:
	void ProcessPkt();
	
	void ProcessUdpPkt();

private:

	struct MsgParseEntry
	{
		MsgRecognizerSP recognizer;
		MsgProcessor processor;
	};

	typedef std::vector<MsgParseEntry> MsgParseEntryVec;
	typedef MsgParseEntryVec::iterator MsgParseEntryVecIter;

	class PktBuf;

private:
	MsgParseEntryVec msg_parse_entries_;

	boost::scoped_ptr<PktBuf> pkt_buf_;
};

/******************************************************************************
 * 描述: 报文重组缓冲区
 * 作者：teck_zhou
 * 时间：2013/11/19
 *****************************************************************************/
class PktRecombiner::PktBuf : public boost::noncopyable
{
public:
	PktBuf();

	void AppendData(const char* buf, uint32 len);
	void RetriveData(uint32 len);

	void AppendUdpData(const char* buf, uint32 len);

	const char* GetBuf();
	uint32 GetBufSize();

private:
	std::vector<char> buf_;
	uint32 read_index_;
	uint32 buf_size_;
};

}

#endif
