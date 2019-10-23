#include "hfs_xtp_adaptor.hpp"

using namespace std;

hfs_xtp_adaptor::hfs_xtp_adaptor(const a2ftool::XmlNode &cfg):hfs_base_adaptor(cfg) {
  m_accountid = cfg.getAttrDefault("account", "");
  m_passwd = cfg.getAttrDefault("password", "");
  m_server = cfg.getAttrDefault("server", "");
  m_port = cfg.getAttrDefault("port", 0);
  m_id = cfg.getAttrDefault("id", "");
  m_accountKey = cfg.getAttrDefault("accountKey", "");
  m_clientid = cfg.getAttrDefault("clientid", 11);
  m_export_path = cfg.getAttrDefault("export_path", "./export/");
  int timex = current();
  if (timex > 930000)
    m_orderRef = (timex / 10000 * 3600) + ((timex % 10000) / 100 * 60)
        + (timex % 100);
  else
    m_orderRef = 0;
  // m_orderRef = (today()%10)*100 + m_orderRef;
  m_rc = new RiskControl(cfg);
}

int hfs_xtp_adaptor::Init() {
  m_pUserApi = TraderApi::CreateTraderApi(m_clientid, "./", XTP_LOG_LEVEL_DEBUG);
  m_pUserApi->SubscribePublicTopic((XTP_TE_RESUME_TYPE) 2);
  m_pUserApi->SetSoftwareVersion("1.1.0");              //设定此软件的开发版本号，用户自定义
  m_pUserApi->SetSoftwareKey(m_accountKey.c_str());  //设定用户的开发代码，在XTP申请开户时，由xtp人员提供
  m_pUserApi->SetHeartBeatInterval(5);  //设定交易服务器超时时间，单位为秒，此为1.1.16新增接口
  m_pUserApi->RegisterSpi(this);
  int ret = m_pUserApi->Login(m_server.c_str(), m_port, m_accountid.c_str(),
                              m_passwd.c_str(), XTP_PROTOCOL_TCP);
  if (ret == 0) {
    LOG_INFO("[xtp adaptor] login error : {}",
             m_pUserApi->GetApiLastError()->error_msg);
    m_state = -1;
    return -1;
  }
  m_state = 2;
  m_sessionID = ret;
  initFile();
  // qryAccount();
  return 0;
}

void hfs_xtp_adaptor::initFile() {
  char fname[64] = { 0 };
  sprintf(fname, "%s/STOCK_HOLDING_%d.csv", m_export_path.c_str(), today());
  if (m_position_file.is_open()) {
    m_position_file.close();
  }
  m_position_file.open(fname, std::ios::out);
  m_position_file
      << "Account,StockCode,HoldingQty,TradeableQty,CostPrice,MarketValue,PositionStr"
      << endl;

  sprintf(fname, "%s/STOCK_ENTRUST_%d.csv", m_export_path.c_str(), today());
  if (m_entrust_file.is_open()) {
    m_entrust_file.close();
  }
  m_entrust_file.open(fname, std::ios::out);
  m_entrust_file
      << "Account,StockCode,TradeData,EntrustTime,OrderID,ClientID,EntrustSide,EntrustType,OrdStatus,Price,OrderQty,AvgPx,FilledQty,CancelledQty,Msg,PositionStr,OrigOrderID"
      << endl;

  sprintf(fname, "%s/STOCK_DETAIL_%d.csv", m_export_path.c_str(), today());
  if (m_detail_file.is_open()) {
    m_detail_file.close();
  }
  m_detail_file.open(fname, std::ios::out);
  m_detail_file
      << "Account,StockCode,TradeData,TradeTime,OrderID,EntrustSide,ExecType,LastPx,LastQty,PositionStr,OrigOrderID"
      << endl;

  sprintf(fname, "%s/STOCK_ACCOUNT_%d.csv", m_export_path.c_str(), today());
  if (m_account_file.is_open()) {
    m_account_file.close();
  }
  m_account_file.open(fname, std::ios::out);
  m_account_file << "Account,Available,MarketValue,AssetBalance" << endl;
}

bool hfs_xtp_adaptor::onRequest(int teid, hfs_order_t &order) {
  switch (order.type) {
    case 'O':
      process_new_entrust(teid, order);
      break;
    case 'X':
      process_cancel_entrust(teid, order);
      break;
    case 'H':
      //return adap.resendTradeHistory(teid);
      break;
    default:
      LOG_ERROR("[ht adaptor {}] unexpected order type: {}", m_id, order.type);
      return false;
  };
}

bool hfs_xtp_adaptor::process_new_entrust(int teid, hfs_order_t &order) {
  assert(order.type == 'O');
  dump(&order);
  if (m_state != 2) {
    LOG_INFO("[xtp adaptor {}] adaptor not login, entrust failed", m_id);
    push_reject_order(teid, order, "adaptor not login");
    return false;
  }

  if (!m_rc->VerifyAsset(&order) || !m_rc->VerifyPosition(&order)) {
    LOG_INFO("[xtp adaptor {}] check position and asset failed", m_id);
    push_reject_order(teid, order, "check position and asset failed");
    return false;
  }

  if (m_rc->OnInsertOrder(&order) == false || m_rc->IsCanInsert() == false) {
    LOG_INFO("[xtp adaptor {}] entrust failed, RiskControl refused", m_id);
    push_reject_order(teid, order, "RickControl refused");
    return false;
  }
  // char buf[10] = {0};
  // sprintf(buf, "%d%03d%ld", teid, order.oseq, ++m_orderRef);  //TODO

  XTPOrderInsertInfo info;
  memset(&info, 0, sizeof(info));
  info.order_client_id = teid * 100000 + (++m_orderRef);

  string symbol(order.symbol);
  strcpy(info.ticker, symbol.substr(0, 6).c_str());
  if (symbol.find("SH") != std::string::npos) {
    info.market = XTP_MKT_SH_A;
  } else if (symbol.find("SZ") != std::string::npos) {
    info.market = XTP_MKT_SZ_A;
  } else {
    LOG_ERROR("[xtp adaptor {}] process_new_entrust unkonwn symbol : {}", m_id,
              symbol);
    push_reject_order(teid, order, "unkonwn symbol");
    return false;
  }
  info.price = round(order.prc * 100) / 100.0;
  info.quantity = order.qty;
  if (order.side == 'B') {
    info.side = XTP_SIDE_BUY;
  } else if (order.side == 'S') {
    info.side = XTP_SIDE_SELL;
  } else {
    LOG_ERROR("[xtp adaptor {}] process_new_entrust unkonwn side : {}", m_id,
              symbol);
    push_reject_order(teid, order, "unkonwn side");
    return false;
  }
  info.price_type = XTP_PRICE_LIMIT;
  info.business_type = XTP_BUSINESS_TYPE_CASH;
  uint64_t ret = m_pUserApi->InsertOrder(&info, m_sessionID);
  if (ret == 0) {
    LOG_INFO("[xtp adaptor {}] insert order error : {}", m_id,
             m_pUserApi->GetApiLastError()->error_msg);
    push_reject_order(teid, order, m_pUserApi->GetApiLastError()->error_msg);
    return false;
  }

  // order.tm = current();
  sprintf(order.orderid, "%lu", ret);
  if (!registerTrader(teid, order, ret)) {
    LOG_ERROR(
        "[ht adaptor {}] register trader failed, teid:{}, pid:{}, oseq:{}, sid:{}",
        m_id, teid, order.pid, order.oseq, ret);
  }

  return true;
}

bool hfs_xtp_adaptor::process_cancel_entrust(int teid, hfs_order_t &order) {
  assert(order.type == 'X');
  dump(&order);
  if (m_state != 2) {
    LOG_INFO("[xtp adaptor {}] adaptor not login, cancel failed", m_id);
    return false;
  }
  if (m_rc->IsCanCancel() == false) {
    LOG_INFO("[xtp adaptor {}] RiskControl refuse cancel request", m_id);
    return false;
  }

  OrderID oid(teid, order.pid, order.oseq);
  EntrustNode *node = getENode(oid);
  if (node) {
    int ret = m_pUserApi->CancelOrder(node->sid, m_sessionID);
    if (ret == 0) {
      LOG_INFO("[xtp adaptor {}] cancel order failed : {}", m_id,
               m_pUserApi->GetApiLastError()->error_msg);
      return false;
    }
  } else {
    LOG_ERROR("[xtp adaptor {}] cancel not exists order, teid= {} oseq={}",
              m_id, teid, order.oseq);
    return true;
  }
  return true;
}

bool hfs_xtp_adaptor::start() {
  if (Init() != 0)
    return false;

  auto qry_thread = std::thread(
      std::bind(&hfs_xtp_adaptor::qryThreadFunc, this));
  qry_thread.detach();

  return true;
}

bool hfs_xtp_adaptor::stop() {
  return true;
}

void hfs_xtp_adaptor::OnDisconnected(uint64_t session_id, int reason) {
  LOG_INFO("[xtp adaptor {}] OnDisconnected reason:{}", m_id, reason);
  if (m_sessionID == session_id) {
    m_state = -1;
    LOG_INFO("[xtp adaptor {}] OnDisconnected reason:{}", m_id, reason);
    int ret = m_pUserApi->Login(m_server.c_str(), m_port, m_accountid.c_str(),
                                m_passwd.c_str(), XTP_PROTOCOL_TCP);
    if (ret == 0) {
      LOG_INFO("[xtp adaptor {}] login error : {}", m_id,
               m_pUserApi->GetApiLastError()->error_msg);
      return;
    }
    m_state = 2;
    m_sessionID = ret;
  }
}

void hfs_xtp_adaptor::OnError(XTPRI *error_info) {
  // IsErrorInfo(error_info);
  LOG_INFO("[xtp adaptor] OnError error_id:{}, ,error_msg:{}",
           error_info->error_id, error_info->error_msg);
}

void hfs_xtp_adaptor::OnOrderEvent(XTPOrderInfo *order_info, XTPRI *error_info,
                                   uint64_t session_id) {
  LOG_INFO(
      "[xtp adaptor {}] OnOrderEvent ticker:{}, order_xtp_id:{}, client_id:{}, status:{}, price:{}, qty:{}, side:{}, qty_left:{}",
      m_id, order_info->ticker, order_info->order_xtp_id,
      order_info->order_client_id, order_info->order_status, order_info->price,
      order_info->quantity, order_info->side, order_info->qty_left);

  uint64_t xtp_id = order_info->order_xtp_id;
  EntrustNode *enode = getENode(xtp_id);
  if (!enode) {
    LOG_INFO("[xtp adaptor {}] cannot find enode : {} ", m_id, xtp_id);
    return;
  }
  hfs_order_t order;
  memset(&order, 0, sizeof(hfs_order_t));
  strncpy(order.symbol, enode->origOrder.symbol, sizeof(order.symbol));
  sprintf(enode->origOrder.orderid, "%lu", xtp_id);
  order.idnum = enode->origOrder.idnum;
  order.side = enode->origOrder.side;
  order.ocflag = enode->origOrder.ocflag;
  order.pid = enode->nid.pid;
  order.oseq = enode->nid.oseq;

  if (IsErrorInfo(error_info)) {
    order.qty = enode->qty;
    order.type = 'J';
    order.state = ORD_REJECTED;
    enode->origOrder.state = ORD_REJECTED;
    strncpy(order.algo, error_info->error_msg, sizeof(order.algo) - 1);
    LOG_INFO("[xtp adaptor {}] OnOrderEvent get error:{}", m_id,
             error_info->error_msg);
  } else {
    if (order_info->order_status == XTP_ORDER_STATUS_NOTRADEQUEUEING) {
      // 'A'
      order.qty = enode->qty;
      order.type = 'A';
      order.state = ORD_NEW;
      enode->origOrder.state = ORD_NEW;
    } else if (order_info->order_status == XTP_ORDER_STATUS_CANCELED
        || order_info->order_status == XTP_ORDER_STATUS_PARTTRADEDNOTQUEUEING) {
      // 'C'
      order.qty = order_info->qty_left;
      order.type = 'C';
      order.state = ORD_CANCELED;
      enode->origOrder.state = ORD_CANCELED;
    } else if (order_info->order_status == XTP_ORDER_STATUS_REJECTED) {
      // J
      order.qty = enode->qty;
      order.type = 'J';
      order.state = ORD_REJECTED;
      enode->origOrder.state = ORD_REJECTED;
    } else {

      return;
    }
  }
  if (!onResponse(enode->nid.teid, m_id, order)) {
    LOG_ERROR("[xtp adaptor {}] onResponse failed, teid:{}, pid:{}, osep:{}",
              m_id, enode->nid.teid, enode->nid.pid, enode->nid.oseq);
  }
  m_rc->OnRtnOrder(enode->nid.teid, enode->origOrder);

}

void hfs_xtp_adaptor::OnTradeEvent(XTPTradeReport *trade_info,
                                   uint64_t session_id) {
  LOG_INFO(
      "[xtp adaptor {}] OnTradeEvent ticker:{}, order_xtp_id:{}, client_id:{}, price:{}, vol:{}, side:{}",
      m_id, trade_info->ticker, trade_info->order_xtp_id,
      trade_info->order_client_id, trade_info->price, trade_info->quantity,
      trade_info->side);

  uint64_t order_id = trade_info->order_xtp_id;
  EntrustNode *enode = getENode(order_id);
  if (!enode) {
    LOG_INFO("[xtp adaptor {}] OnTradeEvent cannot find enode : {}", m_id,
             order_id);
    return;
  }
  hfs_order_t order;
  memset(&order, 0, sizeof(hfs_order_t));
  strncpy(order.symbol, enode->origOrder.symbol, sizeof(order.symbol));
  order.idnum = enode->origOrder.idnum;
  order.side = enode->origOrder.side;
  order.ocflag = enode->origOrder.ocflag;
  order.pid = enode->nid.pid;
  order.oseq = enode->nid.oseq;
  sprintf(enode->origOrder.orderid, "%lu", order_id);

  double prc = (enode->eamt + trade_info->trade_amount)
      / (trade_info->quantity + enode->eqty + 0.000001);
  order.state = ORD_NEW;
  enode->origOrder.state = ORD_NEW;
  order.type = 'E';
  order.qty = trade_info->quantity;
  order.prc = prc;
  enode->eqty += trade_info->quantity;  // 增量成交
  enode->eamt += trade_info->trade_amount;
  if (enode->eqty >= enode->qty) {
    enode->origOrder.state = ORD_FILLED;
  }

  if (!onResponse(enode->nid.teid, m_id, order)) {
    LOG_ERROR("[ht_adaptor {}] onResponse failed, teid:{}, pid:{}, osep:{}",
              m_id, enode->nid.teid, enode->nid.pid, enode->nid.oseq);
  }
  m_rc->OnRtnOrder(enode->nid.teid, enode->origOrder);
  // TODO holding
  m_rc->OnShiftHoldings(order);
}

void hfs_xtp_adaptor::OnCancelOrderError(XTPOrderCancelInfo *cancel_info,
                                         XTPRI *error_info,
                                         uint64_t session_id) {
  if (IsErrorInfo(error_info)) {
    uint64_t order_id = cancel_info->order_xtp_id;
    EntrustNode *enode = getENode(order_id);
    if (!enode)
      return;
    LOG_ERROR("[xtp adaptor {}] OnCancelOrderError : {}", m_id, order_id);
  }
}

void hfs_xtp_adaptor::OnQueryOrder(XTPQueryOrderRsp *order_info,
                                   XTPRI *error_info, int request_id,
                                   bool is_last, uint64_t session_id) {
  LOG_INFO("[xtp adaptor] OnQueryOrder");
  if (IsErrorInfo(error_info)) {

    return;
  }
  // Account,StockCode,TradeData,EntrustTime,OrderID,ClientID,EntrustSide,EntrustType,OrdStatus,Price,OrderQty,AvgPx,FilledQty,CancelledQty,Msg,PositionStr,OrigOrderID
  m_entrust_file << m_accountid << "," << order_info->ticker << ","
      << order_info->insert_time / 1000000000 << "," << order_info->insert_time
      << "," << order_info->order_xtp_id << "," << order_info->order_client_id
      << "," << (int) order_info->side << "," << (int) order_info->order_type
      << "," << order_info->order_status << "," << order_info->price << ","
      << order_info->quantity << ","
      << order_info->trade_amount / order_info->qty_traded << ","
      << order_info->qty_traded << "," << order_info->qty_left << ",,,"
      << order_info->order_cancel_client_id << endl;
  m_entrust_file.flush();
  if (order_info->insert_time > m_qry_begin_time) {
    m_qry_begin_time = order_info->insert_time;
  }

  if (order_info->order_status == XTP_ORDER_STATUS_INIT
      || order_info->order_status == XTP_ORDER_STATUS_PARTTRADEDQUEUEING
      || order_info->order_status == XTP_ORDER_STATUS_NOTRADEQUEUEING) {
    int ret = m_pUserApi->CancelOrder(order_info->order_xtp_id, m_sessionID);
    if (ret == 0) {
      LOG_INFO("[xtp adaptor] cancel order failed : {}", ret);
    }
  }
  if (is_last) {

  }
}

void hfs_xtp_adaptor::OnQueryPosition(XTPQueryStkPositionRsp *position,
                                      XTPRI *error_info, int request_id,
                                      bool is_last, uint64_t session_id) {
  LOG_INFO("[xtp adaptor] OnQueryPosition");
  if (IsErrorInfo(error_info)) {
    qryOrders();
    return;
  }
  string instrumentID = position->ticker;
  if (position->market == XTP_MKT_SH_A) {
    instrumentID.append(".SH");
  } else {
    instrumentID.append(".SZ");
  }
  // "Account,StockCode,HoldingQty,TradeableQty,CostPrice,MarketValue,PositionStr"
  m_position_file << m_accountid << "," << instrumentID << ","
      << position->total_qty << "," << position->sellable_qty << ","
      << position->avg_price << "," << position->total_qty * position->avg_price
      << "," << endl;
  m_position_file.flush();
  auto iter = m_rc->m_holdings.find(instrumentID);
  if (iter == m_rc->m_holdings.end()) {
    hfs_holdings_t *holding = new hfs_holdings_t();
    memset(holding, 0, sizeof(hfs_holdings_t));
    strcpy(holding->symbol, position->ticker);
    strcpy(holding->exch, position->market == XTP_MKT_SH_A ? "SH" : "SZ");
    holding->cidx = -1;
    m_rc->m_holdings.insert(make_pair(instrumentID, holding));
  }
  hfs_holdings_t *holding = m_rc->m_holdings[instrumentID];
  holding->qty_long += position->total_qty;
  holding->qty_long_yd += position->sellable_qty;
  holding->avgprc_long = position->avg_price;
  if (is_last) {
    qryOrders();
  }
}

void hfs_xtp_adaptor::OnQueryAsset(XTPQueryAssetRsp *asset, XTPRI *error_info,
                                   int request_id, bool is_last,
                                   uint64_t session_id) {
  LOG_INFO("[xtp adaptor] OnQueryAsset");
  if (IsErrorInfo(error_info)) {
    qryTrade();
    return;
  }
  //Account,Available,MarketValue,AssetBalance
  m_account_file << m_accountid << "," << asset->buying_power << ","
      << asset->total_asset - asset->buying_power << "," << asset->total_asset
      << endl;
  m_rc->m_accountinfo.capital = asset->total_asset;
  m_rc->m_accountinfo.available = asset->buying_power;
  if (is_last) {
    qryTrade();
  }
}

void hfs_xtp_adaptor::OnQueryTrade(XTPQueryTradeRsp *trade_info,
                                   XTPRI *error_info, int request_id,
                                   bool is_last, uint64_t session_id) {
  LOG_INFO("[xtp adaptor] OnQueryTrade");
  if (IsErrorInfo(error_info)) {
    qryPosition();
    return;
  }
  // Account,StockCode,TradeData,TradeTime,OrderID,EntrustSide,ExecType,LastPx,LastQty,PositionStr,OrigOrderID
  m_detail_file << m_accountid << "," << trade_info->ticker << ","
      << trade_info->trade_time / 1000000000 << "," << trade_info->trade_time
      << "," << trade_info->order_xtp_id << "," << (int) trade_info->side << ","
      << trade_info->trade_type << "," << trade_info->price << ","
      << trade_info->quantity << ",," << trade_info->order_client_id << endl;
  m_detail_file.flush();
  if (is_last) {
    qryPosition();
  }
}
void hfs_xtp_adaptor::qryThreadFunc() {
  while (true) {
    char fname[64] = { 0 };
    sprintf(fname, "%s/STOCK_HOLDING_%d.csv", m_export_path.c_str(), today());
    if (m_position_file.is_open()) {
      m_position_file.close();
    }
    m_position_file.open(fname, std::ios::out);
    m_position_file
        << "Account,StockCode,HoldingQty,TradeableQty,CostPrice,MarketValue,PositionStr"
        << endl;

    sprintf(fname, "%s/STOCK_ENTRUST_%d.csv", m_export_path.c_str(), today());
    if (!m_entrust_file.is_open()) {
      m_entrust_file.open(fname, std::ios::app);
    }

    sprintf(fname, "%s/STOCK_DETAIL_%d.csv", m_export_path.c_str(), today());
    if (!m_detail_file.is_open()) {
      m_detail_file.open(fname, std::ios::app);
    }

    sprintf(fname, "%s/STOCK_ACCOUNT_%d.csv", m_export_path.c_str(), today());
    if (m_account_file.is_open()) {
      m_account_file.close();
    }
    m_account_file.open(fname, std::ios::out);
    m_account_file << "Account,Available,MarketValue,AssetBalance" << endl;

    qryAccount();

    sleep(5 * 60);  // 每隔5分钟查询一次全量数据
  }
}

void hfs_xtp_adaptor::qryAccount() {
  int ret = m_pUserApi->QueryAsset(m_sessionID, ++m_requestID);
  if (ret != 0) {
    LOG_INFO("[xtp adaptor] qry account error : {}",
             m_pUserApi->GetApiLastError()->error_msg);
  }
}

void hfs_xtp_adaptor::qryOrders() {
  XTPQueryOrderReq req;
  memset(&req, 0, sizeof(req));
  req.begin_time = 0;

  int ret = m_pUserApi->QueryOrders(&req, m_sessionID, ++m_requestID);
  if (ret != 0) {
    LOG_INFO("[xtp adaptor] qry order error : {}",
             m_pUserApi->GetApiLastError()->error_msg);
  }
}

void hfs_xtp_adaptor::qryPosition() {
  int ret = m_pUserApi->QueryPosition("", m_sessionID, ++m_requestID);
  if (ret != 0) {
    LOG_INFO("[xtp adaptor] qry position error : ",
             m_pUserApi->GetApiLastError()->error_msg);
  }
}

void hfs_xtp_adaptor::qryTrade() {
  XTPQueryTraderReq req;
  memset(&req, 0, sizeof(req));
  req.begin_time = 0;
  LOG_INFO("[xtp adaptor] qry trade from : {}", m_qry_begin_time);
  int ret = m_pUserApi->QueryTrades(&req, m_sessionID, ++m_requestID);
  if (ret != 0) {
    LOG_INFO("[xtp adaptor] qry trade error : {}",
             m_pUserApi->GetApiLastError()->error_msg);
  }
}

bool hfs_xtp_adaptor::IsErrorInfo(XTPRI *error_info) {
  if (error_info && error_info->error_id != 0) {
    LOG_INFO("[xtp adaptor] error_id:{}, ,error_msg:{}", error_info->error_id,
             error_info->error_msg);
    return true;
  }
  return false;
}

bool hfs_xtp_adaptor::registerTrader(int teid, const hfs_order_t &order,
                                     uint64_t sid) {
  try {
    EntrustNode *node = new EntrustNode();
    memset(node, 0, sizeof(EntrustNode));
    node->nid.teid = teid;
    node->nid.pid = order.pid;
    node->nid.oseq = order.oseq;
    node->sid = sid;
    strcpy(node->tkr, order.symbol);
    node->qty = order.qty;
    node->eqty = 0;
    node->canceled = 0;
    memcpy(&node->origOrder, &order, sizeof(hfs_order_t));
    {
      std::lock_guard < std::mutex > lk(m_nid_mtx);
      // this->evec.push_back(node);
      this->nmap.insert(make_pair(node->nid, node));
    }
    {
      std::lock_guard < std::mutex > lk(m_sid_mtx);
      this->smap.insert(make_pair(sid, node));
    }
    LOG_INFO("[xtp adaptor {}] register trader : {}", m_id, sid);
    return true;
  } catch (std::exception &ex) {
    LOG_ERROR("[xtp adaptor {}] register trader {} failed : {}", m_id, sid,
              ex.what());
    return false;
  }
}

void hfs_xtp_adaptor::push_reject_order(int teid, hfs_order_t &order,
                                        const char *errmsg) {
  hfs_order_t od;
  memcpy(&od, &order, sizeof(hfs_order_t));
  od.qty = order.qty;
  od.type = 'J';
  od.state = ORD_REJECTED;
  strncpy(od.algo, errmsg, sizeof(od.algo) - 1);
  if (!onResponse(teid, m_id, od)) {
    LOG_ERROR("[xtp adaptor {}] onResponse failed, teid:{}, pid:{}, osep:{}",
              m_id, teid, order.pid, order.oseq);
  }
}

EntrustNode* hfs_xtp_adaptor::getENode(OrderID &oid) {
  std::lock_guard < std::mutex > lk(m_nid_mtx);
  EntrustNode *rv = nullptr;
  if (nmap.find(oid) != nmap.end()) {
    return nmap[oid];
  }
  return rv;
}

EntrustNode* hfs_xtp_adaptor::getENode(uint64_t sid) {
  std::lock_guard < std::mutex > lk(m_sid_mtx);
  EntrustNode *enode = nullptr;

  auto iter = smap.find(sid);
  if (iter != smap.end()) {
    enode = smap[sid];
  }
  return enode;
}

void hfs_xtp_adaptor::dump(hfs_holdings_t *holding) {
  if (!holding)
    return;
  LOG_INFO(
      "[ht adaptor {}]holding:symbol:{},cidx:{},exch:{},qty_long:{},qty_short:{},qty_long_yd:{},qty_short_yd:{},avgprc_long:{},avgprc_short:{}",
      m_id, holding->symbol, holding->cidx, holding->exch, holding->qty_long,
      holding->qty_short, holding->qty_long_yd, holding->qty_short_yd,
      holding->avgprc_long, holding->avgprc_short);
}

void hfs_xtp_adaptor::dump(hfs_order_t *order) {
  if (!order)
    return;

  LOG_INFO(
      "[ht adaptor {}]order:symbol:{},exch:{},pid:{},orderid:{},tm:{},type:{},qty:{},prc:{},side:{},ocflag:{},state:{}",
      m_id, order->symbol, order->exch, order->pid, order->orderid, order->tm,
      (char) order->type, order->qty, order->prc, (char) order->side,
      (char) order->ocflag, (char) order->state);
}

//加密
void hfs_xtp_adaptor::encode(char *pstr, int *pkey) {
  int len = strlen(pstr);  //获坖长度
  for (int i = 0; i < len; i++)
    *(pstr + i) = *(pstr + i) ^ i;
}

//解密
void hfs_xtp_adaptor::decode(char *pstr, int *pkey) {
  int len = strlen(pstr);
  for (int i = 0; i < len; i++)
    *(pstr + i) = *(pstr + i) ^ i;
}

std::string hfs_xtp_adaptor::getOPStation(int traderid) {
  if (m_opstation.find(traderid) != m_opstation.end()) {
    return m_opstation[traderid];
  }
  return m_sysinfo;
}
