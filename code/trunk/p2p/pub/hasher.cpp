/*#############################################################################
 * �ļ���   : hasher.cpp
 * ������   : teck_zhou	
 * ����ʱ�� : 2013��8��28��
 * �ļ����� : Hasher����
 * ##########################################################################*/

#include "hasher.hpp"

namespace BroadCache
{


Sha1Hasher::Sha1Hasher()
{
	SHA1_Init(&context_);
}

Sha1Hasher::Sha1Hasher(const char* data, int len)
{
	SHA1_Init(&context_);
	BC_ASSERT(data != 0);
	BC_ASSERT(len > 0);
	SHA1_Update(&context_, reinterpret_cast<unsigned char const*>(data), len);
}

void Sha1Hasher::Update(const char* data, int len)
{
	BC_ASSERT(data != 0);
	BC_ASSERT(len > 0);
	SHA1_Update(&context_, reinterpret_cast<unsigned char const*>(data), len);
}

void Sha1Hasher::Reset()
{
	SHA1_Init(&context_);
}

BigNumber Sha1Hasher::Final()
{
	BigNumber digest(HASH_NUMBER_SIZE);
	SHA1_Final(digest.begin(), &context_);
	return digest;
}

}