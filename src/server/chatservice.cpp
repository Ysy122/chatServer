#include"chatservice.hpp"
#include<muduo/base/Logging.h>
#include<iostream>

#include<vector>
using namespace std;
using namespace muduo;


ChatService* ChatService::instance()
{
    static ChatService service;
    return &service;
}


//注册消息以及对应的handler回调操作
ChatService::ChatService(){
    _msgHandlerMap.insert({LOGIN_MSG,std::bind(&ChatService::login,this,_1,_2,_3)});

    //
    _msgHandlerMap.insert({LOGINOUT_MSG,std::bind(&ChatService::quit,this,_1,_2,_3)});

    _msgHandlerMap.insert({REG_MSG,std::bind(&ChatService::reg,this,_1,_2,_3)});
    _msgHandlerMap.insert({ONE_CHAT_MSG,std::bind(&ChatService::oneChat,this,_1,_2,_3)});
    _msgHandlerMap.insert({ADD_FRIEND_MSG,std::bind(&ChatService::addFriend,this,_1,_2,_3)});

    //群组管理相关事件处理回调注册
    _msgHandlerMap.insert({CREATE_GROUP_MSG,std::bind(&ChatService::createGroup,this,_1,_2,_3)});
    _msgHandlerMap.insert({ADD_GROUP_MSG,std::bind(&ChatService::addGroup,this,_1,_2,_3)});
    _msgHandlerMap.insert({GROUP_CHAT_MSG,std::bind(&ChatService::groupChat,this,_1,_2,_3)});

    //连接redis服务器
    if(_redis.connect()){
        //设置上报消息的回调
        _redis.init_notify_handler(std::bind(&ChatService::handleRedisSubcribeMessage,this,_1,_2));
    }

}

//获取消息对应的处理器
MsgHandler ChatService::getHandler(int msgid){
    //记录错误日志，msgid没有对应的事件处理回调
    auto it =_msgHandlerMap.find(msgid);
    if(it==_msgHandlerMap.end()){
        return [=](const TcpConnectionPtr &conn ,json &js,Timestamp time){
             LOG_ERROR<<"msgid:"<<msgid<<"can not find handler!";
        };
    
    }else{
        return _msgHandlerMap[msgid];
    }
}

void ChatService::login(const TcpConnectionPtr &conn,json &js,Timestamp time){

    int id=js["id"].get<int>();
    string password=js["password"];

    User user=_userModel.query(id);

    if(user.getPassword()==password&&user.getId()==id){
        if(user.getState()=="online"){
            //该用户已经登录，不允许重复登录
            json response;
            response["msgid"]=LOGIN_MSG_ACK;
            response["errno"]=2;
            response["errmsg"]="该账号已经登录，请重新输入新账号 ";
            conn->send(response.dump());
        }else{
            //登录成功
            //记录用户连接信息
            {
                lock_guard<mutex> lock(_connmutex);
                _userConnmap.insert({id,conn});
            }

            //id用户登录成功后，向redis订阅channel
           _redis.subscribe(id);


            //将数据库中的在线状态刷新
            user.setState("online");


            _userModel.updatestate(user);

            json response;
            response["msgid"]=LOGIN_MSG_ACK;
            response["errno"]=0;
            response["id"]=user.getId();
            response["name"]=user.getName();


            cout<<"3333333333333"<<endl;
            //查询用户是否有离线消息
            vector<string> vec=_offlinemsgmodel.query(id);
            if(!vec.empty()){
                response["offlinemsg"]=vec;
                cout<<"4444444444444"<<endl;
                _offlinemsgmodel.remove(id);
            }

            cout<<"555555555555555555"<<endl;
            //查询用户的好有信息并返回
            vector<User> uservec=_friendmodel.query(id);
            if(!uservec.empty()){
                vector<string> vec2;
                for(User &user:uservec){
                    json js;
                    js["friendid"]=user.getId();
                    js["friendname"]=user.getName();
                    js["friendstate"]=user.getState();
                    vec2.push_back(js.dump());
                }
                response["friends"]=vec2;
            }

            //查询用户的群组信息
            vector<Group> groupuserVec=_groupmodel.queryGroups(id);
            if(!groupuserVec.empty()){
                vector<string> groupV;
                for(Group &group:groupuserVec){
                    json grpjs;
                    grpjs["id"]=group.getId();
                    grpjs["groupname"]=group.getName();
                    grpjs["groupdesc"]=group.getDesc();
                    vector<string> userV;
                    for(GroupUser &user:group.getUsers()){
                        json js;
                        js["id"]=user.getId();
                        js["name"]=user.getName();
                        js["state"]=user.getState();
                        js["role"]=user.getRole();
                        userV.push_back(js.dump());

                    }
                    grpjs["users"]=userV;
                    groupV.push_back(grpjs.dump());
                }

                response["groups"]=groupV;
            }

            conn->send(response.dump());

        }
    }else{
        //登录失败
        json response;
        response["msgid"]=LOGIN_MSG_ACK;
        response["errno"]=1;
        response["errmsg"]="用户名或密码错误";
        conn->send(response.dump());
    }
    LOG_INFO<<"do login service";
}


void ChatService::reg(const TcpConnectionPtr &conn,json &js,Timestamp time){

    string name=js["name"];
    string password=js["password"];


    User user;
    user.setName(name);
    user.setPassword(password);
    bool state=_userModel.insert(user);
    if(state){
        //注册成功
        json response;
        response["msgid"]=REG_MSG_ACK;
        response["errno"]=0;
        response["id"]=user.getId();
        conn->send(response.dump());

    }else{
        //注册失败
        json response;
        response["msgid"]=REG_MSG_ACK;
        response["errno"]=1;
        conn->send(response.dump());

    }
    LOG_INFO<<"do reg service"; 
}

//处理客户端异常退出
void ChatService::clientCloseException(const TcpConnectionPtr &conn){
    lock_guard<mutex> lock(_connmutex);
    User user;
    for(auto it=_userConnmap.begin();it!=_userConnmap.end();++it){
        if(it->second==conn){
            user.setId(it->first);
            //从map表中删除用户的链接信息
            _userConnmap.erase(it);
            break;
        }
    }
        //在redis中取消订阅通道
    _redis.unsubscribe(user.getId());

    //更新用户的状态信息
    if(user.getId()!=-1){
        user.setState("offline");
        _userModel.updatestate(user);
    }
}

//一对一聊天业务
void ChatService::oneChat(const TcpConnectionPtr &conn,json &js,Timestamp time){

    int toid=js["toid"].get<int>();

    {
        lock_guard<mutex> lock(_connmutex);
        auto it =_userConnmap.find(toid);
        if(it!=_userConnmap.end()){
            //toid在线，转发消息，A->B的消息，先发给服务器，服务器再进行中转
            it->second->send(js.dump());
            return; 

        }
    }

    //查询toid是否在线
    User user=_userModel.query(toid);
    if(user.getState()=="online"){
        _redis.publish(toid,js.dump());
        return;
    }

    //toid不在线，存储离线消息
    _offlinemsgmodel.insert(toid,js.dump());

}


void ChatService::reset(){


    //把所用online用户的状态，转成offline
    _userModel.resetState();

}


//添加好友业务
void ChatService::addFriend(const TcpConnectionPtr &conn,json &js,Timestamp time){
    int userid=js["id"].get<int>();
    int friendid=js["friendid"].get<int>();

    //存储好友信息
    _friendmodel.insert(userid,friendid);


}

//创建群组业务
void ChatService::createGroup(const TcpConnectionPtr &conn,json &js,Timestamp time){
    int userid=js["id"].get<int>();
    string name=js["groupname"];
    string desc=js["groupdesc"];

    Group group(-1,name,desc);

    if(_groupmodel.createGroup(group)){
        //存储创建人信息
        _groupmodel.addGroup(userid,group.getId(),"creator");
    }

}

//加入群组业务
void ChatService::addGroup(const TcpConnectionPtr &conn,json &js,Timestamp time){
    int userid=js["id"].get<int>();
    int groupid=js["groupid"].get<int>();
    _groupmodel.addGroup(userid,groupid,"normal");

}

//群聊天业务
void ChatService::groupChat(const TcpConnectionPtr &conn,json &js,Timestamp time){
    int userid=js["id"].get<int>();
    int groupid=js["groupid"].get<int>();

    vector<int> useridVec=_groupmodel.queryGroupUsers(userid,groupid);

    lock_guard<mutex> lock(_connmutex);
    for(int id:useridVec){
        auto it=_userConnmap.find(id);
        if(it!=_userConnmap.end()){
            //转发群消息
            it->second->send(js.dump()); 
        }else{

            //查询toid是否在线
            User user=_userModel.query(id);
            if(user.getState()=="online"){
                _redis.publish(id,js.dump());
            }else{
                //存储离线消息
                _offlinemsgmodel.insert(userid,js.dump());

            }
        }
    }
}

//退出业务
void ChatService::quit(const TcpConnectionPtr &conn,json &js,Timestamp time){
    int userid =js["id"].get<int>();
    {
        lock_guard<mutex> lock(_connmutex);
        auto it=_userConnmap.find(userid);
        if(it!=_userConnmap.end()){
            _userConnmap.erase(it);
        }

    }
    //在redis中取消订阅通道
    _redis.unsubscribe(userid);
    //更新状态信息
    User user(userid,"","","offline");
    _userModel.updatestate(user);

}

//从redis消息队列中获取订阅的消息
void ChatService::handleRedisSubcribeMessage(int channel,string message){
    lock_guard<mutex> lock(_connmutex);
    auto it=_userConnmap.find(channel);
    if(it!=_userConnmap.end()){
        it->second->send(message);
        return;
    }

    //存储该用户的离线消息
    _offlinemsgmodel.insert(channel,message);

}