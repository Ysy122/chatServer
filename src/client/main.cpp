#include"json.hpp"
#include<iostream>
#include<thread>
#include<string>
#include<vector>
#include<chrono>
#include<ctime>

using namespace std;
using json=nlohmann::json;

#include<unistd.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<semaphore.h>
#include<atomic> 

#include"group.hpp"
#include"user.hpp"
#include"public.hpp"

//记录当前系统登录的用户信息
User g_currentUser;

//记录当前登录用户的好友列表信息
vector<User> g_currentUserFriendList;

//记录当前登录用户的群组列表信息
vector<Group> g_currentUserGroupList;

//读写线程之间通信
sem_t rwsem;
//记录登录状态
atomic_bool g_loginState{false};

//控制主聊天页面
bool isMainMenuRunning=false;

//显示当前登录成功用户的基本信息
void showCurrentUserData();

//接收线程
void readTaskHandler(int clientfd);

//获取系统时间
string getCurrentTime();

//主聊天页面程序
void mainMenu(int clientfd);


//聊天客户端程序实现，main线程用作发送线程，子线程用作接收线程

int main(int argc,char **argv){

    if(argc<3){
        cerr<<"command invalid! example: ./ChatClient 127.0.0.1 6000"<<endl;
        exit(-1);
    }

    //解析通过命令行参数传递的ip和port
    char *ip=argv[1];
    uint16_t port=atoi(argv[2]);

    //创建client端的socket
    int clientfd=socket(AF_INET,SOCK_STREAM,0);
    if(-1==clientfd){
        cerr<<"socket create error"<<endl;
        close(clientfd);
        exit(-1);
    }

    //填写client需要连接的server信息ip+port
    sockaddr_in server;
    memset(&server,0,sizeof(sockaddr_in));

    server.sin_family=AF_INET;
    server.sin_port=htons(port);
    server.sin_addr.s_addr=inet_addr(ip);

    //client和server进行拼接
    int num=connect(clientfd,(sockaddr *)&server,sizeof(sockaddr_in));
    cout<<"连接编号"<<num<<endl;
    if(num==-1){
        cerr<<"connect server error"<<endl;
        close(clientfd);
        exit(-1);
    }


    sem_init(&rwsem,0,0);
    std::thread readTask(readTaskHandler,clientfd);
    readTask.detach();

    //main线程用于接收用户输入，负责发送数据
    for(;;){
        //显示首页面菜单，登录、注册、退出
        cout<<"==================="<<endl;
        cout<<"1. login"<<endl;
        cout<<"2. register"<<endl;
        cout<<"3. quit"<<endl;
        cout<<"==================="<<endl;
        cout<<"choice:";
        int choice=0;
        cin>>choice;
        cin.get();//读掉缓冲区残留的回车

        switch(choice){
        case 1:
        {
            int id=0;
            char pwd[50]={0};
            cout<<"userid:";
            cin>>id;
            cin.get();
            cout<<"userpassword:";
            cin.getline(pwd,50);

            json js;
            js["msgid"]=LOGIN_MSG;
            js["id"]=id;
            js["password"]=pwd;
            string request=js.dump();


            g_loginState=false;

            int len=send(clientfd,request.c_str(),strlen(request.c_str())+1,0);

            if(len==-1){
                cerr<<"send login msg error:"<<request<<endl;
            }

            sem_wait(&rwsem);//等待信号量

            if(g_loginState){
                //进入聊天主菜单页面
                isMainMenuRunning=true;
                mainMenu(clientfd);
            }


        }
        break;
        case 2://注册业务
        {
            char name[50]={0};
            char pwd[50]={0};

            cout<<"username:";
            cin.getline(name,50);
            cout<<"userpassword:";
            cin.getline(pwd,50);


            json js;
            js["msgid"]=REG_MSG;
            js["name"]=name;
            js["password"]=pwd;
            string request=js.dump();

            g_loginState=false;

            int len=send(clientfd,request.c_str(),strlen(request.c_str())+1,0);
            if(len==-1){
                cerr<<"send login msg error"<<request<<endl;

            }

            sem_wait(&rwsem);

        }
        break;
        case 3:
            close(clientfd);
            sem_destroy(&rwsem);
            exit(0);
        default:
            cerr<<"invalid input"<<endl;
            break;


        }
    }

    return 0;

}


//显示当前登录成功用户的基本信息
void showCurrentUserData(){
    cout<<"======================Login User======================="<<endl;
    cout<<"current login user=>id:"<<g_currentUser.getId()<<"name:"<<g_currentUser.getName()<<endl;
    cout<<"----------------------Friend List----------------------"<<endl;

    if(!g_currentUserFriendList.empty()){
        for(User &user :g_currentUserFriendList){
            cout<<user.getId()<<" "<<user.getName()<<" "<<user.getState()<<endl;
        }
    }
    cout<<"----------------------Group list-----------------------"<<endl;
    if(!g_currentUserGroupList.empty()){
        for(Group &group :g_currentUserGroupList){
            cout<<group.getId()<<" "<<group.getName()<<" "<<group.getDesc()<<endl;
            for(GroupUser &user :group.getUsers()){
                cout<<user.getId()<<" "<<user.getName()<<" "<<user.getState()
                <<" "<<user.getRole()<<endl;
            }
        }
    }
    cout<<"=========================================="<<endl;

}

void handlerLoginResponse(json &responsejs){

    if(0!=responsejs["errno"].get<int>())//登录失败
    {
        cerr<<responsejs["errmsg"]<<endl;
        g_loginState=false;
    }//登录成功
    else{
        cout<<"2222222"<<endl;
        //记录当前用户的好友列表信息
        if(responsejs.contains("friends")){
            g_currentUserFriendList.clear();
            vector<string> vec=responsejs["friends"];
            for(string &str:vec){
                json js=json::parse(str);
                User user;
                user.setId(js["friendid"].get<int>());
                user.setName(js["friendname"]);
                user.setState(js["friendstate"]);
                g_currentUserFriendList.push_back(user);
            }
        }
                        
        cout<<"33333333"<<endl;
        //记录当前用户的群组列表信息
        if(responsejs.contains("groups")){
            g_currentUserGroupList.clear();
            vector<string> vec1=responsejs["groups"];
            for(string &groupstr:vec1){
                json groupjs=json::parse(groupstr);
                Group group;
                group.setId(groupjs["id"].get<int>());
                group.setName(groupjs["groupname"]);
                group.setDesc(groupjs["groupdesc"]);

                vector<string> vec2=groupjs["users"];
                for(string &userstr:vec2){
                    GroupUser user;
                    json js=json::parse(userstr);
                    user.setId(js["id"].get<int>());
                    user.setName(js["name"]);
                    user.setState(js["state"]);
                    user.setRole(js["role"]);
                    group.getUsers().push_back(user);
                }
                g_currentUserGroupList.push_back(group);
            }

        }

        cout<<"44444444"<<endl;
        //显示登录用户的基本信息
        showCurrentUserData();

        //显示当前用户的离线消息，个人聊天信息或者群组消息
        if(responsejs.contains("offlinemsg")){
            vector<string> vec=responsejs["offlinemsg"];
            for(string &str:vec){
                json js=json::parse(str);
                if(ONE_CHAT_MSG==js["msgid"].get<int>()){
                    cout<<js["time"].get<string>()<<"["<<js["id"]<<"]"<<js["name"].get<string>()
                            <<"said: "<<js["msg"].get<string>()<<endl;
                    continue;
                }
                else{
                    cout<<"群消息["<<js["groupid"]<<"]: "<<js["time"].get<string>()<<"["<<js["id"]<<"]"<<js["name"].get<string>()
                    <<"said: "<<js["msg"].get<string>()<<endl;
                    continue;
                }
            }
        }

        g_loginState=true;

    }

}

void handleRegisterResponse(json &responsejs){

    if(0!=responsejs["errno"].get<int>()){
        cerr<<"is already exist,register error!"<<endl;
    }else{
        cout<<"resgister success,userId is"<<responsejs["id"]
        <<",do not forget it!"<<endl;
    }

}

//接收线程
void readTaskHandler(int clientfd){

    for(;;){
        char buffer[1024]={0};
        int len=recv(clientfd,buffer,1024,0); //阻塞了
        if(len==-1||len==0){
            close(clientfd);
            exit(-1);
        }

        json js=json::parse(buffer);
        int msgType=js["msgid"].get<int>();
        if(ONE_CHAT_MSG==msgType){
            cout<<js["time"].get<string>()<<"["<<js["id"]<<"]"<<js["name"].get<string>()
            <<"said: "<<js["msg"].get<string>()<<endl;
            continue;
        }
        if(GROUP_CHAT_MSG==msgType){
            cout<<"群消息["<<js["groupid"]<<"]: "<<js["time"].get<string>()<<"["<<js["id"]<<"]"<<js["name"].get<string>()
            <<"said: "<<js["msg"].get<string>()<<endl;
            continue;
        }

        if(LOGIN_MSG_ACK==msgType){
            handlerLoginResponse(js);
            sem_post(&rwsem); 
            continue;

        }

        if(REG_MSG_ACK==msgType){
            handleRegisterResponse(js);
            sem_post(&rwsem);
            continue;
        }
    }

}
//获取系统时间
string getCurrentTime(){
    return "";
}

//系统支持的客户端命令列表
unordered_map<string,string> commandMap={
    {"help","显示所有支持的命令，格式help"},
    {"chat","一对一聊天，格式chat:friend:message"},
    {"addfriend","添加好友,格式addfriend:friendid"},
    {"creategroup","创建群组，格式creategroup:groupname:groupdesc"},
    {"addgroup","加入群组，格式addgroup:groupid"},
    {"groupchat","群聊,格式groupchat:groupid:message"},
    {"quit","注销，格式quit"}
};

void help(int fd=0,string str="");

void chat(int,string);

void addfriend(int,string);

void creategroup(int,string);

void addgroup(int,string);

void groupchat(int,string);

void quit(int,string);
//注册系统支持的客户端命令处理
unordered_map<string,function<void(int,string)>> commandHandlerMap={
    {"help",help},
    {"chat",chat},
    {"addfriend",addfriend},
    {"creategroup",creategroup},
    {"addgroup",addgroup},
    {"groupchat",groupchat},
    {"quit",quit}
};



//主聊天页面程序
void mainMenu(int clientfd){
    help();

    char buffer[1024]={0};
    while(isMainMenuRunning){
        cin.getline(buffer,1024);
        string commandbuf(buffer);
        string command;//存储命令

        int idx=commandbuf.find(":");
        if(-1==idx){
            command=commandbuf;
        }else{
            command=commandbuf.substr(0,idx);
        }
        auto it=commandHandlerMap.find(command);
        if(it==commandHandlerMap.end()){
            cerr<<"invalid input command!"<<endl;
            continue;
        }

        //调用相应的事件处理回调，mainMenu对修改封闭，添加新功能不需要修改该函数
        it->second(clientfd,commandbuf.substr(idx+1,commandbuf.size()-idx));


    }

}

//帮助
void help(int,string){
    cout<<"show command list>>>>"<<endl;
    for(auto &p:commandMap){
        cout<<p.first<<": "<<p.second<<endl;
    }
    cout<<endl;
}

//一对一聊天
void chat(int clientfd,string str) {
    int idx=str.find(":");
    if(idx==-1){
        cerr<<"chat command invalid"<<endl;
        return;
    }

    int friendid=atoi(str.substr(0,idx).c_str());
    string message=str.substr(idx+1,str.size()-idx);

    json js;
    js["msgid"]=ONE_CHAT_MSG;
    js["id"]=g_currentUser.getId();
    js["name"]=g_currentUser.getName();
    js["toid"]=friendid;
    js["msg"]=message;
    js["time"]=getCurrentTime();

    string buffer=js.dump();

    int len=send(clientfd,buffer.c_str(),strlen(buffer.c_str())+1,0);
    if(len==-1){
        cerr<<"send chat msg error"<<buffer<<endl;
    }

}

//添加好友
void addfriend(int clientfd,string str){
    int  friendid=atoi(str.c_str());
    json js;
    js["msgid"]=ADD_FRIEND_MSG;
    js["id"]=g_currentUser.getId();
    js["friendid"]=friendid;
    string buffer=js.dump();

    int len=send(clientfd,buffer.c_str(),strlen(buffer.c_str())+1,0);
    if(len==-1){
        cerr<<"send addfriend msg error"<<buffer<<endl;
    }
}

//创建群组
void creategroup(int clientfd,string str){
    int idx=str.find(":");
    if(idx==-1){
        cerr<<"creategroup command invaild"<<endl;
        return;
    }

    string groupname=str.substr(0,idx);
    string groupdesc=str.substr(idx+1,str.size()-idx);

    json js;
    js["msgid"]=CREATE_GROUP_MSG;
    js["id"]=g_currentUser.getId();
    js["groupname"]=groupname;
    js["groupdesc"]=groupdesc;

    string buffer=js.dump();

    int len=send(clientfd,buffer.c_str(),strlen(buffer.c_str())+1,0);
    if(len==-1){
        cerr<<"send create group msg error"<<endl;
    }

}

//添加群组
void addgroup(int clientfd,string str){
    int groupid=atoi(str.c_str());

    json js;
    js["msgid"]=ADD_GROUP_MSG;
    js["id"]=g_currentUser.getId();
    js["groupid"]=groupid;

    string buffer=js.dump();
    int len=send(clientfd,buffer.c_str(),strlen(buffer.c_str())+1,0);
    if(len==-1){
        cerr<<"send add group msg error"<<endl;
    }

}

//群组聊天
void groupchat(int clientfd,string str){
    int idx=str.find(":");

    if(idx==-1){
        cerr<<"groupchat command error"<<endl;
        return;
    }

    int groupid=atoi(str.substr(0,idx).c_str());
    string message=str.substr(idx+1,str.size()-idx);

    json js;
    js["msgid"]=GROUP_CHAT_MSG;
    js["id"]=g_currentUser.getId();
    js["name"]=g_currentUser.getName();
    js["groupid"]=groupid;
    js["msg"]=message;
    js["time"]=getCurrentTime();

    string buffer=js.dump();
    int len=send(clientfd,buffer.c_str(),strlen(buffer.c_str())+1,0);
    if(len==-1){
        cerr<<"send group chat msg error"<<endl;
    }

}

//退出消息
void quit(int clientfd,string str){
    json js;
    js["msgid"]=LOGINOUT_MSG;
    js["id"]=g_currentUser.getId();
    string buffer=js.dump();
    int len=send(clientfd,buffer.c_str(),strlen(buffer.c_str())+1,0);
    if(len==-1){
        cerr<<"send quit msg error"<<endl;
    }else{
        isMainMenuRunning=false;

    }

}

