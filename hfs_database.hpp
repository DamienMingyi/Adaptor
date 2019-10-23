// #pragma once 

// #include "hfs_log.hpp"
// #include <mutex>
// /// 数据库操作

// #include "mysql/mysql.h"

// class hfs_database_mysql {
// public:
//     hfs_database_mysql(const char*user, const char* passwd, const char* server, const char *dataBase="prodrw", int port=3306) {
//         m_conn = mysql_init(nullptr);
//         unsigned int timeout = 60;
//         mysql_options(m_conn, MYSQL_OPT_CONNECT_TIMEOUT, (const char *)&timeout);
//         if(!mysql_real_connect(m_conn,server,user,passwd,dataBase,0,NULL,0)) {
//             LOG_ERROR("mysql database conncet failed:{}", mysql_error(m_conn));
//         } else {
//             // if(!(m_stmt=mysql_stmt_init(m_conn))) {
//             //     LOG_ERROR("mysql database init statement failed:{}", mysql_stmt_error(m_stmt));
//             // }
//         }

//     };

//     virtual ~hfs_database_mysql() {
//         mysql_stmt_close(m_stmt);
//         mysql_close(m_conn);
//     };
//     void select_db(const char *dataBase) {
//         mysql_select_db(m_conn, dataBase);
//     }
//     void start(){
//         m_mutex.lock();

//     };

//     MYSQL_RES* query(const char* sql) {
//         int t = mysql_query(m_conn,sql);
//         if (t) {
//             LOG_ERROR("mysql database query error:{}", mysql_error(m_conn));
//             return nullptr;
//         }
//         m_rs = mysql_use_result(m_conn);
//         return m_rs;
//     };
//     MYSQL_ROW next() {
//         m_row = mysql_fetch_row(m_rs);
//         return m_row;
//     }

//     int insert(const char* sql) {
//         return 0;
//     }

//     // int commit() {
//         // return m_conn->Commit();
//     // }
//     // int rollback() {
//         // return m_conn->rollback;
//     // }
    
//     void end() {
//         mysql_free_result(m_rs); 
//         m_rs = nullptr;
//         m_row = nullptr;
//         m_mutex.unlock();
//     }
// private:
//     // Environment *m_env = nullptr;
//     MYSQL *m_conn = nullptr;
//     MYSQL_STMT *m_stmt = nullptr;
//     MYSQL_RES *m_rs = nullptr;
//     MYSQL_ROW m_row = nullptr;
//     MYSQL_FIELD *mysql_field = nullptr;
//     std::mutex m_mutex;
// };

// #pragma once 

#include "hfs_log.hpp"
#include <occi.h>
#include <mutex>

using namespace oracle::occi;
/// 数据库操作

class hfs_database {
public:
    hfs_database(const char*user, const char* passwd, const char *server) {
        m_env=Environment::createEnvironment();
        if (m_env == nullptr) LOG_ERROR("oracle environment create failed : {}", server);
        m_conn = m_env->createConnection(user, passwd, server);
        if (!m_conn) LOG_ERROR("oracle connect failed : {}", user);
        
    };

    virtual ~hfs_database() {
        m_env->terminateConnection(m_conn);
        Environment::terminateEnvironment(m_env);
    };

    void start(){
        m_mutex.lock();
        m_stmt = m_conn->createStatement();
        if (m_stmt == nullptr) {
            LOG_ERROR("oracle createstatement failed");
            //m_mutex.unlock();
        }
    };

    ResultSet* query(const char* sql) {
        if (m_stmt == nullptr) return nullptr;
        try{
            m_rs = m_stmt->executeQuery(sql);
            return m_rs;
        }catch(SQLException ex) {
            LOG_ERROR("Exception thrown for query : {}, Errorid : {}, Errormsg : {}", sql, ex.getErrorCode(), ex.getMessage());
            return nullptr;
        }
    };

    int insert(const char* sql) {
        if (m_stmt == nullptr) return -1;
        try{
        m_stmt->setSQL(sql);
        return m_stmt->executeUpdate();
        }catch(SQLException ex){
        LOG_ERROR("Exception thrown for query : {}, Errorid : {}, Errormsg : {}", sql, ex.getErrorCode(), ex.getMessage());
        return -1; 
        }
    }

    // int commit() {
        // return m_conn->Commit();
    // }
    // int rollback() {
        // return m_conn->rollback;
    // }
    
    void end() {
        m_stmt->closeResultSet(m_rs);
        m_rs = nullptr;
        m_mutex.unlock();
    }
private:
    Environment *m_env = nullptr;
    Connection *m_conn = nullptr;
    Statement *m_stmt = nullptr;
    ResultSet *m_rs = nullptr;
    std::mutex m_mutex;
};