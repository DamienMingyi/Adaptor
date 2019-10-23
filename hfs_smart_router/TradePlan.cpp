#include "TradePlan.hpp"

#include <math.h>
#include <chrono> 

TradePlan::TradePlan(hfs_adaptor_mgr & _adMgr,Communicator & _comm,a2ftool::XmlNode _cfg)
:cfg(_cfg)
{
    Init(_adMgr,_comm);
    parentOrdMgr = new ParentOrdMgr(_adMgr,_comm,selfBarginCtl);
}

void TradePlan::Init(hfs_adaptor_mgr & _adMgr,Communicator & _comm)
{
    string db_type = cfg.getAttrDefault("db_type", "orcl");
    string db_addr = cfg.getAttrDefault("db_addr", "192.168.1.16:1521/orcl");
    string db_user = cfg.getAttrDefault("db_user", "prodrw");
    string db_pass = cfg.getAttrDefault("db_pass", "rwprod");
    string redis_addr = cfg.getAttrDefault("redis_addr", "192.168.1.16");
    int redis_port = cfg.getAttrDefault("redis_port", 6379);
    Ord::m_redisMgr = new redisMgr(redis_addr.c_str(),redis_port); 
    Ord::m_redisKey = cfg.getAttrDefault("redis_key", "T0TRADE_TEST_ORD_ZF"); 
    tag = cfg.getAttrDefault("tag", "");
    selfBarginCtl = cfg.getAttrDefault("selfBarginCtl", false);
    
    if(db_type== "orcl")
    {
        db = new hfs_database(db_user.c_str(), db_pass.c_str(), db_addr.c_str());
    }
    else 
    {
        db = new hfs_database(db_user.c_str(), db_pass.c_str(), db_addr.c_str());
        //db = new hfs_database_mysql(db_user.c_str(), db_pass.c_str(), db_addr.c_str());
    }
    db->start();
    //init exchList
    ChildOrdMgr * childOrdMgr =  new ChildOrdMgr(_adMgr,_comm);
    //string sql0 = "select DISTINCT ACCOUNTID  from T0HOLDINGS where  DTE=(SELECT to_char(sysdate,'yyyymmdd') FROM dual) ";
    string sql0 = "select accountid,available from ACCOUNTS where DTE=(select max(DTE) from ACCOUNTS ) and LVL=0";
    string sql1 = "select accountid,available from ACCOUNTS where DTE=(select max(DTE) from ACCOUNTS ) and LVL=1";
    string sql2 = "select accountid,available from ACCOUNTS where DTE=(select max(DTE) from ACCOUNTS ) and LVL=2";

    ResultSet* rs = db->query(sql0.c_str());
    int count0 = 0;
    int count1 = 0;
    while (rs && rs->next()){
       string exch =   rs->getString(1);
       float av = rs->getNumber(2);
       exchList.push_back(exch);
       exchBal[exch] = av;
       exchPos.insert(pair<string, Pos>(exch, Pos()));
       childOrdMap.insert(pair<string,ChildOrdMgr *>(exch,childOrdMgr));
       count0++;
       LOG_INFO("[load holdings 0]:{} {}",exch,av);
    }

    rs = db->query(sql1.c_str());
    while (rs && rs->next()){
       string exch =   rs->getString(1);
       float av = rs->getNumber(2);
       exchList.push_back(exch);
       exchBal[exch] = av;
       exchPos.insert(pair<string, Pos>(exch, Pos()));
       childOrdMap.insert(pair<string,ChildOrdMgr *>(exch,childOrdMgr));
       count1++;
       LOG_INFO("[load holdings 1]:{} {}",exch,av);
    }

    rs = db->query(sql2.c_str());
    while (rs && rs->next()){
       string exch = rs->getString(1);
       float av = rs->getNumber(2);
       exchList.push_back(exch);
       exchBal[exch] = av;
       exchPos.insert(pair<string, Pos>(exch, Pos()));
       childOrdMap.insert(pair<string,ChildOrdMgr *>(exch,childOrdMgr));
       LOG_INFO("[load holdings 2]:{} {}",exch,av);
    }

    exchIt0 = exchList.begin() + count0;
    exchIt1 = exchList.begin() + count0 + count1;

    // //init t0 holdings
    // string sql = "select ACCOUNTID,TKR,QTY from T0HOLDINGS where DTE=(SELECT to_char(sysdate,'yyyymmdd') FROM dual) ";
    // if (tag != "")
    // { 
    //     std::stringstream ss;
    //     ss << sql << " and TAG='"<<tag<<"'";
    //     sql = ss.str();
    //     //LOG_INFO("SQL:{}",sql);
    // }
    // rs = db->query(sql.c_str());

    // while (rs && rs->next()){
    //     string ACCOUNTID =  rs->getString(1);
    //     string TKR =  rs->getString(2);
    //     int QTY = rs->getInt(3);
    //     exchPos[ACCOUNTID][TKR] = QTY;
    //     exchPosInit[ACCOUNTID][TKR] = QTY;
    // }
    LoadT0Pos();
    // init fund
    rs = db->query("select traderid,max_exposure,teid from Trader ");
    while (rs && rs->next()){
        int traderid = rs->getInt(1);   
        int max_exposure = rs->getInt(2);  
        int teid = rs->getInt(3);  
        traderFund.insert(pair<TraderFundKey,int >(TraderFundKey(teid,traderid),max_exposure));//make teid =1 
    }

    LoadAlgoPos();
    //db->end();
    
}

bool TradePlan::OnReceiveIns(int teid, hfs_order_t& order) 
{
    Ord* parentOrd = parentOrdMgr->NewIns(teid,order);
    if (parentOrd ==nullptr)
    {
        return false;
    }

    
    float fozFund = order.prc * order.qty;
    

    if(teid < Communicator::MaxTECount/2 )
    {
        if (order.side == 'B')
        {
            float availFund = traderFund[TraderFundKey(teid,order.oseq)];
            
            if (availFund < fozFund)
            {
                LOG_INFO("[OPEN:{}  unenough fund ins]availFund:{};fozFund:{}",order.symbol,availFund,fozFund);
                parentOrdMgr->SetFundNotEnoughResp(teid,parentOrd,order,availFund);
                return false;
            }
            else 
            {
                traderFund[TraderFundKey(teid,order.oseq)] -= fozFund;
                LOG_INFO("[After Buy {} ] PID:{}, currend available fund:{}",order.symbol,order.pid,traderFund[TraderFundKey(teid,order.oseq)]);
            }
        }

        int leftHand = order.qty / 100;


        for (string exch : exchList ) 
        {
            UnfitPosKey uft(teid, order.oseq, order.symbol,order.side,exch); 
            uft.SwitchSide();
            auto iter = unfitPos.find(uft);
            if (iter != unfitPos.end() && iter->second >= 100)
            {
                //if exist
                int unfitHand = iter->second / 100; 
                LOG_INFO("[{} unfitHand found ]exch:{},unfitHand:{}",order.symbol,exch,unfitHand);
                if (leftHand < unfitHand)
                {
                    // it is part close ins
                    iter->second -= leftHand * 100;
                    
                    //send exch,leftHand
                    LOG_INFO("[CLOSE ins:{} part close ]exch:{},leftHand:{},unfitHand:{}",order.symbol,exch,leftHand,unfitHand);
                    childOrdMap[exch]->NewIns(teid,exch,leftHand * 100,parentOrd,order,'C');
                    leftHand = 0;
                    return true;

                }
                else if (leftHand == unfitHand)
                {
                    // it is exctaly close ins
                    LOG_INFO("[CLOSE ins:{} exctaly close ]exch:{},leftHand:{},unfitHand:{}",order.symbol,exch,leftHand,unfitHand);
                    childOrdMap[exch]->NewIns(teid,exch,leftHand * 100,parentOrd,order,'C');//send exch,leftHand
                    unfitPos.erase (iter);
                    leftHand = 0;
                    
                    return true;
                }
                else
                {
                    // part close ,part open  
                    LOG_INFO("[CLOSE ins:{} total close ,other continue ]exch:{},unfitHand:{},leftHand:{}",order.symbol,exch,unfitHand,leftHand);
                    childOrdMap[exch]->NewIns(teid,exch,unfitHand * 100,parentOrd,order,'C'); //send exch,unfitHand
                    leftHand -= unfitHand;
                    unfitPos.erase (iter);
                }
            }
        }

        unsigned seed = std::chrono::system_clock::now ().time_since_epoch ().count ();
        shuffle (exchList.begin (), exchIt0,default_random_engine(seed)) ;
        shuffle (exchIt0, exchIt1,default_random_engine(seed)) ;
        shuffle (exchIt1, exchList.end (),default_random_engine(seed)) ;

        int availableHand = 0;
        for ( string exch : exchList ) 
        {
            availableHand += exchPos[exch][order.symbol]/100 ;
            if (exchPos[exch][order.symbol]>0)
            {
                LOG_INFO("[{} current available amount]exch:{},available:{},balance:{}",order.symbol,exch,exchPos[exch][order.symbol],exchBal[exch] );
            }
        }


        for (string exch : exchList ) 
        {
            int hand =  exchPos[exch][order.symbol]/100  ;
        
            if (hand <= 0 )
                continue;
            if (hand >= leftHand)//enough
            {
                if ( exchBal[exch] < leftHand * 100 * order.prc && order.side == 'B')
                {
                    continue;
                }
                if (childOrdMap[exch]->NewIns(teid,exch,leftHand * 100,parentOrd,order,'O'))//send exch,leftHand
                {
                    if (order.side == 'B')
                    {
                        exchBal[exch] -= leftHand * 100 * order.prc;
                    }
                    exchPos[exch][order.symbol] -= leftHand *100;
                    UnfitPosKey uft(teid, order.oseq, order.symbol,order.side,exch); 
                    unfitPos[uft] += leftHand *100;
                    LOG_INFO("[OPEN:{} enough available amount ins],exch:{},leftHand:{},exchBal:{}",order.symbol,exch,leftHand,exchBal[exch]); 
                    leftHand =0;
                    break;
                }
            }
            else 
            {
                if ( exchBal[exch] < hand * 100 * order.prc && order.side == 'B')
                {
                    continue;
                }
                if( childOrdMap[exch]->NewIns(teid,exch,hand * 100,parentOrd,order,'O'))//send exch,hand
                {
                    if (order.side == 'B')
                    {
                        exchBal[exch] -= hand * 100 * order.prc;
                    }
                    LOG_INFO("[OPEN:{} unenough available amount ins]exch:{},hand:{}",order.symbol,exch,hand,exchBal[exch]);
                    leftHand -= hand;
                    exchPos[exch][order.symbol] = 0;
                    UnfitPosKey uft(teid, order.oseq, order.symbol,order.side,exch); 
                    unfitPos[uft] += hand *100;
                }

            }
        }
        if (leftHand >0)
        {
            LOG_INFO("[OPEN:{} final unenough available amount ins]hand:{}",order.symbol,leftHand);
            parentOrdMgr->SetNotEnoughResp(teid,parentOrd,order,leftHand * 100);
            if (order.side == 'B')
            {
                traderFund[TraderFundKey(teid,order.oseq)] += leftHand * 100 * order.prc;
                LOG_INFO("[After Buy Fail {} ] PID:{}, currend available fund:{}",order.symbol,order.pid,traderFund[TraderFundKey(teid,order.oseq)]);
            }
            
        }
    }
    else // for algo 
    {
        unsigned seed = std::chrono::system_clock::now ().time_since_epoch ().count ();
        shuffle (exchList.begin (), exchIt0,default_random_engine(seed)) ;
        shuffle (exchIt0, exchIt1,default_random_engine(seed)) ;
        shuffle (exchIt1, exchList.end (),default_random_engine(seed)) ;

        int leftQty = order.qty;
        for (string exch : exchList ) 
        {
            if (order.side == 'B' && algoPosB[exch][order.symbol] > 0 )
            {
                LOG_INFO("[{} algoPosB available amount]exch:{},available:{},balance:{}",order.symbol,exch,algoPosB[exch][order.symbol],exchBal[exch] );
                int buyQty =  leftQty / 100 * 100;
                if (buyQty<=1)
                    continue;
                if (algoPosB[exch][order.symbol]  >= buyQty )
                {
                    if (exchBal[exch] >= leftQty * order.prc && childOrdMap[exch]->NewIns(teid,exch,buyQty ,parentOrd,order,'A'))
                    {
                        algoPosB[exch][order.symbol] -= buyQty;
                        exchBal[exch] -= buyQty * order.prc;
                        LOG_INFO("[Algo Buy:{} enough available amount ins],exch:{},leftQty:{} ,exchBal:{}.Finished",order.symbol,exch,buyQty,exchBal[exch]); 
                        leftQty = 0;
                        break;
                    }
                }
                else
                {
                    if (exchBal[exch] >= algoPosB[exch][order.symbol] * order.prc && childOrdMap[exch]->NewIns(teid,exch,algoPosB[exch][order.symbol],parentOrd,order,'A'))
                    {
                        leftQty -= algoPosB[exch][order.symbol]; 
                        exchBal[exch] -= algoPosB[exch][order.symbol] * order.prc;
                        LOG_INFO("[Algo Buy:{} unenough available amount ins],exch:{},hand:{} ,exchBal:{}",order.symbol,exch,algoPosB[exch][order.symbol],exchBal[exch]); 
                        algoPosB[exch][order.symbol] = 0;
                    }
                }
                
            }
            else if (order.side == 'S' && algoPosS[exch][order.symbol] > 0 )
            {
                LOG_INFO("[{} algoPosS available amount]exch:{},available:{},balance:{}",order.symbol,exch,algoPosS[exch][order.symbol],exchBal[exch] );
                int exchQty = algoPosS[exch][order.symbol] ;
                if (leftQty <= exchQty )
                {
                    if (childOrdMap[exch]->NewIns(teid,exch,leftQty ,parentOrd,order,'A'))
                    {
                        algoPosS[exch][order.symbol] -= leftQty;
                        exchBal[exch] += leftQty * order.prc;
                        LOG_INFO("[Algo Sell:{} enough available amount ins],exch:{},leftQty:{} ,exchBal:{}",order.symbol,exch,leftQty,exchBal[exch]); 
                        leftQty = 0;
                        break;
                    }
                    
                }
                else 
                {
                    if (childOrdMap[exch]->NewIns(teid,exch,exchQty ,parentOrd,order,'A'))
                    {
                        leftQty -= exchQty; 
                        exchBal[exch] += exchQty* order.prc;
                        LOG_INFO("[Algo Sell:{} unenough available amount ins],exch:{},exchQty:{} ,exchBal:{}",order.symbol,exch,exchQty,exchBal[exch]); 
                        algoPosS[exch][order.symbol] = 0;
                    }
                }
            }
        }

        if (leftQty >0)
        {
            LOG_INFO("[Alog:{} final unenough available amount ins]leftQty:{}",order.symbol,leftQty);
            parentOrdMgr->SetNotEnoughResp(teid,parentOrd,order,leftQty);
            if (order.side == 'B')
            {
                traderFund[TraderFundKey(teid,order.oseq)] += leftQty * order.prc;
                LOG_INFO("[After Algo Fail {} ] PID:{}, currend available fund:{}",order.symbol,order.pid,traderFund[TraderFundKey(teid,order.oseq)]);
            }
        }
    }
    return true;
}

bool TradePlan::OnReceiveCancel(int teid, hfs_order_t& order)
{
    LOG_INFO("[Cancel Ins: PID:{},symbol:{}]",order.pid,order.symbol);
    parentOrdMgr->CancelIns(teid,order);
    return true;
}

bool TradePlan::OnReceiveNewResp(int teid, std::string execid, hfs_order_t& order)
{
    ChildOrdMgr * childOrdMgr = childOrdMap[execid];
    childOrdMgr->ReceiveNewResp(teid,execid,order);
    
    return true;
}

bool TradePlan::OnReceiveFillResp(int teid, std::string execid, hfs_order_t& order)
{
    float fund = order.prc * order.qty;
    if (order.side == 'S')
    {
        traderFund[TraderFundKey(teid,order.oseq)] += fund;
        LOG_INFO("[After Sell {} ] PID:{}, currend available fund:{}",order.symbol,order.pid,traderFund[TraderFundKey(teid,order.oseq)]);
    }

    ChildOrdMgr * childOrdMgr = childOrdMap[execid];
    Ord * ord = childOrdMgr->ReceiveFillResp(teid,execid,order);

    if(ord != nullptr && ord->ocflag=='C' && order.side == 'S')
    {
        exchBal[execid] += fund;
        LOG_INFO("After Close Filled. PID:{};execid:{};fund:{};exchBal:{}",order.pid,execid,fund,exchBal[execid]);
    }
    return true;
}

bool TradePlan::OnReceiveCancelResp(int teid, std::string execid, hfs_order_t& order)
{  
     // change corresponding position
    ChildOrdMgr * childOrdMgr = childOrdMap[execid];
    Ord * ord = childOrdMgr->ReceiveCancelResp(teid,execid,order);
    if (ord == nullptr)
        return false;
    float fund = ord->prc * order.qty;
    if (ord != nullptr && ord->side == 'B')
    {
        
        traderFund[TraderFundKey(teid,order.oseq)] += fund;
        LOG_INFO("[After OnReceiveCancelResp {} ] PID:{}, currend available fund:{}",order.symbol,order.pid,traderFund[TraderFundKey(teid,order.oseq)]);
    }
    if (ord != nullptr && (ord->ocflag == 'O'|| ord->ocflag == 'A') && ord->side == 'B')
    {
        exchBal[execid] += fund;
        LOG_INFO("After Open Cancel  PID:{};execid:{};fund:{};exchBal:{}",order.pid,execid,fund,exchBal[execid]);
    }
    if(teid < Communicator::MaxTECount/2 )
    {
        UnfitPosKey uft(teid, order.oseq, order.symbol,order.side,execid); 
        auto iter = unfitPos.find(uft);
        if (iter != unfitPos.end() )
        {
            if (iter->second > order.qty)
            {
                iter->second  -= order.qty;
                exchPos[execid][order.symbol] += order.qty;
                LOG_INFO("After Cancel:exchPos[{}][{}]  return position to {}", execid,order.symbol,exchPos[execid][order.symbol] );
            }
            else
            {
                int openqty = order.qty - iter->second;
                exchPos[execid][order.symbol] += iter->second;
                unfitPos.erase (iter);
                if (openqty>0)
                {
                    uft.SwitchSide();
                    unfitPos[uft] =  openqty;
                    LOG_INFO("After part close Cancel: {}  return position to {}",uft.ToString(),openqty );
                }

            }
        }
        else 
        {
            uft.SwitchSide();
            unfitPos[uft] += order.qty;
            LOG_INFO("After open Cancel: {}  return position to {}",uft.ToString(),order.qty );
        }
    }
    else
    {
        if (order.side == 'B')
        {
            algoPosB[execid][order.symbol] += order.qty;
            LOG_INFO("After Algo Buy Cancel:exchPos[{}][{}]  return position to {}", execid,order.symbol,algoPosB[execid][order.symbol] );
        }
        else 
        {
            algoPosS[execid][order.symbol] += order.qty;
            LOG_INFO("After Algo Sell Cancel:exchPos[{}][{}]  return position to {}", execid,order.symbol,algoPosS[execid][order.symbol] );
        }
        
    }

    return true;
}

bool TradePlan::OnReceiveFailResp(int teid, std::string execid, hfs_order_t& order)
{
    ChildOrdMgr * childOrdMgr = childOrdMap[execid];
    Ord* ord = childOrdMgr->ReceiveFailResp(teid,execid,order);
    if (ord == nullptr)
        return false;

    UnfitPosKey uft(teid, order.oseq, order.symbol,order.side,execid);
    int failqty =  order.qty;
    if(teid < Communicator::MaxTECount/2 )
    {
        auto iter = unfitPos.find(uft);
        if (iter != unfitPos.end() )
        {
            if (iter->second > failqty)
            {
                iter->second  -= failqty;
                exchPos[execid][order.symbol] += failqty;
                LOG_INFO("After Fail:exchPos[{}][{}]  return position to {}]", execid,order.symbol,exchPos[execid][order.symbol] );
            }
            else
            {
                int openqty = failqty - iter->second;
                exchPos[execid][order.symbol] += iter->second;
                unfitPos.erase (iter);
                if (openqty>0)
                {
                    uft.SwitchSide();
                    unfitPos[uft] =  openqty;
                    LOG_INFO("After part close Fail: {}  return position to {}]",uft.ToString(),openqty );
                }
            }
        }
        else 
        {
            uft.SwitchSide();
            unfitPos[uft] += failqty;
            LOG_INFO("After open Fail: {}  return position to {}]",uft.ToString(),order.qty );
        }
    }
    else
    {
        if (order.side == 'B')
        {
            algoPosB[execid][order.symbol] += order.qty;
            LOG_INFO("After Algo Buy Fail:exchPos[{}][{}]  return position to {}", execid,order.symbol,algoPosB[execid][order.symbol] );
        }
        else 
        {
            algoPosS[execid][order.symbol] += order.qty;
            LOG_INFO("After Algo Sell Fail:exchPos[{}][{}]  return position to {}", execid,order.symbol,algoPosS[execid][order.symbol] );
        }
    }

    float fund = order.prc * order.qty;
    if (order.side == 'B')
    { 
        traderFund[TraderFundKey(teid,order.oseq)] += fund;
        LOG_INFO("[After OnReceiveFailResp {} ] PID:{}, currend available fund:{}",order.symbol,order.pid,fund);
    }

    if (ord != nullptr && (ord->ocflag == 'O'|| ord->ocflag == 'A') && order.side == 'B')
    {
        exchBal[execid] += fund;
        LOG_INFO("After Open Fail. PID:{},execid:{},fund:{},exchBal:{}",order.pid,execid,fund,exchBal[execid]);
    }

    return true;
}



int TradePlan::GetCurPos(string tkr) // search unfitPos first ,then exchPos.
{
    int curPos = 0;
   
    for ( auto& p : exchPos ) 
    {
        curPos += p.second[tkr];
    }
    return curPos;
}

int TradePlan::GetCurPos(string tkr,string exch)
{
    return exchPos[exch][tkr];
}

// void TradePlan::reload(string exch)
// {
//     LOG_INFO("Reload position {}",exch);
//     if (exch.length()==0)
//     {
//         for (string ex : exchList ) 
//         {
//             LoadAlgoPos(ex);
//         }
//     }
//     else 
//     {
//         LoadAlgoPos(exch);
//     }
// }

// void TradePlan::rejectpn(string exch)
// {
//     LOG_INFO("reject pending new  {}",exch);
//     if (exch.length()==0)
//     {
//         for (string ex : exchList ) 
//         {
//             RejectPendingNew(ex);
//         }
//     }
//     else 
//     {
//         RejectPendingNew(exch);
//     }
// }

// void TradePlan::cancelnew(string exch)
// {
//     LOG_INFO("cancel  new  {}",exch);
//     if (exch.length()==0)
//     {
//         for (string ex : exchList ) 
//         {
//             CancelNew(ex);
//         }
//     }
//     else 
//     {
//         CancelNew(exch);
//     }
// }

void TradePlan::LoadT0Pos(string exch)
{
    LOG_INFO("LoadT0Pos: {}",exch);
    if (exch.length()==0)
     {
         for (string ex : exchList ) 
         {
            LoadT0Pos(ex);
         }
     }
     else 
     {

        char sqlPos[1024] = {0};
    
        if (tag != "")
        { 
            sprintf(sqlPos,"select ACCOUNTID,TKR,QTY from T0HOLDINGS where ACCOUNTID='%s' and DTE=%d and TAG='%s'",exch.c_str(),today(),tag.c_str());
        }
        else 
        {
            sprintf(sqlPos,"select ACCOUNTID,TKR,QTY from T0HOLDINGS where ACCOUNTID='%s' and DTE=%d",exch.c_str(),today());
        }
        ResultSet* rs = db->query(sqlPos);

        while (rs && rs->next()){
            string ACCOUNTID =  rs->getString(1);
            string TKR =  rs->getString(2);
            int QTY = rs->getInt(3);
            
            //exchPos[ACCOUNTID][TKR] = QTY;
            //exchPosInit[ACCOUNTID][TKR] = QTY;

            //bug if not in new T0HOLDINGS.
            exchPos[ACCOUNTID][TKR] += QTY - exchPosInit[ACCOUNTID][TKR];
            exchPosInit[ACCOUNTID][TKR] = QTY;
        }
     }
}

void TradePlan::RemoveT0Pos(string exch)
{
    LOG_INFO("RemoveT0Pos: {}",exch);
    if (exch.length()==0)
     {
         for (string ex : exchList ) 
         {
            RemoveT0Pos(ex);
         }
     }
     else 
     {
        exchPos[exch].clear();
     }
}


void TradePlan::LoadAlgoPos(string exch)
{
    LOG_INFO("LoadAlgoPos: {}",exch);
    if (exch.length()==0)
     {
         for (string ex : exchList ) 
         {
            LoadAlgoPos(ex);
         }
     }
     else 
     {
        algoPosB[exch].clear();
        algoPosS[exch].clear();
        //db->start();
        char sqlIns[1024] = {0};
        if (tag != "")
        {
            sprintf(sqlIns, "select ACCOUNTID,TKR,Detlashare from instructs where ACCOUNTID='%s' and DTE=%d and TAG='%s'",exch.c_str(), today(),tag.c_str());
        }
        else 
        { 
            sprintf(sqlIns, "select ACCOUNTID,TKR,Detlashare from instructs where ACCOUNTID='%s' and DTE=%d",exch.c_str(),today());
        }
        ResultSet* rs = db->query(sqlIns);
        while (rs && rs->next())
        {
            string ACCOUNTID = rs->getString(1);   
            string TKR = rs->getString(2);  
            int Detlashare =  rs->getInt(3);  
            if (Detlashare>0)
            {
                Detlashare = Detlashare / 100 * 100;
            
                algoPosB[ACCOUNTID][TKR] = Detlashare;
            }
            else 
            {
                algoPosS[ACCOUNTID][TKR] = -Detlashare;
            }
        }
    //db->end();
    }

    
}

void TradePlan::RejectPendingNew(string exch)
{
    LOG_INFO("reject pending new  {}",exch);
     if (exch.length()==0)
     {
         for (string ex : exchList ) 
         {
            RejectPendingNew(ex);
         }
     }
     else 
     {
        ChildOrdMgr * com = childOrdMap[exch];
        if (com == nullptr )
        {
            LOG_ERROR("cancel new command failed: {}",exch)
        }
        com->RejectAll();
     }
}

void TradePlan::CancelNew(string exch)
{
    LOG_INFO("CancelNew:  {}",exch);
     if (exch.length()==0)
     {
         for (string ex : exchList ) 
         {
            CancelNew(ex);
         }
     }
     else 
     {
        ChildOrdMgr * com = childOrdMap[exch];
        if (com ==nullptr )
        {
            LOG_ERROR("cancel new command failed: {}",exch)
        }
        com->CancelAll();
     }
}

void TradePlan::SetAllOrdFinished()
{
    for (string exch : exchList ) 
    {
        LOG_INFO("Set All  {} Ord Finished",exch);
        for(auto &o : childOrdMap[exch]->orders)
        {
            Ord* ord = o.second;
            if(ord->nqty !=0  )
            {
                 //TODO create a fail order
                
            }
            else if(ord->pnqty != 0  )
            {
                //TODO create a fail order
                //ord->SetFailResp(ord->pnqty)
            }
        }
       
    }
}