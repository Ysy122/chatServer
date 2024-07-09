#include"db.hpp"

//数据库配置信息
static string m_server="127.0.0.1";
static string m_user="root";
static string m_password="123456";
static string m_dbname="chat";

MySQL::MySQL(){
     _conn=mysql_init(nullptr);
}

MySQL::~MySQL(){
    if(_conn!=nullptr){
        mysql_close(_conn);
    }
}

bool MySQL::connnect(){

    cout<<"准备数据库连接"<<endl;
    cout<<_conn<<" "<<m_server<<" "<<m_user<<" "<<m_password<<" "<<m_dbname<<endl;
    MYSQL *p=mysql_real_connect(_conn,m_server.c_str(),m_user.c_str(),
                                    m_password.c_str(),m_dbname.c_str(),3306,nullptr,0);
    cout<<"数据库连接"<<p<<endl;

        
    if(p!=nullptr){
            //C和C++代码默认的编码字符是ASCII，如果不设置，从Mysql上拉下来的中文显示会乱码
        mysql_query(_conn,"set names gbk");
        LOG_INFO<<"connect mysql success";
    }else {
        LOG_INFO<<"connect mysql fail";
    }

    return p;
}

//更新操作
bool MySQL::update(string sql){
    if(mysql_query(_conn, sql.c_str())){
        LOG_INFO<<__FILE__<<":"<<__LINE__<<":"
            <<sql<<"更新失败";
        return false;
    }
    return true;
}


//查询操作
MYSQL_RES * MySQL::query(string sql){

    if(mysql_query(_conn,sql.c_str())){
        LOG_INFO<<__FILE__<<":"<<__LINE__<<":"
            <<sql<<"查询失败";
        return nullptr;
    }

    return mysql_use_result(_conn);
}


//获取连接
MYSQL* MySQL::getConnection(){
    return _conn;
}

