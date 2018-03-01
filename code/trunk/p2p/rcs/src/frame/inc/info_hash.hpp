/*#############################################################################
 * �ļ���   : info_hash.hpp
 * ������   : teck_zhou	
 * ����ʱ�� : 2013��8��21��
 * �ļ����� : InfoHash�����������ӿ������͸�����������
 * ##########################################################################*/
#ifndef HEADER_INFO_HASH
#define HEADER_INFO_HASH

#include "bc_typedef.hpp"
#include "rcs_typedef.hpp"
#include "big_number.hpp"
#include "hash_stream.hpp"

namespace BroadCache
{

/*------------------------------------------------------------------------------
 *��  ��: info-hash�Ļ���
 *��  ��: rosan
 *ʱ  ��: 2013.09.17
 -----------------------------------------------------------------------------*/
INTERFACE InfoHash
{
	virtual ~InfoHash() {}
   
    // �жϴ˶����Ƿ����һ��InfoHash������� 
    virtual bool Equal(const InfoHash& info_hash) const = 0;

    // ��ȡ�˶����ԭʼ����
	virtual std::string to_string() const = 0;

    // ��ȡ�˶���16���Ƶ��ַ�����ʾ
    virtual std::string raw_data() const = 0;
};

/*------------------------------------------------------------------------------
 * ��  ��: �ж�����InfoHash�����Ƿ����
 * ��  ��: [in] lhs ��һ��InfoHash����
 *         [in] rhs �ڶ���InfoHash����
 * ����ֵ: ����InfoHash�����Ƿ����
 * ��  ��:
 *   ʱ�� 2013.09.17
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/
inline bool operator==(const InfoHash& lhs, const InfoHash& rhs)
{
    return lhs.Equal(rhs);
}

/*------------------------------------------------------------------------------
 * ��  ��: �ж�����InfoHash�����Ƿ����
 * ��  ��: [in] lhs ��һ��InfoHash����
 *         [in] rhs �ڶ���InfoHash����
 * ����ֵ: ����InfoHash�����Ƿ����
 * ��  ��:
 *   ʱ�� 2013.09.17
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/
inline bool operator!=(const InfoHash& lhs, const InfoHash& rhs)
{
    return !(lhs == rhs);
}

/*------------------------------------------------------------------------------
 * ��  ��: �ж�����InfoHashSP�����Ƿ����
 * ��  ��: [in] lhs ��һ��InfoHashSP����
 *         [in] rhs �ڶ���InfoHashSP����
 * ����ֵ: ����InfoHashSP�����Ƿ����
 * ��  ��:
 *   ʱ�� 2013.10.31
 *   ���� teck_zhou
 *   ���� ����
 -----------------------------------------------------------------------------*/
inline bool operator==(const InfoHashSP& lhs, const InfoHashSP& rhs)
{
    return *lhs == *rhs;
}

/*------------------------------------------------------------------------------
 * ��  ��: ����hashֵ������unorderd_map��hash����
 * ��  ��: [in] info_hash InfoHashSP
 * ����ֵ: hashֵ
 * ��  ��:
 *   ʱ�� 2013��10��31��
 *   ���� teck_zhou
 *   ���� ����
 -----------------------------------------------------------------------------*/
inline std::size_t hash_value(const InfoHashSP& info_hash)
{
	HashStream hash_stream;
	hash_stream & info_hash->to_string();

	return hash_stream.value();
}

/*------------------------------------------------------------------------------
 *��  ��: 16λ��info-hash��
 *��  ��: rosan
 *ʱ  ��: 2013.09.17
 -----------------------------------------------------------------------------*/
class InfoHash16 : public InfoHash
{
public:
    explicit InfoHash16(const std::string& hash);

    virtual bool Equal(const InfoHash& info_hash) const override;
    virtual std::string to_string() const override;
    virtual std::string raw_data() const override;

private:
    std::string hash_; //hash����
};

/*------------------------------------------------------------------------------
 *��  ��: 20λ��info-hash��
 *��  ��: rosan
 *ʱ  ��: 2013.09.17
 -----------------------------------------------------------------------------*/
class InfoHash20 : public InfoHash
{
public:
    explicit InfoHash20(const std::string& hash);

    virtual bool Equal(const InfoHash& info_hash) const override;
    virtual std::string to_string() const override; 
    virtual std::string raw_data() const override;

private:
    std::string hash_; //hash����
};
 
typedef InfoHash20 BtInfoHash; //����btЭ���info-hash
typedef InfoHash20 PpsInfoHash; //����ppsЭ��info_hash

}

#endif
