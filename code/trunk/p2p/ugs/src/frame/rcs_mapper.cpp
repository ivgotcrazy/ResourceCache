/*#############################################################################
 * �ļ���   : rcs_mapper.cpp
 * ������   : tom_liu	
 * ����ʱ�� : 2013��11��21��
 * �ļ����� : Session��ʵ�� ��infohash��rcs֮���ӳ�� 
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#include "rcs_mapper.hpp"
#include "bc_util.hpp"

namespace BroadCache
{
/*-----------------------------------------------------------------------------
 * ��  ��: Ϊһ�����������ӣ������rcs��Դ
 * ��  ��: [in] rcs Rcs��ַ
 *         [in] node ��Դ�ڵ�
 * ����ֵ: 
 * ��  ��:
 *   ʱ�� 2013��11��21��
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
void RcsMapper::AddRcsResource(const ServiceNode& node, const RcsList& rcs)
{
	LOG(INFO) << "Add rcs resouce "; 

	RcsServiceNodes::iterator iter = std::find_if(alive_rcs_nodes_.begin(),
										alive_rcs_nodes_.end(), Compare(node));	
	if (iter != alive_rcs_nodes_.end()) return;

	RcsMapEntry entry(node, rcs);
	alive_rcs_nodes_.push_back(entry);						
}

/*-----------------------------------------------------------------------------
 * ��  ��: �Ƿ������Դ
 * ��  ��: [in] info_hash  ��Դ��info_hash
 * ����ֵ: 
 * ��  ��:
 *   ʱ�� 2013��11��21��
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
bool RcsMapper::ExsitHash(const std::string& info_hash)
{
	bool ret = false;
	for (auto pair : rcs_map_)
	{
		if (pair.second.find(info_hash) != pair.second.end())
		{
			ret = true;
		}
	}
	return ret;
}

/*-----------------------------------------------------------------------------
 * ��  ��: Ϊһ�����������ӣ������info-hash�ļ���Դ 
 * ��  ��: [in] info_hash  ��Դhash�ַ���
 *         [in] rcs Rcs��ַ
 * ����ֵ: 
 * ��  ��:
 *   ʱ�� 2013��11��21��
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
void RcsMapper::AddFileResource(const ServiceNode& node, const std::vector<std::string>& info_hashs)
{
	LOG(INFO) << "Add rcs file resouce "; 
		
	HashCollection hash_set;
	for (std::string hash : info_hashs)
	{
		if (!ExsitHash(hash))
			hash_set.insert(hash);
	}
	rcs_map_.insert(std::make_pair(node, hash_set));
}

/*-----------------------------------------------------------------------------
 * ��  ��: ɾ��һ�������㣬ͬʱҪɾ���˷������Ӧ������rcs��Դ��info-hash�ļ���Դ
 * ��  ��: [in] node ��Դ�ڵ��ַ
 * ����ֵ: 
 * ��  ��:
 *   ʱ�� 2013��11��21��
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
void RcsMapper::RemoveServiceNode(const ServiceNode& node)
{
	LOG(INFO) << "Remove rcs Resouce "; 

	RcsServiceNodes::iterator iter = std::find_if(alive_rcs_nodes_.begin(),
										alive_rcs_nodes_.end(), Compare(node));
	if (iter != alive_rcs_nodes_.end()) alive_rcs_nodes_.erase(iter);
	
	RcsMappingMap::iterator it = rcs_map_.find(node);
	if (it == rcs_map_.end()) return;
	rcs_map_.erase(it);
}

/*-----------------------------------------------------------------------------
 * ��  ��: ��ȡ�ֲ�ʽ���صĴ���rcs��һ���������෵��һ��rcs��ַ, �ݲ����� request, info_hash����������
 * ��  ��: [in] rcs Rcs��ַ
 * ����ֵ: 
 * ��  ��:
 *   ʱ�� 2013��11��21��
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
RcsList RcsMapper::GetRcsProxy(const EndPoint& requester, const std::string& info_hash, uint32 num_want)
{
	LOG(INFO) << "Get rcs proxy"; 
		
	RcsList list;
	uint32 count = 0;
	for (auto service_nodes : alive_rcs_nodes_)
	{
		if (count > num_want) break;
		
		EndPoint& ep = *(service_nodes.node_list.begin()
							+ RandomUniformInt<size_t>(0,service_nodes.node_list.size() - 1));
		list.push_back(ep);
	}
	
    return list;
}
	
/*-----------------------------------------------------------------------------
 * ��  ��: ��ȡһ������ڵ�Ķ���˿ڵ�ַ������peer�ṩ���� 
 * ��  ��: [in] info_hash  ��Դhash�ַ���
 * ����ֵ: Rcs��ַ
 * ��  ��:
 *   ʱ�� 2013��11��21��
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
RcsList RcsMapper::GetPeer(const std::string& info_hash)
{
	LOG(INFO) << "Distribute rcs mapping peer " << ToHex(info_hash); 

	if (alive_rcs_nodes_.empty()) return RcsList();

	RcsMappingMap::iterator iter;
	for (iter = rcs_map_.begin(); iter != rcs_map_.end(); ++iter)
	{
		if (iter->second.find(info_hash) != iter->second.end()) break;
	}

	RcsList rcs_list;
	if (iter != rcs_map_.end()) //�Ѵ���ӳ���ϵ
	{
		EndPoint ep = iter->first;
		RcsServiceNodes::iterator it = std::find_if(alive_rcs_nodes_.begin(),
									alive_rcs_nodes_.end(), Compare(ep));	
		if (it == alive_rcs_nodes_.end()) return rcs_list;

		for (EndPoint& child_ep : it->node_list)
		{
			rcs_list.push_back(child_ep);
		}
	}
	else
	{	
		size_t pos  = boost::hash_value(info_hash) % alive_rcs_nodes_.size();
		RcsServiceNodes::iterator node_iter = alive_rcs_nodes_.begin();
		std::advance(node_iter, pos);

		//������Դ�� rcs_map_��
		RcsMappingMap::iterator map_iter = rcs_map_.find(node_iter->node_name);
		if (map_iter == rcs_map_.end()) return rcs_list;
		map_iter->second.insert(info_hash);

		//��Ӷ˿ڵ�ַ�����ؽڵ���
		for (EndPoint& child_ep : node_iter->node_list)
		{
			rcs_list.push_back(child_ep);
		}
	}
	
 	return rcs_list;
}

/*-----------------------------------------------------------------------------
 * ��  ��: �������е�rcs�ڵ��ַ
 * ��  ��: 
 * ����ֵ: Rcs��ַ
 * ��  ��:
 *   ʱ�� 2013��11��21��
 *   ���� tom_liu
 *   ���� ����
 ----------------------------------------------------------------------------*/
RcsList RcsMapper::GetAllRcs()
{
	RcsList  list;

	for (auto rcs_nodes : alive_rcs_nodes_)
	{
		for (auto node : rcs_nodes.node_list)
		{
			list.push_back(node);
		}
	}

	return list;
}

}
