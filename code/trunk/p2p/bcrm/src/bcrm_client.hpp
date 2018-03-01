/*#############################################################################
 * 文件名   : bcrm_client.hpp
 * 创建人   : vicent_pan	
 * 创建时间 : 2013/10/19
 * 文件描述 : 远程监测诊断工具客户端相关类声明
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#ifndef HEADER_BCRM_CLIENT
#define HEADER_BCRM_CLIENT

#include "bcrm_message.pb.h"
#include "protobuf_msg_encode.hpp"
#include "depend.hpp"

#define CONNECTIONTIME    3          //用户读取指令buf 大小

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
 * 描述: bcrm 客户端 远程监测诊断工具
 * 作者：vicent_pan
 * 时间：2013/11/15
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
 * 描述: bcrm socket 客户端的实现类
 * 作者：vicent_pan
 * 时间：2013/11/15
 *****************************************************************************/
class BcrmSocketClient
{
public:
	explicit BcrmSocketClient(boost::asio::io_service& ios);
	bool Connect(const boost::asio::ip::tcp::endpoint & endpoint);		
	void DoSend();       //发送接口
	void DoReceive();    //接收接口
	void set_command(const PbMessage& msg);		
	inline std::vector<char> & info(){ return info_; }
	inline InfoHashMap & info_hash_map() { return info_hash_map_; }
	inline std::vector<char> & command(){ return command_; }
	inline boost::asio::ip::tcp::socket & socket(){ return socket_; }
private:
	std::vector<char> info_;                  //接收缓冲区 即接收server端的响应
	std::vector<char> command_;               //发送缓冲区 即发送指令到server端
	boost::asio::ip::tcp::socket socket_;
	InfoHashMap info_hash_map_; 
};

void ShowSize(uint32 width, uint64 size);
void ShowBandwidth(uint32 width, uint64 bandwidth);
void ShowTime(uint32 width, uint32 time);
void GetLine(std::string & line);

/******************************************************************************
 * 描述: bcrm  cmd处理类的基类
 * 作者：vicent_pan
 * 时间：2013/11/15
 *****************************************************************************/
class CmdHandler
{
public:
	virtual bool Match(const std::string& cmd) = 0;
	virtual void Excute(BcrmSocketClient & bsc) = 0;
};

/******************************************************************************
 * 描述: bcrm help命令处理类
 * 作者：vicent_pan
 * 时间：2013/11/15
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
 * 描述: bcrm exit命令处理类
 * 作者：vicent_pan
 * 时间：2013/11/15
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
 * 描述: bcrm show session 命令处理类
 * 作者：vicent_pan
 * 时间：2013/11/15
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
 * 描述: bcrm show torrent命令处理类
 * 作者：vicent_pan
 * 时间：2013/11/15
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
 * 描述: bcrm show peer命令处理类
 * 作者：vicent_pan
 * 时间：2013/11/15
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
 * 描述: bcrm show cache命令处理类
 * 作者：vicent_pan
 * 时间：2013/11/15
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
 * 描述: bcrm show system命令处理类
 * 作者：vicent_pan
 * 时间：2013/11/15
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

void signal_handler(int sig);          // 定义signal 的处理函数 为全局的

#endif  //HEADER_BCRM_CLIENT
