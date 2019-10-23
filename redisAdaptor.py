import struct
import socket
import time,random
import sys,pdb,datetime
import threading, redis
import traceback

server_ip = "127.0.0.1"
server_port = 12345
redis_ip = "192.168.1.16"
redis_port = 6379

class redisAdaptor:
    def __init__(self, acc_id, ins_key, ord_key):
        self.teid = 1
        self.ins_key = ins_key
        self.ord_key = ord_key
        self.acc_id = acc_id
        self.fd = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.fd.connect((server_ip, server_port))
        self.r = redis.Redis(redis_ip, redis_port)
        self.order_t = '<16s64s8s64sIIIIIifccccfifi'
        self.pid = 1
        self.order_map = {}
    
    def _get_ord2(self, client_id):
        for key in self.order_map.keys():
            if self.order_map[key]['client_id'] == client_id:
                return key, self.order_map[key]

        return None, None

    def _get_ord(self, tseq, pid):
        s = "{}_{}".format(tseq, pid)
        if s in self.order_map:
            return self.order_map[s]
        else :
            return None
    def _get_value(self, data, key):
        pos = data.find(key) + len(key) + 1
        pos_end = data.find(',', pos)
        return data[pos: pos_end]

    def _new_order(self):
        od = {}
        od['client_id'] = ''
        od['ord_no'] = ''
        od['ord_status'] = ''
        od['ord_type'] = ''
        od['acct_type'] = ''
        od['acct_type'] = ''
        od['acct_id'] = ''
        od['symbol'] = ''
        od['tradeside'] = ''
        od['ord_qty'] = 0
        od['ord_px'] = 0.0
        od['ord_type'] = ''
        od['filled_qty'] = 0
        od['avg_px'] = 0.0
        od['cxl_qty'] = 0
        od['ord_time'] = ''
        od['err_msg'] = ''
        return od
    
    def login(self):
        sendStr = struct.pack(self.order_t, '_LOGIN_', '', '', '',  0, 0, self.teid, 0, 0, 0, 0, 'H', 'B', 'O', '0', 0, 0, 0, 0)
        self.fd.send(sendStr)
        
    def resend(self):
        sendStr = struct.pack(self.order_t, '_RESEND_', '', '', '', 0, 0, 0, 0, 0, 0, 0, 'H', 'B', 'O', '0', 0, 0, 0, 0)
        self.fd.send(sendStr)

    def read_from_socket(self):
        while(True):
            try:
                buf = self.fd.recv(struct.calcsize(self.order_t) )
                #print 'read ', buf
                symbol, algo, exch, orderid, idnum, pid, oseq, tseq, tm, qty, prc, tp, side, ocflag, state, ask, asksiz, bid, bidsiz =  struct.unpack(self.order_t, buf)
                print "pid:", pid, "oseq:", oseq, "orderid:", orderid
                str_ord = "order:[client_id:{},ord_no:{},ord_status:{},ord_type:{},acct_type:,acct_id:{},cats_acct:,symbol:{},tradeside:{},ord_qty:{},ord_px:{},ord_type:{},ord_param:,corr_type:,corr_id:,filled_qty:{},avg_px:{},cxl_qty:{},ord_time:{},err_msg:{},,reported_qty:200]"
                
                od = self._get_ord(oseq, pid)
                if not od :
                    continue
                od['ord_no'] = orderid.strip()
                if not od :
                    return 
                if tp == 'A':
                    od['ord_status'] = '0'
                elif tp == 'E':
                    od['filled_qty'] = qty
                    od['avg_px'] = prc
                    if od['filled_qty'] >= od['ord_qty']:
                        od['ord_status'] = '2'
                    else :
                        od['ord_status'] = '1'
                elif tp == 'C':
                    od['cxl_qty'] = qty
                    od['ord_status'] = '4'
                elif tp == 'J':
                    od['ord_status'] = '6'
                    od['err_msg'] = 'rejected'
                elif tp == 'O':
                    od['ord_status'] = '8'

                str_ord = str_ord.format(od['client_id'], od['client_id'], od['ord_status'], od['ord_type'], od['acct_id'], od['symbol'], od['tradeside'], od['ord_qty'], od['ord_px'], od['ord_type'], od['filled_qty'], od['avg_px'], od['cxl_qty'], od['ord_time'], od['err_msg'])
                print str_ord
                str_ord = str_ord.replace('[','{')
                str_ord = str_ord.replace(']','}')
                self.r.lpush(self.ord_key, str_ord)
            except Exception, e:
                print 'read_from_socket', e
                traceback.print_exc()
                #pdb.set_trace()
            time.sleep(0.01)

    
    def read_from_redis(self):
        while(True):
            try:
                data = self.r.rpop(self.ins_key)
                if data and data != "":
                    print data
                    od = self._new_order()
                    od['client_id'] = self._get_value(data, "client_id")
                    od['ord_no'] = self._get_value(data, "order_no")
                    od['ord_status'] = '?'
                    od['ord_type'] = self._get_value(data, "inst_type")
                    od['acct_id'] = self._get_value(data, "acct_id")
                    od['symbol'] = self._get_value(data, "symbol")
                    od['tradeside'] = self._get_value(data, "tradeside")
                    od['ord_qty'] = int(self._get_value(data, "ord_qty"))
                    od['ord_px'] = float(self._get_value(data, "ord_price"))
                    od['ord_time'] = datetime.datetime.today().strftime("%Y-%m-%d %H%M%S")    
                    if od['client_id']=='':
                        continue 
                    #trader, _ = od['client_id'].strip().split('_')
                    if od['ord_type']=='C' and self._get_ord2(od['ord_no']):                        
                        key, org_od = self._get_ord2(od['ord_no'])
                        if not org_od:
                            continue
                        print 'cancel ', key, org_od['client_id'], org_od['ord_no']
                        trader, pid = key.strip().split('_')
                        sendStr = struct.pack(self.order_t, org_od['symbol'], '', self.acc_id, org_od['ord_no'], 0, int(pid), int(trader), 0, int(org_od['ord_time'][-6:]), org_od['ord_qty'], org_od['ord_px'], 'X', 'B' if od['tradeside']=='1' else 'S', 'O', '0', 0, 0, 0, 0)
                        self.fd.send(sendStr)
                    else:
                        trader, _ = od['client_id'].strip().split('_')
                        self.order_map["{}_{}".format(trader, self.pid)] = od        
                        sendStr = struct.pack(self.order_t, od['symbol'], '', self.acc_id, "", 0, self.pid, int(trader), 0, int(od['ord_time'][-6:]), od['ord_qty'], od['ord_px'], 'O', 'B' if od['tradeside']=='1' else 'S', 'O', '0', 0, 0, 0, 0)
                        self.fd.send(sendStr)
                        self.pid = self.pid + 1
            except Exception, e:
                print 'read_from_redis', e
                traceback.print_exc()
                #pdb.set_trace()
            time.sleep(0.01)

    
    def run(self):
        self.login()
        self.resend()
        t1 = threading.Thread(target=self.read_from_socket,args=())
        t1.start()
        # t2 = threading.Thread(target=read_from_redis,args=(self,))
        # t2.start()
        self.read_from_redis()

if __name__ == '__main__':
    redisAdaptor('ZXXX_S', "T0TRADE_TEST_INS", "T0TRADE_TEST_ORD").run()
