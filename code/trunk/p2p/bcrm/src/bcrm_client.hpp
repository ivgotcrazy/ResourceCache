/*#############################################################################
 * �ļ���   : bcrm_client.hpp
 * ������   : vicent_pan	
 * ����ʱ�� : 2013/10/19
 * �ļ����� : Զ�̼����Ϲ��߿ͻ������������
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#ifndef HEADER_BCRM_CLIENT
#define HEADER_BCRM_CLIENT

#include "bcrm_message.pb.h"
#include "protobuf_msg_encode.hpp"
#include "depend.hpp"

#define CONNECTIONTIME    3          //�û���ȡָ��buf ��С

namespace BroadCache
{

typedef boost::uint32_t  uint32;
typedef boost::int32_t   int32;
typedef boost::uint64_t  uint64;
typedef int msg_seq_t;
typedef google::protobuf::Message PbMessage;
typedef boost::unordered_map<int, std::string> InfoHashMap;

class CmdHandler;
class BcrmSocketClient;

/******************************************************************************
 * ����: bcrm �ͻ��� Զ�̼����Ϲ���
 * ���ߣ�vicent_pan
 * ʱ�䣺2013/11/15
 *****************************************************************************/
class BcrmClient
{
public:
	void Run(const char * data);
private:
	void InitMatchCommand();
	void ManageInCommand(const std::string & remote,  BcrmSocketClient & bsc);
private:
    std::list<CmdHandler*> cmd_handlers_; 
};

/******************************************************************************
 * ����: bcrm socket �ͻ��˵�ʵ����
 * ���ߣ�vicent_pan
 * ʱ�䣺2013/11/15
 *****************************************************************************/
class BcrmSocketClient
{
public:
	explicit BcrmSocketClient(boost::asio::io_service& ios);
	bool Connect(const boost::asio::ip::tcp::endpoint & endpoint);		
	void DoSend();       //���ͽӿ�
	void DoReceive();    //���սӿ�
	void set_command(const PbMessage& msg);		
	inline std::vector<char> & info(){ return info_; }
	inline InfoHashMap & info_hash_map() { return info_hash_map_; }
	inline std::vector<char> & command(){ return command_; }
	inline boost::asio::ip::tcp::socket & socket(){ return socket_; }
private:
	std::vector<char> info_;                  //���ջ����� ������server�˵���Ӧ
	std::vector<char> command_;               //���ͻ����� ������ָ�server��
	boost::asio::ip::tcp::socket socket_;
	InfoHashMap info_hash_map_; 
};

void ShowSize(uint32 width, uint64 size);
void ShowBandwidth(uint32 width, uint64 bandwidth);
void ShowTime(uint32 width, uint32 time);
void GetLine(std::string & line);

/******************************************************************************
 * ����: bcrm  cmd������Ļ���
 * ���ߣ�vicent_pan
 * ʱ�䣺2013/11/15
 *****************************************************************************/
class CmdHandler
{
public:
	virtual bool Match(const std::string& cmd) = 0;
	virtual void Excute(BcrmSocketClient & bsc) = 0;
};

/******************************************************************************
 * ����: bcrm help�������
 * ���ߣ�vicent_pan
 * ʱ�䣺2013/11/15
 *****************************************************************************/
class HelpCmdHandler: public CmdHandler 
{
 public:
	bool Match(const std::string& cmd) override;
	void Excute(BcrmSocketClient & bsc) override;
 private:
 	void Help();
};

/******************************************************************************
 * ����: bcrm exit�������
 * ���ߣ�vicent_pan
 * ʱ�䣺2013/11/15
 *****************************************************************************/
class ExitCmdHandler: public CmdHandler
{
public:
	bool Match(const std::string& cmd) override;
	void Excute(BcrmSocketClient & bsc) override;
private:
	void Exit(BcrmSocketClient & bsc);
};

/******************************************************************************
 * ����: bcrm show session �������
 * ���ߣ�vicent_pan
 * ʱ�䣺2013/11/15
 *****************************************************************************/
class ShowSessionCmdHandler: public CmdHandler 
{
public:
	bool Match(const std::string& cmd) override;
	void Excute(BcrmSocketClient & bsc) override;
	RequestSessionMsg GetMsg(BcrmSocketClient & bsc);
private:
	void ShowAllSession(BcrmSocketClient & bsc);
	void ShowRealAllSession(uint32 time, BcrmSocketClient & bsc);
private:
	uint32 time_;
};

/******************************************************************************
 * ����: bcrm show torrent�������
 * ���ߣ�vicent_pan
 * ʱ�䣺2013/11/15
 *****************************************************************************/
class ShowTorrentCmdHandler: public CmdHandler 
{
public:
	bool Match(const std::string& cmd) override;
	void Excute(BcrmSocketClient & bsc) override;
	RequestTorrentMsg GetMsg(BcrmSocketClient & bsc);
private:
	void ShowTorrentPreSession(BcrmSocketClient & bsc);
	void ShowRealTorrentPreSession(uint32 time, BcrmSocketClient & bsc);
private:
	uint32 time_;
	std::string session_type_;
};

/******************************************************************************
 * ����: bcrm show peer�������
 * ���ߣ�vicent_pan
 * ʱ�䣺2013/11/15
 *****************************************************************************/
class ShowPeerCmdHandler: public CmdHandler 
{
public:
	bool Match(const std::string& cmd) override;
	void Excute(BcrmSocketClient & bsc) override;
	RequestPeerMsg GetMsg(BcrmSocketClient & bsc);
private:	
	void ShowPeerPreTorrent(BcrmSocketClient & bsc);
	void ShowRealPeerPreTorrent(uint32 time, BcrmSocketClient & bsc);

private:
	uint32 time_;
	std::string session_type_;
	uint32 torrent_id_;
};

/******************************************************************************
 * ����: bcrm show cache�������
 * ���ߣ�vicent_pan
 * ʱ�䣺2013/11/15
 *****************************************************************************/
class ShowCacheCmdHandler: public CmdHandler 
{
public:
	bool Match(const std::string& cmd) override;
	void Excute(BcrmSocketClient & bsc) override;
	RequestCacheStatMsg GetMsg(BcrmSocketClient & bsc);
private:
	void ShowCacheStatus(BcrmSocketClient & bsc);
	void ShowRealCacheStatus(uint32 time, BcrmSocketClient & bsc);
private:
	uint32 time_;
	std::string session_type_;
};

/******************************************************************************
 * ����: bcrm show system�������
 * ���ߣ�vicent_pan
 * ʱ�䣺2013/11/15
 *****************************************************************************/
class ShowSystemCmdHandler: public CmdHandler 
{
public:
	bool Match(const std::string& cmd) override;
	void Excute(BcrmSocketClient & bsc)override;
	RequestSystemMsg GetMsg(BcrmSocketClient & bsc);
private:
	void ShowSystem(BcrmSocketClient & bsc);
	void ShowRealSystem(uint32 time, BcrmSocketClient & bsc);
	void ShowCpuInfo(BcrmSystemInfo& sys);
	void ShowMemInfo(BcrmSystemInfo& sys);
	void ShowDiskInfo(BcrmSystemInfo& sys);
	void ShowNetInfo(BcrmSystemInfo& sys);
private:
	uint32 time_;
};

}  //namespace BroadCache end

void signal_handler(int sig);          // ����signal �Ĵ����� Ϊȫ�ֵ�

#endif  //HEADER_BCRM_CLIENT
