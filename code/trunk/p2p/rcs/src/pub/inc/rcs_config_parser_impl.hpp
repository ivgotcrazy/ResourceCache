/*#############################################################################
 * �ļ���   : rcs_config_parser_impl.hpp
 * ������   : tom_liu	
 * ����ʱ�� : 2013-8-19
 * �ļ����� : rcs_config_parser ͷ�ļ���ģ�庯���Ķ���
 * ##########################################################################*/

#ifndef HEADER_RCS_CONFIG_PARSER_IMPL
#define HEADER_RCS_CONFIG_PARSER_IMPL

#include "depend.hpp"
#include "rcs_config_parser.hpp"

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
bool RcsConfigParser::GetValue(const std::string& key, T &t)
{
	bool ret = false;
	RcsConfigType type;
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


