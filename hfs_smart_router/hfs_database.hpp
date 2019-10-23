#pragma once 

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