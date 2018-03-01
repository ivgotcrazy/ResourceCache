/*##############################################################################
 * �ļ���   : single_value_model.hpp
 * ������   : rosan 
 * ����ʱ�� : 2013.10.20
 * �ļ����� : ���ļ�ʵ��SingleValueModelģ����
 * ��Ȩ���� : Copyright(c)2013 BroadInter.All rights reserved.
 * ###########################################################################*/
#ifndef HEADER_SINGLE_VALUE_MODEL
#define HEADER_SINGLE_VALUE_MODEL

namespace BroadCache
{

/*------------------------------------------------------------------------------
 *��  ��: ����ʵ�ֶ�һ����ֵ�İ�װ
 *��  ��: rosan
 *ʱ  ��: 2013.10.14
 -----------------------------------------------------------------------------*/ 
template<typename T, size_t/*dummy*/>
class SingleValueModel
{
    typedef T ValueType;  // ��ֵ����
    typedef int DiffType;  // ��ֵ֮��ľ�������

public:
    explicit SingleValueModel(ValueType v = ValueType()) : value_(v) {}  // ���캯��������һ����ֵ

    inline ValueType value() const { return value_; }  // ��ȡ��ֵ
    inline ValueType& value_ref() { return value_; }  // ��ȡ��ֵ����
    inline void set_value(ValueType value) { value_ = value; }  // ������ֵ
    inline void advance(DiffType diff) { value_ += diff; }  // ������ֵ��ǰ����(���ݼ�,���diff�Ǹ����Ļ�)���ɵ�λ

private:
    ValueType value_; // ����װ����ֵ
};

}

#endif  // HEADER_SINGLE_VALUE_MODEL
