/*#############################################################################
 * �ļ���   : bcrm_client.hpp
 * ������   : vicent_pan	
 * ����ʱ�� : 2013/10/19
 * �ļ����� : Զ�̼����Ϲ��߿ͻ��������ʵ��
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#include "bcrm_client.hpp"

#include <unistd.h>

#include <ctime>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <csignal>
#include <iomanip>

#include "bc_util.hpp"

static bool signal_ready = true;            //���ڽ���signal���˳�ѭ��

static const std::string kPeerClientType[6] = 
	{ "UN", "BC", "XL", "BT", "UT", "BT" };

namespace BroadCache
{

/*-----------------------------------------------------------------------------
 * ��  ��: ��������Run ����bcrm�ͻ���
 * ��  ��: [in] data    main����ȡ���û�������ַ���
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��24��
 *   ���� vincent_pan
 *   ���� ����
 ----------------------------------------------------------------------------*/
void BcrmClient::Run(const char * data)
{	
	std::string addr_str(data);
	auto i = addr_str.find(':');
	if ((i == 0) || (i == std::string::npos) || (i == addr_str.size() - 1))
	{
		std::cout << "Server address format error."<<std::endl;
		return ;
	}
	std::string ips(addr_str.substr(0,i));
    boost::asio::ip::tcp::endpoint endpoint (
		boost::asio::ip::address::from_string(ips),
		boost::lexical_cast<unsigned long> (addr_str.substr(i+1,addr_str.size())));
	
	boost::asio::io_service ios;
	BcrmSocketClient bsc(ios);

	if(bsc.Connect(endpoint)) 
	{	
		InitMatchCommand();
		while (true) ManageInCommand(ips,bsc);
	}
	else 
	{
		std::cout << "Failed to connect bcrm server, please make sure bcrm server is runnig." <<std::endl;
		return ;
	}
}

/*-----------------------------------------------------------------------------
 * ��  ��: �����û�����ָ�������ʾ����
 * ��  ��: [in] remote  ������ip
 		   [in][out] bsc
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��24��
 *   ���� vincent_pan
 *   ���� ����
 ----------------------------------------------------------------------------*/
void BcrmClient::ManageInCommand(const std::string &remote, BcrmSocketClient & bsc)
{	
	std::cout<< "bcrm@"<<remote<<">";
	std::string line;
	GetLine(line);
	if(line.empty()) return ;
	for (CmdHandler* cmd_handler : cmd_handlers_)
    {
        if ( cmd_handler->Match(line))
        {	
			cmd_handler->Excute(bsc);
            break;
        }
    }
}

/*-----------------------------------------------------------------------------
 * ��  ��: ע��command
 * ��  ��: 
 * ����ֵ:  
 * ��  ��:
 *   ʱ�� 2013��11��11��
 *   ���� vincent_pan
 *   ���� ����
 ----------------------------------------------------------------------------*/
void BcrmClient::InitMatchCommand()
{	
	cmd_handlers_.push_back(new HelpCmdHandler());
	cmd_handlers_.push_back(new ExitCmdHandler());
	cmd_handlers_.push_back(new ShowSessionCmdHandler());
	cmd_handlers_.push_back(new ShowTorrentCmdHandler());
	cmd_handlers_.push_back(new ShowPeerCmdHandler());
	cmd_handlers_.push_back(new ShowSystemCmdHandler());
	cmd_handlers_.push_back(new ShowCacheCmdHandler());	
}

/*-----------------------------------------------------------------------------
 * ��  ��: BcrmSocketClient ���캯��
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��24��
 *   ���� vincent_pan
 *   ���� ����
 ----------------------------------------------------------------------------*/
BcrmSocketClient::BcrmSocketClient(boost::asio::io_service& ios):socket_(ios)
{ 
}

/*-----------------------------------------------------------------------------
 * ��  ��: socket �ͻ������Ӻ��� ���ӷ�����
 * ��  ��: [in] endpoint �������˵�ip��ַ��port
 * ����ֵ: bool  true ���ӳɹ� false ����ʧ��
 * ��  ��:
 *   ʱ�� 2013��10��24��
 *   ���� vincent_pan
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool BcrmSocketClient::Connect(const boost::asio::ip::tcp::endpoint & endpoint)
{	
	uint32 i = 0;
	boost::system::error_code error;
	for(; i < CONNECTIONTIME; ++i)      // �������κ��ʧ�� ����ʧ�ܡ�
	{	
		socket_.connect(endpoint,error);  
		if(!error) break;
	}

	if (CONNECTIONTIME == i) 
	{	
		return false;
	}
	return true;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ���ͽӿ�
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��24��
 *   ���� vincent_pan
 *   ���� ����
 ----------------------------------------------------------------------------*/
void BcrmSocketClient::DoSend()
{	
	boost::system::error_code error;
	socket_.send(boost::asio::buffer(command_), 0, error);
	if(error) 
	{
		std::cout<< "Failed to send msg to server, please check the socket is connected..." <<std::endl;
		std::exit(0);
	}
}

/*-----------------------------------------------------------------------------
 * ��  ��: ���սӿ�
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��24��
 *   ���� vincent_pan
 *   ���� ����
 ----------------------------------------------------------------------------*/
void BcrmSocketClient::DoReceive()
{
	socket_.receive(boost::asio::null_buffers());
	uint32 available = socket_.available();
	if(!available) 
	{
		std::cout<< "Bad msg received, please check the socket is connected..." <<std::endl;
		std::exit(0);
	}
	info_.clear();
	info_.resize(available);
	socket_.receive(boost::asio::buffer(info_));
}

/*-----------------------------------------------------------------------------
 * ��  ��: ����command_
 * ��  ��: [in] msg
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��24��
 *   ���� vincent_pan
 *   ���� ����
 ----------------------------------------------------------------------------*/
void BcrmSocketClient::set_command(const PbMessage & msg)
{	                               	
	uint32 command_length = GetProtobufMessageEncodeLength(msg);
	command_.clear();
	command_.resize(command_length); 
	EncodeProtobufMessage(msg, &command_[0], command_length);
}
	
/*-----------------------------------------------------------------------------
 * ��  ��: �ϴ������ش�С�������ʾ
 * ��  ��: [in] width
 		   [in] size		  
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��8��
 *   ���� vincent_pan
 *   ���� ����
 ----------------------------------------------------------------------------*/
void ShowSize(uint32 width, uint64 size)
{
	uint32 flag = 0;
	while(true)
	{
		if (size < 1024)break;      // 1024 = 1k ����
		size = size / 1024;
		flag ++;
	}
	std::string size_to_str;
	
	if (flag == 0)
	{
		size_to_str = boost::lexical_cast<std::string>(size);
		size_to_str += "B";
		std::cout <<std::setw(width)<<size_to_str;
	}
			 
	if (flag == 1)
	{
		size_to_str = boost::lexical_cast<std::string>(size);
		size_to_str += "KB";
		std::cout <<std::setw(width)<<size_to_str;
	}
			
	if (flag == 2)
	{
		size_to_str = boost::lexical_cast<std::string>(size);
		size_to_str += "MB";
		std::cout <<std::setw(width)<<size_to_str;
	}
			
	if (flag >= 3)
	{
		size_to_str = boost::lexical_cast<std::string>(size);
		size_to_str += "GB";
		std::cout <<std::setw(width)<<size_to_str;
	}
}

/*-----------------------------------------------------------------------------
 * ��  ��: �ϴ������ش���������ʾ
 * ��  ��: [in] width
 		   [in] bandwidth
 		  
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��8��
 *   ���� vincent_pan
 *   ���� ����
 ----------------------------------------------------------------------------*/
void ShowBandwidth(uint32 width, uint64 bandwidth)
{
	uint32 flag = 0;
	while(true)
	{
		if (bandwidth < 1024)break;    // 1024 = 1k ����
		bandwidth = bandwidth / 1024;
		flag ++;
	}
	std::string bandwidth_to_str;
	
	if (flag == 0)
	{
		bandwidth_to_str = boost::lexical_cast<std::string>(bandwidth);
		bandwidth_to_str += "B/s";
		std::cout <<std::setw(width)<<bandwidth_to_str;
	}
			 
	if (flag == 1)
	{
		bandwidth_to_str = boost::lexical_cast<std::string>(bandwidth);
		bandwidth_to_str += "KB/s";
		std::cout <<std::setw(width)<<bandwidth_to_str;
	}
			
	if (flag == 2)
	{
		bandwidth_to_str = boost::lexical_cast<std::string>(bandwidth);
		bandwidth_to_str += "MB/s";
		std::cout <<std::setw(width)<<bandwidth_to_str;
	}
			
	if (flag >= 3)
	{
		bandwidth_to_str = boost::lexical_cast<std::string>(bandwidth);
		bandwidth_to_str += "GB/s";
		std::cout <<std::setw(width)<<bandwidth_to_str;
	}	
}

/*-----------------------------------------------------------------------------
 * ��  ��: ת��time����ʾ��ʽ
 * ��  ��: [in] time    
		   [in] width
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��8��
 *   ���� vincent_pan
 *   ���� ����
 ----------------------------------------------------------------------------*/
void ShowTime(uint32 width, uint32 time)
{
	uint32 day = time/(60*60*24);
	uint32 hour = (time%(60*60*24)) /(60 * 60);
	uint32 min  = ((time%(60*60*24)) %(60 * 60))/60;
	uint32 second = ((time%(60*60*24)) %(60 * 60))% 60;
	std::string time_str(boost::lexical_cast<std::string>(day));
	time_str.append("/");
	time_str.append(boost::lexical_cast<std::string>(hour));
	time_str.append("/");
	time_str.append(boost::lexical_cast<std::string>(min));
	time_str.append("/");
	time_str.append(boost::lexical_cast<std::string>(second));
	std::cout<<std::setw(width)<<time_str;
}

void GetLine(std::string & line)
{
	for(int i = 0;; ++i)
	{
		char c = std::getchar();
		if('\n' == c) break;
		if(8 == c && i)
		{
			line.erase(i,1);
			std::putchar('\b');
			std::putchar(' ');
			std::putchar('\b');
			--i;
			continue;
			
		}
		line.append(1,c);
	}
}

/*-----------------------------------------------------------------------------
 * ��  ��: ����ƥ��
 * ��  ��: [in] cmd
 		  
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��8��
 *   ���� vincent_pan
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool HelpCmdHandler::Match(const std::string& cmd)
{
	const char * command_flag = "\\s*(help)\\s*";
	boost::smatch match;
	if(boost::regex_match(cmd, match, boost::regex(command_flag)))
	{
		return true;
	}

	return false;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ����ִ��
 * ��  ��: [in] cmd
 		  
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��8��
 *   ���� vincent_pan
 *   ���� ����
 ----------------------------------------------------------------------------*/
void HelpCmdHandler::Excute(BcrmSocketClient & bsc)
{
	Help();
}

/*-----------------------------------------------------------------------------
 * ��  ��: help��ʾ����
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��24��
 *   ���� vincent_pan
 *   ���� ����
 ----------------------------------------------------------------------------*/
void HelpCmdHandler::Help()
{	
	std::cout<< "The bcrm client commands is as follow:" << std::endl;
	std::cout<< "help                                                           -- show all the commands" << std::endl;
	std::cout<< "show system [ -t refresh-interval ]                            -- show the server system infomation" <<std::endl;
	std::cout<< "show session  { all/bt/pps }  [ -t refresh-interval ]          -- show the session infomation" <<std::endl;
	std::cout<< "show session   bt/pps  torrent numid [ -t refresh-interval ]   -- show the torrent infomaton" <<std::endl;
	std::cout<< "show  io-cache stat  { bt/pps } [ -t refresh-interval ]        -- show the infomation of io cache" <<std::endl;
	std::cout<< "exit                                                           -- quit the program" <<std::endl;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ����ƥ��
 * ��  ��: [in] cmd
 		  
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��8��
 *   ���� vincent_pan
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool ExitCmdHandler::Match(const std::string& cmd)
{
	const char * command_flag = "\\s*(exit)\\s*";
	boost::smatch match;
	if(boost::regex_match(cmd, match, boost::regex(command_flag)))
	{
		return true;
	}

	return false;	
}

/*-----------------------------------------------------------------------------
 * ��  ��: ����ִ��
 * ��  ��: [in] cmd
 		  
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��8��
 *   ���� vincent_pan
 *   ���� ����
 ----------------------------------------------------------------------------*/
void ExitCmdHandler::Excute(BcrmSocketClient & bsc)
{	
	Exit(bsc);
}

/*-----------------------------------------------------------------------------
 * ��  ��: �˳�����
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��24��
 *   ���� vincent_pan
 *   ���� ����
 ----------------------------------------------------------------------------*/
void ExitCmdHandler::Exit(BcrmSocketClient & bsc)
{	
	bsc.socket().close();
	std::exit(0);        
}

/*-----------------------------------------------------------------------------
 * ��  ��: ����ƥ��
 * ��  ��: [in] cmd
 		  
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��8��
 *   ���� vincent_pan
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool ShowSessionCmdHandler::Match(const std::string& cmd)
{
	const char * command_flag[] = { "\\s*(show)\\s*(session)\\s*(all)\\s*",
		                            "\\s*(show)\\s*(session)\\s*(all)\\s*(-t)\\s*([0-9]+)\\s*"
		                          };
		                       
	boost::smatch match;
	if(boost::regex_match(cmd, match, boost::regex(command_flag[0])))
	{
		time_ = 0;
		return true;
	}
	if(boost::regex_match(cmd, match, boost::regex(command_flag[1])))
	{ 
		time_ = boost::lexical_cast<uint32>(match[5]);
		return true;
	}
	return false;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ����ִ��
 * ��  ��: [in] cmd
 		  
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��8��
 *   ���� vincent_pan
 *   ���� ����
 ----------------------------------------------------------------------------*/
void ShowSessionCmdHandler::Excute(BcrmSocketClient & bsc)
{
	bsc.set_command(GetMsg(bsc));
	if(time_) ShowRealAllSession(time_, bsc);
	else ShowAllSession(bsc);
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��ȡcmd��Ϣ
 * ��  ��: [in] cmd
 		  
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��8��
 *   ���� vincent_pan
 *   ���� ����
 ----------------------------------------------------------------------------*/
RequestSessionMsg ShowSessionCmdHandler::GetMsg(BcrmSocketClient & bsc)
{
	return RequestSessionMsg();
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��ʱ����session��Ϣ�ӿ�
 * ��  ��: [in] time ˢ��Ƶ��
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��24��
 *   ���� vincent_pan
 *   ���� ����
 ----------------------------------------------------------------------------*/
void ShowSessionCmdHandler::ShowRealAllSession(uint32 time, BcrmSocketClient & bsc)
{	
	signal_ready = false;
	std::signal(SIGINT, signal_handler);
	while (true)
	{	
		if (signal_ready) break;
		ShowAllSession(bsc);
		sleep(time);               
	}
}

/*-----------------------------------------------------------------------------
 * ��  ��: ����session��Ϣ��ʾ����
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��24��
 *   ���� vincent_pan
 *   ���� ����
 ----------------------------------------------------------------------------*/
void ShowSessionCmdHandler::ShowAllSession(BcrmSocketClient & bsc)
{	
	bsc.DoSend();         //����command
	bsc.DoReceive();      //����info

	auto pbmessagesp = DecodeProtobufMessage(&(bsc.info()[0]), bsc.info().size());
	auto siq = dynamic_cast<BcrmSessionInfo*>(pbmessagesp.get());
	
	if(!signal_ready)
	{
		std::cout<< "\033"<< "\033[2J";                       //����
		std::cout<< "\033"<< "\033[0;0H";                     //��궨λ
	}
	std::cout<< "-------------------------------------------------------------------------------" <<std::endl;
	std::cout<< " Session | Torrent Number | Peer Number | Upload BandWidth | Download BandWidth" <<std::endl;
	std::cout<< "-------------------------------------------------------------------------------" <<std::endl;

	uint32 size = siq->session_info_size();
	for(uint32 i = 0; i < size; ++i)
	{
		BcrmSessionInfo::SessionInfo si = siq->mutable_session_info()->Get(i);
		std::cout<< std::setiosflags(std::ios::left)
			     <<" "<<std::setw(10)<< si.session_type()
			     << std::setw(17)<< si.torrent_num()  
				 << std::setw(14)<< si.peer_num();
		ShowBandwidth(19,si.upload_bandwidth());
		ShowBandwidth(19,si.download_bandwidth());
		std::cout << std::endl;
	}
	std::cout<< "-------------------------------------------------------------------------------" <<std::endl;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ����ƥ��
 * ��  ��: [in] cmd
 		  
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��8��
 *   ���� vincent_pan
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool ShowTorrentCmdHandler::Match(const std::string& cmd) 
{
	const char * command_flag[] = { "\\s*(show)\\s*(session)\\s*(bt|pps\\s*)",
		                            "\\s*(show)\\s*(session)\\s*(bt|pps)\\s*(-t)\\s*([0-9]+)\\s*"
		                          };	                       
	boost::smatch match;
	if (boost::regex_match(cmd, match, boost::regex(command_flag[0])))
	{
		time_ = 0;
		session_type_ = match[3];
		return true;
	}
	if (boost::regex_match(cmd, match, boost::regex(command_flag[1])))
	{
		session_type_ = match[3];
		time_ = boost::lexical_cast<uint32>(match[5]);
		return true;
	}
	return false;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ����ִ��
 * ��  ��: [in] cmd
 		  
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��8��
 *   ���� vincent_pan
 *   ���� ����
 ----------------------------------------------------------------------------*/
void ShowTorrentCmdHandler::Excute(BcrmSocketClient & bsc)
{
	bsc.set_command(GetMsg(bsc));
	if(time_) ShowRealTorrentPreSession(time_, bsc);
	else ShowTorrentPreSession(bsc);	
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��ȡ������Ϣ
 * ��  ��: [in] cmd
 		  
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��8��
 *   ���� vincent_pan
 *   ���� ����
 ----------------------------------------------------------------------------*/
RequestTorrentMsg ShowTorrentCmdHandler::GetMsg(BcrmSocketClient & bsc)
{
	RequestTorrentMsg torrent_msg;
	torrent_msg.set_session_type(session_type_);
	return torrent_msg;
}

/*-----------------------------------------------------------------------------
 * ��  ��: session������torrent��ʾ����
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��24��
 *   ���� vincent_pan
 *   ���� ����
 ----------------------------------------------------------------------------*/	
void ShowTorrentCmdHandler::ShowTorrentPreSession(BcrmSocketClient & bsc)
{
	bsc.DoSend();         //����command
	bsc.DoReceive();      //����info
	auto pbmessagesp = DecodeProtobufMessage(&(bsc.info()[0]), bsc.info().size());
	auto tiq = dynamic_cast<BcrmTorrentInfo*>(pbmessagesp.get());
	
	if(!signal_ready)
	{
		std::cout<< "\033"<< "\033[2J";                       //����
		std::cout<< "\033"<< "\033[0;0H";                     //��궨λ
	}
	std::cout<< "---------------------------------------------------------------------------------------------" <<std::endl;
	std::cout<< " Torrent | Complete | Alive Time | Inner | Outer | Upload    | Download  | Total  | Total    " <<std::endl;
	std::cout<< " Index   | Percent  | (d/h/m/s)  | Peer  | Peer  | BandWidth | BandWidth | Upload | Download " <<std::endl;
	std::cout<< "---------------------------------------------------------------------------------------------" <<std::endl;

	                  
	uint32 size = tiq->torrent_info_size();
	uint32 number = 1;
	bsc.info_hash_map().clear();      //���map
	for(uint32 i = 0; i < size; ++i, ++number)
	{	
	
		BcrmTorrentInfo::TorrentInfo ti = tiq->torrent_info().Get(i);
		

		std::cout << std::setiosflags(std::ios::left)
			      <<" "<< std::setw(10)<< number  
				  << std::resetiosflags(std::ios::left);
				  
		std::cout << std::setiosflags(std::ios::right)
			      << std::setw(3)<<ti.complete_precent();

		std::cout << std::resetiosflags(std::ios::right)
			      << std::setiosflags(std::ios::left)
			      << std::setw(8)<< "%";
	    ShowTime(13, ti.alive_time());
		std::cout << std::setw(8)<< ti.inner_peer_num()
			      << std::setw(8)<< ti.outer_peer_num();

		ShowBandwidth(12,ti.upload_bandwidth());
		ShowBandwidth(12,ti.download_bandwidth());
		ShowSize(9,ti.total_upload());
		ShowSize(9,ti.total_download());
		std::cout<<std::endl;
		bsc.info_hash_map().insert(std::make_pair(number, ti.info_hash()));
	}
	std::cout<< "---------------------------------------------------------------------------------------------" <<std::endl;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��ʱ����torrent��Ϣ�ӿ�
 * ��  ��: [in] time ˢ��Ƶ��
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��24��
 *   ���� vincent_pan
 *   ���� ����
 ----------------------------------------------------------------------------*/
void ShowTorrentCmdHandler::ShowRealTorrentPreSession(uint32 time, BcrmSocketClient & bsc)
{	
	signal_ready = false;
	std::signal(SIGINT, signal_handler);
	while (true)
	{	
		if (signal_ready) break;
		ShowTorrentPreSession(bsc);
		sleep(time);               
	}
}

/*-----------------------------------------------------------------------------
 * ��  ��: ����ƥ��
 * ��  ��: [in] cmd
 		  
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��8��
 *   ���� vincent_pan
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool ShowPeerCmdHandler::Match(const std::string& cmd)
{
	const char * command_flag[] = { "\\s*(show)\\s*(session)\\s*(bt|pps)\\s*(torrent)\\s*([0-9]+)\\s*",
		                            "\\s*(show)\\s*(session)\\s*(bt|pps)\\s*(torrent)\\s*([0-9]+)\\s*(-t)\\s*([0-9]+)\\s*"
		                          };	                       
	boost::smatch match;
	if (boost::regex_match(cmd, match, boost::regex(command_flag[0])))
	{
		time_ = 0;
		session_type_ = match[3];
		torrent_id_ = boost::lexical_cast<uint32> (match[5]);
		return true;
	}
	if (boost::regex_match(cmd, match, boost::regex(command_flag[1])))
	{
		session_type_ = match[3];
		torrent_id_ = boost::lexical_cast<uint32>(match[5]);
		time_ = boost::lexical_cast<uint32>(match[7]);
		return true;
	}
	return false;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ����ִ��
 * ��  ��: [in] cmd
 		  
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��8��
 *   ���� vincent_pan
 *   ���� ����
 ----------------------------------------------------------------------------*/
void ShowPeerCmdHandler::Excute(BcrmSocketClient & bsc)
{
	bsc.set_command(GetMsg(bsc));
	if(time_) ShowRealPeerPreTorrent(time_,bsc);
	else ShowPeerPreTorrent(bsc);
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��ȡ��Ϣ
 * ��  ��: [in] cmd
 		  
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��8��
 *   ���� vincent_pan
 *   ���� ����
 ----------------------------------------------------------------------------*/
RequestPeerMsg ShowPeerCmdHandler::GetMsg(BcrmSocketClient & bsc)
{
	RequestPeerMsg peer_msg;
	peer_msg.set_session_type(session_type_);
	if (!bsc.info_hash_map().empty() && ( torrent_id_ < (bsc.info_hash_map().size()+1)))
	{
		peer_msg.set_info_hash(bsc.info_hash_map().find(torrent_id_)->second);
	}
	else 
	{
		peer_msg.set_info_hash("");    //���û���ҵ�info_hash ���""
	}
	return peer_msg;
}

/*-----------------------------------------------------------------------------
 * ��  ��: torrent������peer��Ϣ��ʾ����
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��24��
 *   ���� vincent_pan
 *   ���� ����
 ----------------------------------------------------------------------------*/
void ShowPeerCmdHandler::ShowPeerPreTorrent(BcrmSocketClient & bsc)
{	
	bsc.DoSend();         //����command
	bsc.DoReceive();      //����info
	auto pbmessagesp = DecodeProtobufMessage(&(bsc.info()[0]), bsc.info().size());
	auto piq = dynamic_cast<BcrmPeerInfo*>(pbmessagesp.get());
	
	if(!signal_ready)
	{
		std::cout<< "\033"<< "\033[2J";                       //����
		std::cout<< "\033"<< "\033[0;0H";                     //��궨λ
	}	
	std::cout<< "-----------------------------------------------------------------------------------------------------" <<std::endl;
	std::cout<< " Peer IP       | Peer   | Inner  | Complete | Alive Time | Upload    | Download  | Total  | Total    " <<std::endl;
	std::cout<< "               | Client | /Outer | Percent  | (d/h/m/s)  | BandWidth | BandWidth | Upload | Download " <<std::endl;
	std::cout<< "-----------------------------------------------------------------------------------------------------" <<std::endl;

	uint32 size = piq->peer_info_size();
	for(uint32 i = 0; i < size; ++i)
	{	
		BcrmPeerInfo::PeerInfo pi = piq->peer_info().Get(i);

		std::cout << std::setiosflags(std::ios::left)
			      << " " <<std::setw(16)<< boost::asio::ip::address_v4(pi.peer_ip()).to_string();
		if(pi.client_type() < 6)
			std::cout<< std::setw(9)<<std::string(kPeerClientType[pi.client_type()]);
		 else
		 	std::cout<< std::setw(9)<<"ERROR";
		std::cout << std::setw(9)<< pi.peer_type(); 
		std::cout << std::resetiosflags(std::ios::left)
		          << std::setw(3)<< pi.complete_precent();
		std::cout << std::setiosflags(std::ios::left)
		          << std::setw(8)<< "%";
		ShowTime(13, pi.alive_time());
		ShowBandwidth(12, pi.upload_bandwidth());
		ShowBandwidth(12, pi.download_bandwidth());
		ShowSize(9, pi.upload());
		ShowSize(9, pi.download());
		std::cout<<std::endl;
	}
	std::cout<< "-----------------------------------------------------------------------------------------------------" <<std::endl;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��ʱ����peer��Ϣ�ӿ�
 * ��  ��: [in] time ˢ��Ƶ��
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��24��
 *   ���� vincent_pan
 *   ���� ����
 ----------------------------------------------------------------------------*/
void ShowPeerCmdHandler::ShowRealPeerPreTorrent(uint32 time, BcrmSocketClient & bsc)
{	
	signal_ready = false;
	std::signal(SIGINT, signal_handler);
	while (true)
	{	
		if (signal_ready) break;
		ShowPeerPreTorrent(bsc);
		sleep(time);               
	}
}

/*-----------------------------------------------------------------------------
 * ��  ��: ����ƥ��
 * ��  ��: [in] cmd
 		  
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��8��
 *   ���� vincent_pan
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool ShowCacheCmdHandler::Match(const std::string& cmd)
{
	const char * command_flag[] = { "\\s*(show)\\s*(io-cache)\\s*(stat)\\s*(bt|pps)\\s*",
		                            "\\s*(show)\\s*(io-cache)\\s*(stat)\\s*(bt|pps)\\s*(-t)\\s*([0-9]+)\\s*"
		                          };
		                       
	boost::smatch match;
	if (boost::regex_match(cmd, match, boost::regex(command_flag[0])))
	{
		time_ = 0;
		session_type_ = match[4];
		return true;
	}
	if (boost::regex_match(cmd, match, boost::regex(command_flag[1])))
	{	
		session_type_ = match[4];
		time_ = boost::lexical_cast<uint32>(match[6]);
		return true;
	}
	return false;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ����ִ��
 * ��  ��: [in] cmd
 		  
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��8��
 *   ���� vincent_pan
 *   ���� ����
 ----------------------------------------------------------------------------*/
void ShowCacheCmdHandler::Excute(BcrmSocketClient & bsc)
{	
	bsc.set_command(GetMsg(bsc));
	if(time_) ShowRealCacheStatus(time_, bsc);
	else ShowCacheStatus(bsc);
} 

/*-----------------------------------------------------------------------------
 * ��  ��: ��ȡ������Ϣ
 * ��  ��: [in] cmd
 		  
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��8��
 *   ���� vincent_pan
 *   ���� ����
 ----------------------------------------------------------------------------*/
RequestCacheStatMsg ShowCacheCmdHandler::GetMsg(BcrmSocketClient & bsc)
{
	RequestCacheStatMsg cache_stat_msg;
	cache_stat_msg.set_session_type(session_type_);
	return cache_stat_msg;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��ʱ����Cache��Ϣ�ӿ�
 * ��  ��: [in] time ˢ��Ƶ��
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��24��
 *   ���� vincent_pan
 *   ���� ����
 ----------------------------------------------------------------------------*/
void ShowCacheCmdHandler::ShowRealCacheStatus(uint32 time, BcrmSocketClient & bsc)
{	
	signal_ready = false;
	std::signal(SIGINT, signal_handler);
	while (true)
	{	
		if (signal_ready) break;
		ShowCacheStatus(bsc);
		sleep(time);            
	}
}

/*-----------------------------------------------------------------------------
 * ��  ��: ioCache ��Ϣ��ʾ����
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��24��
 *   ���� vincent_pan
 *   ���� ����
 ----------------------------------------------------------------------------*/
void ShowCacheCmdHandler::ShowCacheStatus(BcrmSocketClient & bsc)
{
	bsc.DoSend();      //����command
	bsc.DoReceive();   //����info

	auto pbmessagesp = DecodeProtobufMessage(&(bsc.info()[0]), bsc.info().size());
	auto cache = dynamic_cast<BcrmCacheStatusInfo*>(pbmessagesp.get());
		 
	BcrmCacheStatusInfo::CacheStatusInfo ci = cache->cache_status_info();
	if(!signal_ready)
	{
		std::cout<< "\033"<< "\033[2J";                       //����
		std::cout<< "\033"<< "\033[0;0H";                     //��궨λ
	}
	std::cout << "-------------------------------------------------"<<std::endl;
	uint32 read_hit_radio = 0;
	uint32 write_hit_radio = 0;
	if(ci.blocks_read())
	{
		 read_hit_radio = ci.blocks_read_hit()* 100 / ci.blocks_read();
	}

	if(ci.blocks_write())
	{
		 write_hit_radio =  (ci.blocks_write() - ci.write_to_disk_directly() - ci.writes())*100 / ci.blocks_write();
	}
	
	std::cout << std::setiosflags(std::ios::left)<< std::setw(33)<< "read operation count"<<":"<<ci.reads()<<std::endl;
	std::cout << std::setiosflags(std::ios::left)<< std::setw(33)<< "write operation count"<<":"<<ci.writes()<<std::endl;
	std::cout << std::setiosflags(std::ios::left)<< std::setw(33)<< "read block count"<<":"<<ci.blocks_read()<<std::endl; 
	std::cout << std::setiosflags(std::ios::left)<< std::setw(33)<< "write block count"<<":"<<ci.blocks_write()<<std::endl;
	std::cout << std::setiosflags(std::ios::left)<< std::setw(33)<< "read block hit"<<":"<<ci.blocks_read_hit()<<std::endl;
	std::cout << std::setiosflags(std::ios::left)<< std::setw(33)<< "write to disk directly"<<":"<<ci.write_to_disk_directly()<<std::endl;
	std::cout << std::setiosflags(std::ios::left)<< std::setw(33)<< "read cache size"<<":"<<ci.read_cache_size()<<std::endl;
	std::cout << std::setiosflags(std::ios::left)<< std::setw(33)<< "finished write cache size"<<":"<<ci.finished_write_cache_size()<<std::endl;
	std::cout << std::setiosflags(std::ios::left)<< std::setw(33)<< "unfinished write cache size"<<":"<<ci.unfinished_write_cache_size()<<std::endl;
	std::cout << std::setiosflags(std::ios::left)<< std::setw(33)<< "read cache hit ratio" <<":"<<read_hit_radio<<"%"<<std::endl;
	std::cout << std::setiosflags(std::ios::left)<< std::setw(33)<< "write cache hit ratio" <<":"<<write_hit_radio<<"%"<<std::endl;
	
	std::cout << "-------------------------------------------------" <<std::endl;
		
}

/*-----------------------------------------------------------------------------
 * ��  ��: ����ƥ��
 * ��  ��: [in] cmd
 		  
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��8��
 *   ���� vincent_pan
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool ShowSystemCmdHandler::Match(const std::string& cmd)
{
	const char * command_flag[] = { "\\s*(show)\\s*(system)\\s*",
		                            "\\s*(show)\\s*(system)\\s*(-t)\\s*([0-9]+)\\s*"
		                          };
		                       
	boost::smatch match;
	if (boost::regex_match(cmd, match, boost::regex(command_flag[0])))
	{
		time_ = 0;
		return true;
	}
	if (boost::regex_match(cmd, match, boost::regex(command_flag[1])))
	{
		time_ =boost::lexical_cast<uint32>(match[4]);
		return true;
	}
	return false;

}

/*-----------------------------------------------------------------------------
 * ��  ��: ����ִ��
 * ��  ��: [in] cmd
 		  
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��8��
 *   ���� vincent_pan
 *   ���� ����
 ----------------------------------------------------------------------------*/
void ShowSystemCmdHandler::Excute(BcrmSocketClient & bsc)
{
	bsc.set_command(GetMsg(bsc));
	if(time_) ShowRealSystem(time_, bsc);
	else ShowSystem(bsc);
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��ȡ������Ϣ
 * ��  ��: [in] cmd
 		  
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��8��
 *   ���� vincent_pan
 *   ���� ����
 ----------------------------------------------------------------------------*/
RequestSystemMsg ShowSystemCmdHandler::GetMsg(BcrmSocketClient & bsc)
{		
	return RequestSystemMsg();
}

/*-----------------------------------------------------------------------------
 * ��  ��: ϵͳ��Ϣ��ʾ����
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��24��
 *   ���� vincent_pan
 *   ���� ����
 ----------------------------------------------------------------------------*/
void ShowSystemCmdHandler::ShowSystem(BcrmSocketClient & bsc)
{	
	bsc.DoSend();         //����command
	bsc.DoReceive();      //����info
	auto pbmessagesp = DecodeProtobufMessage(&(bsc.info()[0]), bsc.info().size());
	auto sys = dynamic_cast<BcrmSystemInfo*>(pbmessagesp.get());

	::time_t t;             // ����ctime
	struct ::tm * p = NULL;
	::time(&t);
	p = ::localtime(&t);

	if(!signal_ready)
	{
		std::cout<< "\033"<< "\033[2J";                       //����
		std::cout<< "\033"<< "\033[0;0H";                     //��궨λ
	}
	std::cout<< "system run time:"
		     << 1900 + p->tm_year<< "/"
		     << 1 + p->tm_mon<< "/"
		     << p->tm_mday << " "
		     << p->tm_hour << ":"
		     << p->tm_min << ":"
		     << p->tm_sec << std::endl;

	std::cout<< "----------------------------------------------------------"<<std::endl;
	ShowCpuInfo(*sys);
	ShowMemInfo(*sys);
	ShowDiskInfo(*sys);
	ShowNetInfo(*sys);
	std::cout<< "----------------------------------------------------------"<<std::endl;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��ʱ����ϵͳ��Ϣ�ӿ�
 * ��  ��: [in] time ˢ��Ƶ��
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��10��24��
 *   ���� vincent_pan
 *   ���� ����
 ----------------------------------------------------------------------------*/
void ShowSystemCmdHandler::ShowRealSystem(uint32 time, BcrmSocketClient & bsc)
{	
	signal_ready = false;
	std::signal(SIGINT, signal_handler);
	while (true)
	{	
		if (signal_ready) break;
		ShowSystem(bsc);
		sleep(time);               
	}
}

/*-----------------------------------------------------------------------------
 * ��  ��: ϵͳ��Ϣ��ʾ����
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��8��
 *   ���� vincent_pan
 *   ���� ����
 ----------------------------------------------------------------------------*/
void ShowSystemCmdHandler::ShowCpuInfo(BcrmSystemInfo& sys)
{
	std::cout<< "CPU     | ";
	uint32 cpu_size = sys.system_info().cpu_info_size();

	BcrmSystemInfo::SystemInfo::CpuInfo cio = sys.system_info().cpu_info().Get(0);
	std::cout<<std::setiosflags(std::ios::left)<<std::setw(5)<<cio.cpu_name()<< ":"
	         <<std::setw(3)<<cio.cpu_usage()<< "%" <<std::endl;
 
	std::cout<<std::setiosflags(std::ios::right)<<std::setw(10)<< "| "<<std::resetiosflags(std::ios::right);
	for(uint32 i = 1; i < (cpu_size+1)/2; ++i)
	{
		BcrmSystemInfo::SystemInfo::CpuInfo ci = sys.system_info().cpu_info().Get(i);
		std::cout<<std::setiosflags(std::ios::left)<<std::setw(5)<<ci.cpu_name()<< ":"
		         <<std::setw(3)<<ci.cpu_usage()<<std::setw(4)<< "%";
	}
	std::cout<<std::endl;
	std::cout<<std::setiosflags(std::ios::right)<<std::setw(10)<< "| "<<std::resetiosflags(std::ios::right);
	for(uint32 i = (cpu_size+1)/2; i < cpu_size; ++i)
	{
		BcrmSystemInfo::SystemInfo::CpuInfo ci = sys.system_info().cpu_info().Get(i);
		std::cout<<std::setiosflags(std::ios::left)<<std::setw(5)<<ci.cpu_name()<< ":"
	             <<std::setw(3)<<ci.cpu_usage()<<std::setw(4)<< "%";
	}
	std::cout <<std::endl;
	std::cout<< "----------------------------------------------------------"<<std::endl;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ϵͳ��Ϣ��ʾ����
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��8��
 *   ���� vincent_pan
 *   ���� ����
 ----------------------------------------------------------------------------*/
void ShowSystemCmdHandler::ShowDiskInfo(BcrmSystemInfo& sys)
{
	std::cout<< "DISK    |"<<std::endl;
	uint32 disk_size = sys.system_info().disk_info_size();
	for(uint32 i = 0; i < disk_size; ++i)
	{
		BcrmSystemInfo::SystemInfo::DiskInfo di = sys.system_info().disk_info().Get(i);
		std::cout << std::setiosflags(std::ios::right) << std::setw(9)<<"|";
		std::cout << std::resetiosflags(std::ios::right)
			      << std::setw(6)<< di.disk_name() 
			      << " - total "
			      << std::setw(4)<<di.total_size() 
			      << " GB, usage: " 
			      << di.usage()<<"%"<<std::endl;
	}
	std::cout<< "----------------------------------------------------------"<<std::endl;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ϵͳ��Ϣ��ʾ����
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��8��
 *   ���� vincent_pan
 *   ���� ����
 ----------------------------------------------------------------------------*/
void ShowSystemCmdHandler::ShowMemInfo(BcrmSystemInfo& sys)
{
	std::cout<<std::setiosflags(std::ios::left)
		     << "MEM     | total: "
		     << std::setw(4)<<sys.system_info().mem_total_size()
		     << " GB, usage: "
		     << sys.system_info().mem_usage()<<"%"<<std::endl;
	std::cout<< "----------------------------------------------------------"<<std::endl;
}

/*-----------------------------------------------------------------------------
 * ��  ��: ϵͳ��Ϣ��ʾ����
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013��11��8��
 *   ���� vincent_pan
 *   ���� ����
 ----------------------------------------------------------------------------*/
void ShowSystemCmdHandler::ShowNetInfo(BcrmSystemInfo& sys)
{
	std::cout<< "NET     |"<<std::endl;
	uint32 net_size = sys.system_info().net_info_size();

	for(uint32 j = 0; j < net_size; ++j)
	{
		BcrmSystemInfo::SystemInfo::NetInfo ni = sys.system_info().net_info().Get(j);
		std::cout <<std::setiosflags(std::ios::right)<<std::setw(9)<< "|" 
			      <<std::resetiosflags(std::ios::right)<<std::setw(6)<<ni.net_name();
		uint64 in_size = ni.in_size();
		uint32 flag = 0;
		while(true)
		{
			if (in_size < 1024)break;      // 1024 = 1k ����
			in_size = in_size / 1024;
			flag ++;
		}
		
		if (flag == 0)std::cout <<" - in: "<<std::setw(3)<<in_size <<" b, out: ";
		if (flag == 1)std::cout <<" - in: "<<std::setw(3)<<in_size <<" Kb, out: ";
		if (flag == 2)std::cout <<" - in: "<<std::setw(3)<<in_size <<" Mb, out: ";
		if (flag >= 3)std::cout <<" - in: "<<std::setw(3)<<in_size <<" Gb, out: ";
		
		uint64 out_size = ni.out_size();
		flag = 0;
		while(true)
		{
			if (out_size < 1024)break;    // 1024 = 1k ����
			out_size = out_size / 1024;
			flag ++;
		}
		if (flag == 0)std::cout << std::setw(3)<< out_size<<" b"<< std::endl;		 
		if (flag == 1)std::cout << std::setw(3)<< out_size<<" Kb"<< std::endl;		
		if (flag == 2)std::cout << std::setw(3)<< out_size<<" Mb"<< std::endl;		
		if (flag >= 3)std::cout << std::setw(3)<< out_size<<" Gb"<< std::endl;
	}	
}

}  //namespace BroadCache

/*-----------------------------------------------------------------------------
 * ��  ��: signal �Ĵ����� Ϊȫ��
 * ��  ��: sig   ϵͳ���͵�signalֵ 
 * ����ֵ: 
 * ��  ��:
 *   ʱ�� 2013��10��24��
 *   ���� vincent_pan
 *   ���� ����
 ----------------------------------------------------------------------------*/
void signal_handler(int sig)    
{
	signal_ready = true;
}



