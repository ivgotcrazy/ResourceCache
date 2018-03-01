/**************************************************************************
 *�ļ���  : connection.hpp
 *������  : teck_zhou
 *����ʱ��: 2013.10.08
 *�ļ�����: SocketConnection���������
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
 *************************************************************************/

#ifndef HEADER_CONNECTION
#define HEADER_CONNECTION

#include "endpoint.hpp"
#include "bc_typedef.hpp"
#include "hash_stream.hpp"
#include <boost/system/error_code.hpp>

namespace BroadCache
{

/******************************************************************************
 * ����: ���Ӵ����룬�ϲ�Э����Ҫ��ע�Ĵ�����
 * ���ߣ�teck_zhou
 * ʱ�䣺2013/10/09
 *****************************************************************************/
enum ConnectionError
{
	CONN_ERR_SUCCESS = 0,	// �ɹ�
	CONN_ERR_REMOTE_CLOSED,	// Զ�˹ر�
	CONN_ERR_OTHER			// ��������
};

inline ConnectionError ToConnErr(const error_code& ec)
{
	switch (ec.value())
	{
	case boost::system::errc::success:
		return CONN_ERR_SUCCESS;
	
	default:
		return CONN_ERR_OTHER;
	}
}

/******************************************************************************
 * ����: �������Ӻͱ�������
 * ���ߣ�teck_zhou
 * ʱ�䣺2013/10/09
 *****************************************************************************/
enum ConnectionType
{
	CONN_INVALID, // ��Ч����
	CONN_ACTIVE,  // ��������
	CONN_PASSIVE  // ��������
};

/******************************************************************************
 * ����: ���ӵ�Э������
 * ���ߣ�teck_zhou
 * ʱ�䣺2013/10/09
 *****************************************************************************/
enum ConnectionProtocol
{
	CONN_PROTOCOL_UNKNOWN, // δ֪Э������
	CONN_PROTOCOL_TCP,     // TCPЭ������
	CONN_PROTOCOL_UDP	   // UDPЭ������
};

/******************************************************************************
 * ����: Ψһ��ʶһ�����ӵ���Ԫ��
 * ���ߣ�teck_zhou
 * ʱ�䣺2013/10/09
 *****************************************************************************/
struct ConnectionID
{
	ConnectionID() : protocol(CONN_PROTOCOL_UNKNOWN) {}
	ConnectionID(const EndPoint& local_addr, 
		         const EndPoint& remote_addr, 
				 ConnectionProtocol prot)
		         : local(local_addr)
				 , remote(remote_addr)
				 , protocol(prot) {}

	bool operator==(const ConnectionID& conn_id) const
	{
		return ((protocol == conn_id.protocol) 
			    && (local == conn_id.local)
			    && (remote == conn_id.remote));
	}

	EndPoint local;
	EndPoint remote;
	ConnectionProtocol protocol;
};

template<class Stream>
inline Stream& operator<<(Stream& stream, const ConnectionID& conn_id)
{
	stream << "[" << conn_id.local << "<->" << conn_id.remote << "]";
	return stream;
}

/*------------------------------------------------------------------------------
 * ��  ��: ����hashֵ������unorderd_map��hash����
 * ��  ��: [in] conn_id ConnectionID
 * ����ֵ: hashֵ
 * ��  ��:
 *   ʱ�� 2013��10��9��
 *   ���� teck_zhou
 *   ���� ����
 -----------------------------------------------------------------------------*/
inline std::size_t hash_value(const ConnectionID& conn_id)
{
	HashStream hash_stream;
    hash_stream & conn_id.local & conn_id.remote & conn_id.protocol;

    return hash_stream.value();
}

/******************************************************************************
 * ����: socket����ʹ�õı��ķ��ͻ�����
 * ���ߣ�teck_zhou
 * ʱ�䣺2013/10/09
 *****************************************************************************/
struct SendBuffer
{
	SendBuffer() : buf(0), size(0), len(0), cursor(0) {}

	char* buf;     // ��������
	uint64 size;   // �ڴ��С
	uint64 len;    // ���ͳ���
	uint64 cursor; // �����α꣬��η���ʹ��

	// ���ķ��ͺ�ص�����
	typedef boost::function<void(ConnectionError)> SendBufferHandler;
	SendBufferHandler send_buffer_handler;

	// ���ķ��ͺ��ڴ��ͷ�
	typedef boost::function<void(const char*)> BufferFreeFunc;
	BufferFreeFunc free_func;
};

/******************************************************************************
 * ����: UDP������ʹ�õķ��ͱ��Ļ�����
 *       ���ڷ�����Ҫ�����ͻ��˷��ͱ��ģ������Ҫ�������ķ��͵Ŀͻ��˵�ַ��
 *       ���⣬���ķ�����ɺ���Ҫ֪ͨsocket���ӣ���˻���Ҫ����ָ�����ӵ�ָ��
 * ���ߣ�teck_zhou
 * ʱ�䣺2013/10/09
 *****************************************************************************/
struct SendToBuffer
{
	SendToBuffer(const UdpSocketConnectionSP& conn, const SendBuffer& buf, const EndPoint& ep) 
		: connection(conn), send_buffer(buf), remote(ep) {}

	UdpSocketConnectionSP connection; // ����UDP����
	SendBuffer send_buffer;		      // ��������
	EndPoint   remote;                // ����Ŀ�ĵ�ַ
};

}

#endif
