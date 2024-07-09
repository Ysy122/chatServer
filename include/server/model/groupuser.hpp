#ifndef GROUPUSER_H
#define GROUPUSER_H
#include<string>
#include"user.hpp"
using namespace std;

//群组用户，多一个role角色信息，从User类直接继承，复用User的其他信息
class GroupUser: public User
{
private:
    string role;
public:
    void setRole(string role){this->role=role;}
    string getRole(){return this->role;}

};


#endif