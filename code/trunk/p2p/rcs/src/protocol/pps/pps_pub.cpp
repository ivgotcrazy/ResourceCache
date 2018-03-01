/*#############################################################################
 * �ļ���   : pps_pub.cpp
 * ������   : tom_liu	
 * ����ʱ�� : 2014��1��3��
 * �ļ����� : BTЭ���ڲ����úꡢ�����������ȶ���
 * ��Ȩ���� : Copyright (c) 2013 BroadInter. All rights reserved.
 * ##########################################################################*/

#include "pps_pub.hpp"
 
namespace BroadCache
{



/*-----------------------------------------------------------------------------
 * ��  ��: �ڲ�У��ͼ��㺯��
 * ��  ��: 
 * ����ֵ: 
 * ��  ��:
 *   ʱ�� 2013.12.24
 *   ���� tom_liu
 *   ���� ����
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
 * ��  ��: У��ͼ��㺯��
 * ��  ��: 
 * ����ֵ: У���
 * ��  ��:
 *   ʱ�� 2013.12.24
 *   ���� tom_liu
 *   ���� ����
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
			by += 32;	 //Сд��ɴ�д
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
 * ��  ��: ����Pps��piece��С����
 * ��  ��: 
 * ����ֵ:
 * ��  ��:
 *   ʱ�� 2013.12.24
 *   ���� tom_liu
 *   ���� ����
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
