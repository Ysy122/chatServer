#include"usermodel.hpp"
#include"db.hpp"
#include<iostream>
using namespace std; 
//user表的增加方法
bool UserModel::insert(User &user){

    //1、组装sql语句
    char sql[1024]={0};
    sprintf(sql,"insert into User(name,password,state) values('%s','%s','%s')",user.getName().c_str(),user.getPassword().c_str(),user.getState().c_str());
    MySQL mysql;
    if(mysql.connnect()){
        if(mysql.update(sql)){
            //获取插入成功的用户数据生成的主键
            user.setId(mysql_insert_id(mysql.getConnection()));
            return true;
        }
    }

    return false;
}

//根据主键查询
User UserModel::query(int id){
    char sql[1024]={0};
    sprintf(sql,"select * from User where id= %d",id);

    MySQL mysql;
    User user;
    if(mysql.connnect()){
        MYSQL_RES *res=mysql.query(sql);
        if(res!=nullptr){
            MYSQL_ROW row=mysql_fetch_row(res);  
            if(row!=nullptr){
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setPassword(row[2]);
                user.setState(row[3]);
                

                mysql_free_result(res);
                return user; 
            }
        }
    }
    return user;
}


//更新操作
bool  UserModel::updatestate(User &user){


    cout<<"断点1"<<endl;
    //1、组装sql语句
    char sql[1024]={0};
    sprintf(sql,"update User set state='%s' where id=%d",user.getState().c_str(),user.getId());

    MySQL mysql;
    cout<<"断点2"<<endl;
    mysql.connnect();
    // if(mysql.connnect()){
    //     if(mysql.update(sql)){
    //         return true;
    //     }
    // }

    cout<<"断点3"<<endl;
    return false;

}

//重置用户的状态
void UserModel::resetState(){
    

    char sql[1024]="update User set state='offline' where state='online'";

    MySQL mysql;
    if(mysql.connnect()){
        mysql.update(sql);
    }
    
}
