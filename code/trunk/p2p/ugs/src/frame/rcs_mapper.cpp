/*#############################################################################
 * 文件名   : rcs_mapper.cpp
 * 创建人   : tom_liu	
 * 创建时间 : 2013年11月21日
 * 文件描述 : Session类实现 做infohash和rcs之间的映射 
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#include "rcs_mapper.hpp"
#include "bc_util.hpp"

namespace BroadCache
{
/*-----------------------------------------------------------------------------
 * 描  述: 为一个服务结点增加（多个）rcs资源
 * 参  数: [in] rcs Rcs地址
 *         [in] node 资源节点
 * 返回值: 
 * 修  改:
 *   时间 2013年11月21日
 *   作者 tom_liu
 *   描述 创建
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
 * 描  述: 是否存在资源
 * 参  数: [in] info_hash  资源的info_hash
 * 返回值: 
 * 修  改:
 *   时间 2013年11月21日
 *   作者 tom_liu
 *   描述 创建
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
 * 描  述: 为一个服务结点增加（多个）info-hash文件资源 
 * 参  数: [in] info_hash  资源hash字符串
 *         [in] rcs Rcs地址
 * 返回值: 
 * 修  改:
 *   时间 2013年11月21日
 *   作者 tom_liu
 *   描述 创建
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
 * 描  述: 删除一个服务结点，同时要删除此服务结点对应的所有rcs资源和info-hash文件资源
 * 参  数: [in] node 资源节点地址
 * 返回值: 
 * 修  改:
 *   时间 2013年11月21日
 *   作者 tom_liu
 *   描述 创建
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
 * 描  述: 获取分布式下载的代理rcs，一个服务点最多返回一个rcs地址, 暂不考虑 request, info_hash带来的区别
 * 参  数: [in] rcs Rcs地址
 * 返回值: 
 * 修  改:
 *   时间 2013年11月21日
 *   作者 tom_liu
 *   描述 创建
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
 * 描  述: 获取一个服务节点的多个端口地址给外网peer提供下载 
 * 参  数: [in] info_hash  资源hash字符串
 * 返回值: Rcs地址
 * 修  改:
 *   时间 2013年11月21日
 *   作者 tom_liu
 *   描述 创建
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
	if (iter != rcs_map_.end()) //已存在映射关系
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

		//增加资源到 rcs_map_中
		RcsMappingMap::iterator map_iter = rcs_map_.find(node_iter->node_name);
		if (map_iter == rcs_map_.end()) return rcs_list;
		map_iter->second.insert(info_hash);

		//添加端口地址到返回节点中
		for (EndPoint& child_ep : node_iter->node_list)
		{
			rcs_list.push_back(child_ep);
		}
	}
	
 	return rcs_list;
}

/*-----------------------------------------------------------------------------
 * 描  述: 返回所有的rcs节点地址
 * 参  数: 
 * 返回值: Rcs地址
 * 修  改:
 *   时间 2013年11月21日
 *   作者 tom_liu
 *   描述 创建
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
