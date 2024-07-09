#ifndef USERMODEL_H
#define USERMODEL_H
#include"user.hpp"
//User表的数据操作类
class UserModel{
public:
    //user表的增加方法
    bool insert(User &user);

    User query(int id);

    //更新用户的状态信息
    bool  updatestate(User &user);

    //重置用户的状态
    void resetState();

};

#endif