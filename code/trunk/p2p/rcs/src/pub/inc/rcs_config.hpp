/*#############################################################################
 * �ļ���   : rcs_config.hpp
 * ������   : teck_zhou	
 * ����ʱ�� : 2013��8��21��
 * �ļ����� : ���ļ������˶�RCS����������Ч������ѡ�����:
 *            1) ���ú꣬���ó��������ʺϷŵ������ļ��н�������
 *            2) ����꣬���Ʊ���
 *            3) ȫ�ֺ��const����
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#ifndef HEADER_RCS_CONFIG
#define HEADER_RCS_CONFIG

namespace BroadCache
{

// �����ļ�·��
#define GLOBAL_CONFIG_PATH 	 "../config/rcs.conf"
#define BT_CONFIG_PATH   	 "../config/rcs-bt.conf"
#define XL_CONFIG_PATH   	 "../config/rcs-xl.conf"
#define PPS_CONFIG_PATH   	 "../config/rcs-pps.conf"
#define PPL_CONFIG_PATH   	 "../config/rcs-ppl.conf"
#define IP_FILTER_PATH       "../config/ipfilter.conf"

// IO��/д�����������
#define MAX_READ_CACHE_NUM	1024
#define MAX_WRITE_CACHE_NUM 1024

// �ڴ��������� - MB: memory buffer
#define MB_DATA_BLOCK_SIZE	16 * 1024
#define MB_MSG_COMMON_SIZE	16 * 1024
#define MB_MSG_DATA_SIZE	17 * 1024
#define MB_MAX_NUM			 8 * 1024

// IO��������

// BLOCK��С��������BLOCK��С����Э����صģ������ȴӼ���
// ��Ϊ����Э�鶨���BLOCK��С������ͬ�ģ���λ��Byte
#define BT_BLOCK_SIZE		16 * 1024
#define DEFAULT_FRAGMENT_SIZE	16 * 1024  //����PPS,��ʱȥ������

#define PPS_FRAGMENT_SIZE	1 * 1024

#define PPS_BLOCK_SIZE		16 * 1024

#define UDP_MAX_PACKET_LEN  (2 * 1024) 

// ���Կ���
#ifndef BC_DEBUG
#define BC_DEBUG
#endif

// ÿ�δ�PiecePicker�������ص�block����
#define APPLY_DOWNLOAD_BLOCK_NUM	1

// �ֲ�ʽ���ر�����Ϣħ����
#define DISTRIBUTED_DOWNLOAD_MAGIC_NUMBER 0xFFFFC593


}

#endif
