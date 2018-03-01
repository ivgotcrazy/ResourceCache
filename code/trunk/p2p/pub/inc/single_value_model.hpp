/*##############################################################################
 * 文件名   : single_value_model.hpp
 * 创建人   : rosan 
 * 创建时间 : 2013.10.20
 * 文件描述 : 此文件实现SingleValueModel模板类
 * 版权声明 : Copyright(c)2013 BroadInter.All rights reserved.
 * ###########################################################################*/
#ifndef HEADER_SINGLE_VALUE_MODEL
#define HEADER_SINGLE_VALUE_MODEL

namespace BroadCache
{

/*------------------------------------------------------------------------------
 *描  述: 此类实现对一个数值的包装
 *作  者: rosan
 *时  间: 2013.10.14
 -----------------------------------------------------------------------------*/ 
template<typename T, size_t/*dummy*/>
class SingleValueModel
{
    typedef T ValueType;  // 数值类型
    typedef int DiffType;  // 数值之间的距离类型

public:
    explicit SingleValueModel(ValueType v = ValueType()) : value_(v) {}  // 构造函数，设置一个初值

    inline ValueType value() const { return value_; }  // 获取数值
    inline ValueType& value_ref() { return value_; }  // 获取数值引用
    inline void set_value(ValueType value) { value_ = value; }  // 设置数值
    inline void advance(DiffType diff) { value_ += diff; }  // 将此数值向前递增(向后递减,如果diff是负数的话)若干单位

private:
    ValueType value_; // 被包装的数值
};

}

#endif  // HEADER_SINGLE_VALUE_MODEL
