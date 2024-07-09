#ifndef CHATSERVICE_H
#define CHATSERVICE_H

#include<muduo/net/TcpConnection.h>
#include<unordered_map>
#include<functional>
#include<mutex>
#include"json.hpp"
#include"public.hpp"
#include"usermodel.hpp"
#include"OfflineMsgModel.hpp"
#include"friendmodel.hpp"
#include"groupmodel.hpp"
#include"redis.hpp"

using namespace std;
using namespace muduo;
using namespace muduo::net;
using json=nlohmann::json;
using MsgHandler=std::function<void(const TcpConnectionPtr &conn, json &js,Timestamp)>;


class ChatService{

public:
    //获取单列对象的接口函数
    static ChatService* instance(); 
    //处理登录业务
    void login(const TcpConnectionPtr &conn,json &js,Timestamp time);

    //处理注册业务
    void reg(const TcpConnectionPtr &conn,json &js,Timestamp time);

    //获取消息对应的处理器
    MsgHandler getHandler(int msgid);

    //处理客户端异常退出
    void clientCloseException(const TcpConnectionPtr &conn);

    // 一对一聊天业务
    void oneChat(const TcpConnectionPtr &conn,json &js,Timestamp time);

    //处理服务器异常后，重置user表的状态信息
    void reset();

    //添加好友业务
    void addFriend(const TcpConnectionPtr &conn,json &js,Timestamp time);

    //创建群组业务
    void createGroup(const TcpConnectionPtr &conn,json &js,Timestamp time);

    //加入群组业务
    void addGroup(const TcpConnectionPtr &conn,json &js,Timestamp time);

    //群聊天业务
    void groupChat(const TcpConnectionPtr &conn,json &js,Timestamp time);

    //处理注销业务
    void quit(const TcpConnectionPtr &conn,json &js,Timestamp time);

    //处理redis回调消息
    void handleRedisSubcribeMessage(int channel,string message);


private:
    ChatService();

    //存储消息id和其对应的业务处理方法
    unordered_map<int,MsgHandler> _msgHandlerMap;

    //数据操作类对象
    UserModel _userModel;

    OfflineMsgModel _offlinemsgmodel;

    FriendModel _friendmodel;

    GroupModel _groupmodel;

    //存储在线用户的通信连接
    unordered_map<int ,TcpConnectionPtr> _userConnmap;

    //定义互斥锁，保证_userConnMap的线程安全
    mutex _connmutex;

    //redis操作对象
    Redis _redis;
    

};

#endif