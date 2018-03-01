#ifndef HEADER_FUNCTION_ADAPTOR
#define HEADER_FUNCTION_ADAPTOR

#include <boost/bind.hpp>
#include <boost/function.hpp>

namespace BroadCache
{

/*******************************************************************************
 *描  述: 0元函数适配器，输出1元函数
 *作  者: rosan
 *时  间: 2013.11.15
 ******************************************************************************/
template<class Ret, class R1>
struct NullaryAdaptor1
{
    NullaryAdaptor1(const boost::function<Ret()>& f) : f_(f) {}

    Ret operator()(R1)
    {
        return f_();
    }

    boost::function<Ret()> f_;
};

/*******************************************************************************
 *描  述: 0元函数适配器，输出2元函数
 *作  者: rosan
 *时  间: 2013.11.15
 ******************************************************************************/
template<class Ret, class R1, class R2>
struct NullaryAdaptor2
{
    NullaryAdaptor2(const boost::function<Ret()>& f) : f_(f) {}

    Ret operator()(R1, R2)
    {
        return f_();
    }

    boost::function<Ret()> f_;
};

/*******************************************************************************
 *描  述: 0元函数适配器，输出3元函数
 *作  者: rosan
 *时  间: 2013.11.15
 ******************************************************************************/
template<class Ret, class R1, class R2, class R3>
struct NullaryAdaptor3
{
    NullaryAdaptor3(const boost::function<Ret()>& f) : f_(f) {}

    Ret operator()(R1, R2, R3)
    {
        return f_();
    }

    boost::function<Ret()> f_;
};

/*------------------------------------------------------------------------------
 * 描  述: 从0元函数到1元函数的适配
 * 参  数: [in] f 0元函数
 * 返回值: 1元函数
 * 修  改:
 *   时间 2013.11.15
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
template<class Ret, class R1>
boost::function<Ret(R1)> AdaptFromNullary(const boost::function<Ret()>& f)
{
    return boost::function<Ret(R1)>(NullaryAdaptor1<Ret, R1>(f));
}

/*------------------------------------------------------------------------------
 * 描  述: 从0元函数到2元函数的适配
 * 参  数: [in] f 0元函数
 * 返回值: 2元函数
 * 修  改:
 *   时间 2013.11.15
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
template<class Ret, class R1, class R2>
boost::function<Ret(R1, R2)> AdaptFromNullary(const boost::function<Ret()>& f)
{
    return boost::function<Ret(R1, R2)>(NullaryAdaptor2<Ret, R1, R2>(f));
}

/*------------------------------------------------------------------------------
 * 描  述: 从0元函数到3元函数的适配
 * 参  数: [in] f 0元函数
 * 返回值: 3元函数
 * 修  改:
 *   时间 2013.11.15
 *   作者 rosan
 *   描述 创建
 -----------------------------------------------------------------------------*/ 
template<class Ret, class R1, class R2, class R3>
boost::function<Ret(R1, R2, R3)> AdaptFromNullary(const boost::function<Ret()>& f)
{
    return boost::function<Ret(R1, R2, R3)>(NullaryAdaptor3<Ret, R1, R2, R3>(f));
}

}  // namespace BroadCache

#endif  // HEADER_FUNCTION_ADAPTOR
