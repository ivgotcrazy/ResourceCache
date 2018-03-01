/*##############################################################################
 * �ļ���   : hash_stream.hpp
 * ������   : rosan 
 * ����ʱ�� : 2013.10.20
 * �ļ����� : ������HashStream��,��������&������ 
 * ��Ȩ���� : Copyright(c)2013 BroadInter.All rights reserved.
 * ###########################################################################*/
#ifndef HEADER_HASH_STREAM
#define HEADER_HASH_STREAM

#include <boost/functional/hash.hpp>

#include "single_value_model.hpp"

namespace BroadCache
{

static const size_t kHashStreamClass = 2;

typedef SingleValueModel<size_t, kHashStreamClass> HashStream;  //�����˹�ϣ����,���ڴ��м�������hashֵ

/*------------------------------------------------------------------------------
 * ��  ��: ����һ�������hashֵ
 * ��  ��: [in][out] stream ��ϣ������
 *         [in] t ����˶����hashֵ
 * ����ֵ: ��ϣ������
 * ��  ��:
 *   ʱ�� 2013.10.20
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
template<typename T>
inline HashStream& operator&(HashStream& stream, const T& t)
{
    boost::hash_combine(stream.value_ref(), t);

    return stream;
}

}

#endif  // HEADER_HASH_STREAM
