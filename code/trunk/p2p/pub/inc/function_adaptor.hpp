#ifndef HEADER_FUNCTION_ADAPTOR
#define HEADER_FUNCTION_ADAPTOR

#include <boost/bind.hpp>
#include <boost/function.hpp>

namespace BroadCache
{

/*******************************************************************************
 *��  ��: 0Ԫ���������������1Ԫ����
 *��  ��: rosan
 *ʱ  ��: 2013.11.15
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
 *��  ��: 0Ԫ���������������2Ԫ����
 *��  ��: rosan
 *ʱ  ��: 2013.11.15
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
 *��  ��: 0Ԫ���������������3Ԫ����
 *��  ��: rosan
 *ʱ  ��: 2013.11.15
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
 * ��  ��: ��0Ԫ������1Ԫ����������
 * ��  ��: [in] f 0Ԫ����
 * ����ֵ: 1Ԫ����
 * ��  ��:
 *   ʱ�� 2013.11.15
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
template<class Ret, class R1>
boost::function<Ret(R1)> AdaptFromNullary(const boost::function<Ret()>& f)
{
    return boost::function<Ret(R1)>(NullaryAdaptor1<Ret, R1>(f));
}

/*------------------------------------------------------------------------------
 * ��  ��: ��0Ԫ������2Ԫ����������
 * ��  ��: [in] f 0Ԫ����
 * ����ֵ: 2Ԫ����
 * ��  ��:
 *   ʱ�� 2013.11.15
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
template<class Ret, class R1, class R2>
boost::function<Ret(R1, R2)> AdaptFromNullary(const boost::function<Ret()>& f)
{
    return boost::function<Ret(R1, R2)>(NullaryAdaptor2<Ret, R1, R2>(f));
}

/*------------------------------------------------------------------------------
 * ��  ��: ��0Ԫ������3Ԫ����������
 * ��  ��: [in] f 0Ԫ����
 * ����ֵ: 3Ԫ����
 * ��  ��:
 *   ʱ�� 2013.11.15
 *   ���� rosan
 *   ���� ����
 -----------------------------------------------------------------------------*/ 
template<class Ret, class R1, class R2, class R3>
boost::function<Ret(R1, R2, R3)> AdaptFromNullary(const boost::function<Ret()>& f)
{
    return boost::function<Ret(R1, R2, R3)>(NullaryAdaptor3<Ret, R1, R2, R3>(f));
}

}  // namespace BroadCache

#endif  // HEADER_FUNCTION_ADAPTOR
