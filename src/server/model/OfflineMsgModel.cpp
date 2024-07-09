#include"db.hpp"
#include"OfflineMsgModel.hpp"
#include<iostream>

//存储用户的离线消息
void OfflineMsgModel::insert(int userid,string msg){

    //1、组装sql语句
    char sql[1024]={0};
    sprintf(sql,"insert into OfflineMessage(userid,message) values(%d,'%s')",userid,msg);

    
    MySQL mysql;
    if(mysql.connnect()){
        mysql.update(sql);  
    }



}

//删除用户的离线消息
void OfflineMsgModel::remove(int userid){

    char sql[1024]={0};
    sprintf(sql,"delete from OfflineMessage where userid=%d",userid);

    MySQL mysql;
    if(mysql.connnect()){
        mysql.update(sql);
    }

}

//查询用户的离线消息
vector<string> OfflineMsgModel::query(int userid){
    char sql[1024]={0};
    sprintf(sql,"select *from OfflineMessage where userid=%d",userid);

    vector<string> vec;
    MySQL mysql;
    cout<<"offlineMsgModel断点1"<<endl;
    if(mysql.connnect()){
        cout<<"offlineMsgModel断点2"<<endl;
        MYSQL_RES*res=mysql.query(sql);
        if(res!=nullptr){
            //把userid用户所有的离线消息放入vec中返回
            MYSQL_ROW row;
            while((row=mysql_fetch_row(res))!=nullptr){
                vec.push_back(row[0]);
            }
            mysql_free_result(res);

        }
    }
    cout<<"offlineMsgModel断点3"<<endl;
    return vec;

}


void OfflineMsgModel::test(){

}