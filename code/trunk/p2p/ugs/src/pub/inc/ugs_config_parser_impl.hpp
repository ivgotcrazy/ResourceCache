/*#############################################################################
 * �ļ���   : ugs_config_parser_impl.hpp
 * ������   : tom_liu	
 * ����ʱ�� : 2013-8-19
 * �ļ����� : ugs_config_parser ͷ�ļ���ģ�庯���Ķ���
 * ##########################################################################*/
#ifndef HEADER_UGS_CONFIG_PARSER_IMPL
#define HEADER_UGS_CONFIG_PARSER_IMPL

#include "ugs_config_parser.hpp"

namespace BroadCache
{
/*-----------------------------------------------------------------------------
 * ��  ��: ��ȡ������ڣ��ô˺������в�����ȡ
 * ��  ��: [in] const string& key  ������
 *         [out] T &t ���ͷ��صĲ���ֵ
 * ����ֵ: ��ȡ�����Ƿ�ɹ�
 * ��  ��:
 * ʱ�� 2013��08��19��
 * ���� tom_liu
 * ���� ����
 ----------------------------------------------------------------------------*/
template<typename T>
bool UgsConfigParser::GetValue(const std::string& key, T &t)
{
	bool ret = false;
	UgsConfigType type;
	std::string child_key;
	ParseConfigTypeAndConfigItem(key, type, child_key);

	BasicParserMap::iterator it = basic_config_map_.find(type);
	if (it == basic_config_map_.end())
	{
		LOG(ERROR) << "Can't find the basic config parser ";
	 	return ret;
	}
	
	ret = it->second->GetValue(child_key, t);	
	return ret;
}

}


#endif


