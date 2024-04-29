#pragma once
#include <memory>
#include <functional>

/*
定义回调函数合集
*/

class Buffer;
class TcpConnection;
class Timestamp;

using TcpConnectionPtr=std::shared_ptr<TcpConnection>;//指向TcpConnection的智能指针
using ConnectionCallback=std::function<void(const TcpConnectionPtr&)>;
using CloseCallback=std::function<void(const TcpConnectionPtr&)>;
using WriteCompleteCallback=std::function<void(const TcpConnectionPtr&)>;
//防止发送接收速率相差过大
using HighWaterMarkCallback=std::function<void(const TcpConnectionPtr&,size_t)>;//size_t是未发送完的数据长度 

using MessageCallback=std::function<void(const TcpConnectionPtr&,Buffer*,Timestamp)>;