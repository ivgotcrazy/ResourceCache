/*#############################################################################
 * 文件名   : pps_pub.cpp
 * 创建人   : tom_liu	
 * 创建时间 : 2014年1月3日
 * 文件描述 : BT协议内部公用宏、函数、常量等定义
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#include "pps_pub.hpp"
 
namespace BroadCache
{



/*-----------------------------------------------------------------------------
 * 描  述: 内部校验和计算函数
 * 参  数: 
 * 返回值: 
 * 修  改:
 *   时间 2013.12.24
 *   作者 tom_liu
 *   描述 创建
 ----------------------------------------------------------------------------*/
uint64 CalcInnerChecksum(uint64 a1, unsigned char a2)
{
  int v2; // ecx@1
  uint64 result; // qax@2

  v2 = 64 - a2;
  if ((unsigned char)v2 >= 0x40u)
	result = 0;
  else
	result = (uint64)(5700357406446321664 * a1) >> v2;
  return result;
}

/*-----------------------------------------------------------------------------
 * 描  述: 校验和计算函数
 * 参  数: 
 * 返回值: 校验和
 * 修  改:
 *   时间 2013.12.24
 *   作者 tom_liu
 *   描述 创建
 ----------------------------------------------------------------------------*/
uint16 CalcChecksum(unsigned char * src, uint32 len, unsigned char a3)
{
  unsigned char v3;

  uint16 result;
  uint64 a_value;

  v3 = 0;
  if (src != NULL)
  {
	a_value = 0;
	for (uint32 i=0; i<len; i++)
	{
	  unsigned char by = src[i];
	  if (src[i] != 0)
	  {
		if (by >= 65)
		{
		  if (by <= 90)
			by += 32;	 //小写变成大写
		}

		uint64 tmp = (uint64)((signed char)by) << (8 * v3);
		a_value ^= tmp;
		if (v3 < 6)
		{
		  v3++;
		}
		else
		{
		  v3 = 0;
		}
	  }
	}
	result = (uint16)CalcInnerChecksum(a_value, a3);
  }
  else
  {
	result = 0;
  }
  return result;
}

/*-----------------------------------------------------------------------------
 * 描  述: 计算Pps的piece大小函数
 * 参  数: 
 * 返回值:
 * 修  改:
 *   时间 2013.12.24
 *   作者 tom_liu
 *   描述 创建
 ----------------------------------------------------------------------------*/
uint32 CalcPpsPieceSize(uint32 file_size, uint32 piece_count)
{
	uint32 value = file_size / piece_count;
	uint32 spare = 0;
	uint32 multiply_num  = 1;
	while(value > 1024)
	{
		value = value / 1024;
		spare = value % 1024;
		multiply_num = multiply_num * 1024;
	}

	uint32 piece_size = 0;
	if (spare == 0)
		piece_size = value * multiply_num;
	else 
		piece_size = (value + 1) * multiply_num;
	
	return piece_size;
}

}
