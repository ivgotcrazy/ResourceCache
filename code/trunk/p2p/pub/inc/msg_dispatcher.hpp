/*------------------------------------------------------------------------------
 * 文件名   : msg_dispatcher.hpp
 * 创建人   : rosan 
 * 创建时间 : 2013.08.29
 * 文件描述 : 此文件包含MsgDispatcher类
 * 版权声明 : Copyright (c) 2013 BroadInter. All rights reserved.
 -----------------------------------------------------------------------------*/ 
#ifndef HEADER_MSG_DISPATCHER
#define HEADER_MSG_DISPATCHER

#include <boost/function.hpp>
#include <boost/unordered_map.hpp>
#include "bc_typedef.hpp"

namespace BroadCache
{

/*------------------------------------------------------------------------------
 *描  述: 此类是消息回调对象基类
 *作  者: rosan
 *时  间: 2013.08.29
 -----------------------------------------------------------------------------*/ 
class MsgCallbackBase
{
public:
    virtual ~MsgCallbackBase() {}

    virtual void Call(const PbMessage& msg) = 0;  // 消息回调接口
};

/*------------------------------------------------------------------------------
 *描  述: 消息回调对象实现
 *作  者: rosan
 *时  间: 2013.08.29
 -----------------------------------------------------------------------------*/ 
template<class MsgType>
class MsgCallback : public MsgCallbackBase
{
public:
    typedef boost::function<void(const MsgType&)> CallbackFunction;

public:
    explicit MsgCallback(const CallbackFunction& func)
        : func_(func)
    {
    }
    
    virtual void Call(const PbMessage& msg) override  // 消息回调
    {
        func_(dynamic_cast<const MsgType&>(msg));  // 实现多态分发
    }

private:
    CallbackFunction func_;  // 回调函数
};

/*------------------------------------------------------------------------------
 *描  述: 此类实现Msg多态分发机制
 *作  者: rosan
 *时  间: 2013.08.29
 -----------------------------------------------------------------------------*/ 
class MsgDispatcher
{
public:
    // 为某个特定消息注册回调函数
    template<class MsgType>
    void RegisterCallback(const boost::function<void(const MsgType&)>& func)
    {
        callbacks_[MsgType::descriptor()] = 
            boost::shared_ptr<MsgCallbackBase>(new MsgCallback<MsgType>(func));
    }

    template<class MsgType>
    void RegisterCommonCallback(const boost::function<void(const PbMessage&)>& func)
    {
        common_callbacks_[MsgType::descriptor()] = func;
    }

    // 分发收到的消息
    bool Dispatch(const PbMessage& msg)
    {
        auto i = callbacks_.find(msg.GetDescriptor());  // 查找是否注册消息回调
        bool found = (i != callbacks_.end());

        if (found)
        {
            i->second->Call(msg);
        }
        else
        {
            auto j = common_callbacks_.find(msg.GetDescriptor());  // 继续查找
            found = (j != common_callbacks_.end());
            if (found)
            {
                (j->second)(msg);
            }
        }

        return found;
    }

private: 
    // 每一个消息类型对应一个消息回调对象
    boost::unordered_map<const google::protobuf::Descriptor*, boost::shared_ptr<MsgCallbackBase> > callbacks_;
    boost::unordered_map<const google::protobuf::Descriptor*, boost::function<void(const PbMessage&)> > common_callbacks_;
};

}

#endif  // HEADER_MSG_DISPATCHER
