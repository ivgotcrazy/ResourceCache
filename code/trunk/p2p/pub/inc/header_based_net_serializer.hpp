/*##############################################################################
 * �ļ���   : header_based_net_serializer.hpp
 * ������   : rosan 
 * ����ʱ�� : 2013.11.11
 * �ļ����� : ���ļ���Ҫ��������ģ����HeaderBasedNetSerializer��HeaderBasedNetUnserializer 
 * ��Ȩ���� : Copyright(c)2013 BroadInter.All rights reserved.
 * ###########################################################################*/
#ifndef HEADER_HEADER_BASED_NET_SERIALIZER
#define HEADER_HEADER_BASED_NET_SERIALIZER

#include "bc_assert.hpp"
#include "net_byte_order.hpp"

namespace BroadCache
{

/*------------------------------------------------------------------------------
 * ��  ��: ��ȡ������Ϣͷ��������������Ϣ�ĳ��ȣ�����Ϣ�ܳ��ȣ�������Ϣͷ����
 * ��  ��: [in] buf ��Ϣ����
 *         [in] length ��Ϣ����
 * ����ֵ: ����Ϣ���ܳ���
 * ��  ��:
 *   ʱ�� 2013.11.12
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
template<typename HeaderDataType>
inline msg_length_t GetHeaderBasedMsgLength(const char* buf, msg_length_t length)
{
    BC_ASSERT(sizeof(HeaderDataType) <= length);

    return msg_length_t(NetToHost<HeaderDataType>(buf) + sizeof(HeaderDataType));
}

/*******************************************************************************
 *��  ��: ������Ҫʵ������Ϣͷ���Զ������Ϣ���ȵĹ���
          ģ�����HeaderDataTypeָ������Ϣͷ������Ϣ���ȵ���������
 *��  ��: rosan
 *ʱ  ��: 2013.11.11
 ******************************************************************************/
template<typename HeaderDataType>
class HeaderBasedNetSerializer
{
public:
    typedef HeaderDataType HeaderType;  // ��Ϣͷ������Ϣ���ȵ���������

public:
    explicit HeaderBasedNetSerializer(char* buf)
        : begin_(buf), net_serializer_(buf + sizeof(HeaderType))
    {
    }

    // ��������Ϣ����Ϣ�岿�������ϣ��������Ϣͷ������
    HeaderType Finish()
    {
        HeaderType length(net_serializer_.value() - begin_);  // ��ȡ��Ϣ�峤��
        
        // �����Ϣͷ��
        net_serializer_.set_value(begin_);
        net_serializer_ & HeaderType(length - sizeof(HeaderType));

        return length; 
    }

    // ��������ת��������
    operator NetSerializer&()
    {
        return net_serializer_;
    }

    // ����ָ�������
    NetSerializer* operator->()
    {
        return &net_serializer_;
    }

private:
    char* begin_;  // ��Ϣͷ��λ��
    NetSerializer net_serializer_;  // �������л�����
};

/*******************************************************************************
 *��  ��: ������Ҫʵ���Զ�������Ϣͷ���Ĺ���
          ģ�����HeaderDataTypeָ������Ϣͷ������Ϣ���ȵ���������
 *��  ��: rosan
 *ʱ  ��: 2013.11.11
 ******************************************************************************/
template<typename HeaderDataType>
class HeaderBasedNetUnserializer
{
public:
    typedef HeaderDataType HeaderType;  // ��Ϣͷ������Ϣ���ȵ���������

public:
    //������Ϣͷ�������ȣ�
    explicit HeaderBasedNetUnserializer(const char* buf)
        : begin_(buf), net_unserializer_(buf + sizeof(HeaderType))
    {
    }

    // ��ȡ��Ϣ���ܳ��ȣ�������Ϣͷ����
    HeaderType GetLength() const
    {
        return NetToHost<HeaderType>(begin_) + sizeof(HeaderType);
    }

    // ��������ת��������
    operator NetUnserializer&()
    {
        return net_unserializer_;
    }

    // ����ָ�������
    NetUnserializer* operator->()
    {
        return &net_unserializer_;
    }

private:
    const char* begin_;  // ��Ϣͷ��λ��
    NetUnserializer net_unserializer_;  // ���練���л�����
}; 

}  // namespace BroadCache

#endif  // HEADER_HEADER_BASED_NET_SERIALIZER
