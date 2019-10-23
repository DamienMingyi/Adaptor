#pragma once 

#include <unordered_map>
#include <map>
#include <iostream>
#include <vector>
#include "ChildOrdMgr.hpp"
#include "ParentOrdMgr.hpp"
#include "hfs_base_router.hpp"
#include "HfsCommunicator.hpp"
#include "UnfitPosKey.hpp"
#include "TraderFundKey.hpp"
#include "hfs_database.hpp"

using namespace std;

typedef unordered_map<string, int> Pos;//position for one account ,which the key is tkr 
typedef unordered_map<string, Pos> ExchPos;//position for each account , which the key is the exch
typedef map<UnfitPosKey, int> UnfitPos;//unfitted position for each account 
typedef map<TraderFundKey, float> TraderFund;//unfitted position for each account 

class TradePlan
{
public:
    TradePlan(hfs_adaptor_mgr &_adMgr,Communicator &_comm,a2ftool::XmlNode _cfg); 
    bool OnReceiveIns(int teid, hfs_order_t& order);
    bool OnReceiveCancel(int teid, hfs_order_t& order);
    bool OnReceiveNewResp(int teid, std::string execid, hfs_order_t& order);
    bool OnReceiveCancelResp(int teid, std::string execid, hfs_order_t& order);
    bool OnReceiveFillResp(int teid, std::string execid, hfs_order_t& order);
    bool OnReceiveFailResp(int teid, std::string execid, hfs_order_t& order);  

    int GetCurPos(string tkr); 
    int GetCurPos(string tkr,string exch);
    // void reload(string exch="");
    // void rejectpn(string exch="");
    // void cancelnew(string exch="");
    void LoadAlgoPos(string exch="");
    void LoadT0Pos(string exch="");
    void RemoveT0Pos(string exch="");
    void RejectPendingNew(string exch="");
    void CancelNew(string exch="");
    void SetAllOrdFinished();
private:
    void Init(hfs_adaptor_mgr & _adMgr,Communicator & _comm);//load from DB

    ExchPos exchPosInit;
    ExchPos exchPos;
    ExchPos algoPosB;
    ExchPos algoPosS; 
    UnfitPos unfitPos;
    TraderFund traderFund;
    vector<string> exchList;
    map<string,float> exchBal;
    ParentOrdMgr *parentOrdMgr;
    unordered_map<string, ChildOrdMgr *> childOrdMap;
    a2ftool::XmlNode &cfg;
    bool selfBarginCtl;
    vector<string>::iterator exchIt0;
    vector<string>::iterator exchIt1;

    hfs_database* db ;
    string tag;
};