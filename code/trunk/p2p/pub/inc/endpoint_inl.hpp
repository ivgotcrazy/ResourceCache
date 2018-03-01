/*------------------------------------------------------------------------------
 *�ļ���  : endpoint_inl.hpp
 *������  : rosan
 *����ʱ��: 2013.08.22
 *�ļ�����: endpoint.hpp�е�EndPoint���ȫ�ֺ���
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
 -----------------------------------------------------------------------------*/ 
#ifndef HEADER_END_POINT_INL
#define HEADER_END_POINT_INL

#include <boost/lexical_cast.hpp>

#include "hash_stream.hpp"
#include "bc_typedef.hpp"

namespace BroadCache
{

/*------------------------------------------------------------------------------
 * ��  ��: EndPoint��Ĺ��캯��
 * ��  ��:
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.10.14
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
inline EndPoint::EndPoint() : ip(0), port(0)
{
}

/*------------------------------------------------------------------------------
 * ��  ��: EndPoint��Ĺ��캯��
 * ��  ��: [in] ep_ip ip��ַ
 *         [in] ep_port �˿ں�
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.10.14
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
inline EndPoint::EndPoint(unsigned long ep_ip, unsigned short ep_port) : ip(ep_ip), port(ep_port)
{
}

/*------------------------------------------------------------------------------
 * ��  ��: �ж�һ��EndPoint�����Ƿ�С����һ��EndPoint����
 * ��  ��: [in] lhs ����������EndPoint����
 *         [in] rhs �������Ҳ��EndPoint����
 * ����ֵ: һ��EndPoint�����Ƿ�С����һ��EndPoint����
 * ��  ��:
 *   ʱ�� 2013.10.14
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
inline bool operator<(const EndPoint& lhs, const EndPoint& rhs)
{
    return (lhs.ip < rhs.ip) || (lhs.ip == rhs.ip && lhs.port < rhs.port);
}

/*------------------------------------------------------------------------------
 * ��  ��: �ж�һ��EndPoint�����Ƿ������һ��EndPoint����
 * ��  ��: [in] lhs ����������EndPoint����
 *         [in] rhs �������Ҳ��EndPoint����
 * ����ֵ: һ��EndPoint�����Ƿ������һ��EndPoint����
 * ��  ��:
 *   ʱ�� 2013.10.14
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
inline bool operator==(const EndPoint& lhs, const EndPoint& rhs)
{
    return !(lhs < rhs) && !(rhs < lhs);
}

/*------------------------------------------------------------------------------
 * ��  ��: �ж�һ��EndPoint�����Ƿ������һ��EndPoint����
 * ��  ��: [in] lhs ����������EndPoint����
 *         [in] rhs �������Ҳ��EndPoint����
 * ����ֵ: һ��EndPoint�����Ƿ������һ��EndPoint����
 * ��  ��:
 *   ʱ�� 2013.10.14
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
inline bool operator>(const EndPoint& lhs, const EndPoint& rhs)
{
   return !(lhs < rhs) && (lhs != rhs); 
}

/*------------------------------------------------------------------------------
 * ��  ��: �ж�һ��EndPoint�����Ƿ񲻵�����һ��EndPoint����
 * ��  ��: [in] lhs ����������EndPoint����
 *         [in] rhs �������Ҳ��EndPoint����
 * ����ֵ: һ��EndPoint�����Ƿ񲻵�����һ��EndPoint����
 * ��  ��:
 *   ʱ�� 2013.10.14
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
inline bool operator!=(const EndPoint& lhs, const EndPoint& rhs)
{
    return !(lhs == rhs);
}

/*------------------------------------------------------------------------------
 * ��  ��: �ж�һ��EndPoint�����Ƿ�С�ڻ������һ��EndPoint����
 * ��  ��: [in] lhs ����������EndPoint����
 *         [in] rhs �������Ҳ��EndPoint����
 * ����ֵ: һ��EndPoint�����Ƿ�С�ڻ������һ��EndPoint����
 * ��  ��:
 *   ʱ�� 2013.10.14
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
inline bool operator<=(const EndPoint& lhs, const EndPoint& rhs)
{
    return !(lhs > rhs);
}

/*------------------------------------------------------------------------------
 * ��  ��: �ж�һ��EndPoint�����Ƿ���ڻ������һ��EndPoint����
 * ��  ��: [in] lhs ����������EndPoint����
 *         [in] rhs �������Ҳ��EndPoint����
 * ����ֵ: һ��EndPoint�����Ƿ���ڻ������һ��EndPoint����
 * ��  ��:
 *   ʱ�� 2013.10.14
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
inline bool operator>=(const EndPoint& lhs, const EndPoint& rhs)
{
    return !(lhs < rhs);
}

/*------------------------------------------------------------------------------
 * ��  ��: ��һ������ת����boost���е�ip��ַ����
 * ��  ��: [in] ip ����һ��ip��ַ������
 * ����ֵ: boost���е�ip��ַ����
 * ��  ��:
 *   ʱ�� 2013.10.14
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
inline boost::asio::ip::address to_address(unsigned long ip)
{
    // return boost::asio::ip::address(boost::asio::ip::address_v4(ip));
    return boost::asio::ip::address(boost::asio::ip::address_v4(uint32(ip)));
}

/*------------------------------------------------------------------------------
 * ��  ��: ��һ�����ʮ���Ƶ�ip��ַ�ַ���ת����һ������
 * ��  ��: [in] ip ���ʮ���Ƶ�ip��ַ�ַ���
 * ����ֵ: ����һ��ip��ַ������
 * ��  ��:
 *   ʱ�� 2013.10.14
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
inline unsigned long to_address_ul(const std::string& ip)
{
    return to_address_ul(boost::asio::ip::address::from_string(ip));
}

/*------------------------------------------------------------------------------
 * ��  ��: ��һ��boost���е�ip��ַ����ת��������
 * ��  ��: [in] ip boost���е�ip��ַ����
 * ����ֵ: ����һ��ip��ַ������
 * ��  ��:
 *   ʱ�� 2013.10.14
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
inline unsigned long to_address_ul(const boost::asio::ip::address& ip)
{
    return ip.to_v4().to_ulong();
}

/*------------------------------------------------------------------------------
 * ��  ��: ��һ��EndPoint����ת����udpЭ���endpoint����
 * ��  ��: [in] endpoint EndPoint����
 * ����ֵ: udpЭ���endpoint����
 * ��  ��:
 *   ʱ�� 2013.10.14
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
inline boost::asio::ip::udp::endpoint to_udp_endpoint(const EndPoint& endpoint)
{
    return boost::asio::ip::udp::endpoint(to_address(endpoint.ip), endpoint.port);
}

/*------------------------------------------------------------------------------
 * ��  ��: ��һ��EndPoint����ת����tcpЭ���endpoint����
 * ��  ��: [in] endpoint EndPoint����
 * ����ֵ: tcpЭ���endpoint����
 * ��  ��:
 *   ʱ�� 2013.10.14
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
inline boost::asio::ip::tcp::endpoint to_tcp_endpoint(const EndPoint& endpoint)
{
    return boost::asio::ip::tcp::endpoint(to_address(endpoint.ip), endpoint.port);
}

/*------------------------------------------------------------------------------
 * ��  ��: ��һ��udpЭ���endpoint����ת����EndPoint����
 * ��  ��: [in] endpoint udpЭ���endpoint����
 * ����ֵ: EndPoint����
 * ��  ��:
 *   ʱ�� 2013.10.14
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
inline EndPoint to_endpoint(const boost::asio::ip::udp::endpoint& endpoint)
{
    return EndPoint(endpoint.address().to_v4().to_ulong(), endpoint.port());
}

/*------------------------------------------------------------------------------
 * ��  ��: ��һ��tcpЭ���endpoint����ת����EndPoint����
 * ��  ��: [in] endpoint tcpЭ���endpoint����
 * ����ֵ: EndPoint����
 * ��  ��:
 *   ʱ�� 2013.10.14
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
inline EndPoint to_endpoint(const boost::asio::ip::tcp::endpoint& endpoint)
{
    return EndPoint(endpoint.address().to_v4().to_ulong(), endpoint.port());
}

/*------------------------------------------------------------------------------
 * ��  ��: ��һ��������ʽ��ip��ַת����һ�����ʮ���Ƶ��ַ���
 * ��  ��: [in] ip ������ʽ��ip��ַ
 * ����ֵ: ���ʮ���Ƶ��ַ���
 * ��  ��:
 *   ʱ�� 2013.10.14
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
inline std::string to_string(unsigned long ip)
{
    return boost::asio::ip::address_v4(ip).to_string();
}

/*------------------------------------------------------------------------------
 * ��  ��: ��һ��EndPoint����ת�����ַ���
 *         �ǵ��ʮ����ip��ַ�Ӷ˿ںŸ�ʽ��(<ip>:<port>)����192.168.0.1:8878
 * ��  ��: [in] endpoint EndPoint����
 * ����ֵ: ת������ַ���
 * ��  ��:
 *   ʱ�� 2013.10.14
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
inline std::string to_string(const EndPoint& endpoint)
{
    return to_string(endpoint.ip) + ':'
        + boost::lexical_cast<std::string>(endpoint.port);
}

/*------------------------------------------------------------------------------
 *��  ��: ����EndPoint�����hashֵ
 *��  ��: [in] endpoint EndPoint����
 *����ֵ: ��EndPoint�����hashֵ 
 *��  ��:
 *  ʱ�� 2013.08.22
 *  ���� rosan
 *  ���� ����
 -----------------------------------------------------------------------------*/ 
inline std::size_t hash_value(const EndPoint& endpoint)
{
    HashStream hash_stream;
    hash_stream & endpoint.ip & endpoint.port;

    return hash_stream.value();
}

/*------------------------------------------------------------------------------
 * ��  ��: ��EndPoint�������������
 * ��  ��: [in][out] stream ������ 
 *         [in] endpoint EndPoint����
 * ����ֵ: ���EndPoint������������
 * ��  ��:
 *   ʱ�� 2013.10.14
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
template<class Stream>
inline Stream& operator<<(Stream& stream, const EndPoint& endpoint)
{
    stream << to_string(endpoint);

    return stream;
}

/*------------------------------------------------------------------------------
 * ��  ��: ��һ��EndPoint�������л�
 * ��  ��: [in][out] serializer ���л���
 *         [in] ep �����л���EndPoint����
 * ����ֵ: ���л�EndPoint���������л���
 * ��  ��:
 *   ʱ�� 2013.10.14
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
inline Serializer& serialize(Serializer& serializer, const EndPoint& ep)
{
    return serializer & ep.ip & ep.port;
}

/*------------------------------------------------------------------------------
 * ��  ��: ��һ��EndPoint�������л�
 * ��  ��: [in][out] unserializer �����л���
 *         [out] ep �������л���EndPoint����
 * ����ֵ: �����л�EndPoint�����ķ����л���
 * ��  ��:
 *   ʱ�� 2013.10.14
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
inline Unserializer& unserialize(Unserializer& unserializer, EndPoint& ep)
{
    return unserializer & ep.ip & ep.port;
}

/*------------------------------------------------------------------------------
 * ��  ��: ��ȡһ��EndPoint�������л����軺������С
 * ��  ��: [in][out] size_helper ���ڻ�ȡĳ���������л����軺������С
 *         [in] ep EndPoint����
 * ����ֵ: �����SizeHelper����
 * ��  ��:
 *   ʱ�� 2013.10.14
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
inline SizeHelper& serialize_size(SizeHelper& size_helper, const EndPoint& ep)
{
    return size_helper & ep.ip & ep.port;
}

}

#endif  // HEADER_END_POINT_INL
