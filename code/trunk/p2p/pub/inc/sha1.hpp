/*#############################################################################
 * �ļ���   : sha1.hpp
 * ������   : teck_zhou	
 * ����ʱ�� : 2013��8��22��
 * �ļ����� : ��libtorrent��ֲ��������ͷ�ļ�
 * ##########################################################################*/

#ifndef HEADER_SHA1
#define HEADER_SHA1

#include <boost/cstdint.hpp>

namespace BroadCache
{

typedef boost::uint32_t u32;
typedef boost::uint8_t u8;

struct SHA_CTX
{
	u32 state[5];
	u32 count[2];
	u8 buffer[64];
};

void SHA1_Init(SHA_CTX* context);
void SHA1_Update(SHA_CTX* context, u8 const* data, u32 len);
void SHA1_Final(u8* digest, SHA_CTX* context);

}

#endif