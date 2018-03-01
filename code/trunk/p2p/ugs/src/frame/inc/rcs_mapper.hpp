/*#############################################################################
 * 文件名   : rcs_mapper.hpp
 * 创建人   : tom_liu	
 * 创建时间 : 2013年11月21日
 * 文件描述 : BtRcsMapper类声明
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#ifndef HEAD_BT_RCS_MAPPER
#define HEAD_BT_RCS_MAPPER

#include "ugs_typedef.hpp"
#include "bt_types.hpp"
#include "endpoint.hpp"

namespace BroadCache
{
typedef std::vector<EndPoint> RcsList;

/*------------------------------------------------------------------------------
 *描  述: 用于Rcs映射
 *作  者: tom_liu
 *时  间: 2013.12.24
 -----------------------------------------------------------------------------*/ 
class RcsMapper
{
public:
    void AddRcsResource(const ServiceNode& node, const RcsList& rcs);

    void AddFileResource(const ServiceNode& node, const std::vector<std::string>& info_hashs);

    void RemoveServiceNode(const ServiceNode& node);

    RcsList GetRcsProxy(const EndPoint& requester, const std::string& info_hash, uint32 num_want);
	
    RcsList GetPeer(const std::string& info_hash);

	RcsList GetAllRcs();
     
private:
	bool ExsitHash(const std::string& info_hash);

private:
	struct RcsMapEntry
	{
		RcsMapEntry(EndPoint ep, RcsList list): node_name(ep), node_list(list)
		{
		}
		
		EndPoint  node_name;
		RcsList   node_list;
	};

	struct Compare
	{
		Compare(EndPoint point): ep(point)
		{		
		}
		
		bool operator ()(RcsMapEntry& current)
		{
			return ep == current.node_name ? true : false;
		}	

		EndPoint ep;
	};

	
private:
	typedef boost::unordered_set<std::string> HashCollection;
	typedef boost::unordered_map<EndPoint, HashCollection> RcsMappingMap;
	RcsMappingMap rcs_map_;
	typedef std::vector<RcsMapEntry> RcsServiceNodes;
	RcsServiceNodes alive_rcs_nodes_;
};

}

#endif
