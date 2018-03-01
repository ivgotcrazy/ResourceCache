/*#############################################################################
 * �ļ���   : peer.hpp
 * ������   : teck_zhou	
 * ����ʱ�� : 2013��09��03��
 * �ļ����� : Peer�������
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#ifndef HEADER_PEER
#define HEADER_PEER

#include "bc_typedef.hpp"
#include "big_number.hpp"
#include "endpoint.hpp"

namespace BroadCache
{

enum PeerConnType
{
	PEER_CONN_COMMON,
	PEER_CONN_SPONSOR,
	PEER_CONN_PROXY
};

enum PeerType
{
	PEER_UNKNOWN,
	PEER_INNER,
	PEER_OUTER,
	PEER_CACHED_RCS,
	PEER_INNER_PROXY,
	PEER_OUTER_PROXY
};

enum PeerRetriveType
{
	PEER_RETRIVE_UNKNOWN,
	PEER_RETRIVE_INCOMMING,
	PEER_RETRIVE_TRACKER,
	PEER_RETRIVE_DHT,
	PEER_RETRIVE_UGS
};

enum PeerClientType
{
	PEER_CLIENT_UNKNOWN,
	PEER_CLIENT_BITCOMET,
	PEER_CLIENT_XUNLEI,
	PEER_CLIENT_BITTORRENT,
	PEER_CLIENT_UTORRENT,
	PEER_CLIENT_BITSPRIT
};

}

#endif
