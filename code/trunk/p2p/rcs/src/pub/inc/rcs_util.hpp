/*#############################################################################
 * �ļ���   : rcs_util.hpp
 * ������   : teck_zhou	
 * ����ʱ�� : 2013��11��13��
 * �ļ����� : RCS����ʹ�ú���
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#ifndef HEADER_RCS_UTIL
#define HEADER_RCS_UTIL

#include "bc_typedef.hpp"
#include "rcs_typedef.hpp"
#include "peer_request.hpp"
#include "bc_util.hpp"

namespace BroadCache
{

// ��ReadDataJobת��ΪPeerRequest
PeerRequest ToPeerRequest(const ReadDataJob& read_job);
PeerRequest ToPeerRequest(const WriteDataJob& write_job);

// ��ȡ������
#define GET_RCS_CONFIG_INT(item, para) GET_CONFIG_INT(RcsConfigParser, item, para)
#define GET_RCS_CONFIG_STR(item, para) GET_CONFIG_STR(RcsConfigParser, item, para)
#define GET_RCS_CONFIG_BOOL(item, para) GET_CONFIG_BOOL(RcsConfigParser, item, para)

// shared pointerǿת
#define SP_CAST(Type, shared_ptr) boost::dynamic_pointer_cast<Type>(shared_ptr)

}

#endif
