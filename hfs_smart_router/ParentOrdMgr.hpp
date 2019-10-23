#pragma once 
#include "Ord.hpp"
#include <unordered_map>
#include <iostream>
#include "HfsCommunicator.hpp"
using namespace std;

struct  ParentOrderKey
{
	int teid;
	unsigned int oseq;
	unsigned int pid;

	bool operator==(const ParentOrderKey & p) const
	{
		return teid == p.teid && oseq == p.oseq && pid == p.pid;
	}

};

struct hash_ParentOrderKey{
	size_t operator()(const ParentOrderKey & p) const {
		return hash<int>()(p.teid) ^ hash<unsigned int>()(p.oseq) ^ hash<unsigned int>()(p.pid);
	}
};

class ParentOrdMgr
{
public:
    ParentOrdMgr(hfs_adaptor_mgr & _adMgr,Communicator& _comm,bool selfBarginCtl);
    Ord* NewIns(int teid,hfs_order_t& origal_order);
    void CancelIns(int teid,hfs_order_t& origal_order);
    void SetNotEnoughResp(int teid,Ord* ord,hfs_order_t& origal_order,int qty);
	void SetFundNotEnoughResp(int teid,Ord* ord,hfs_order_t& origal_order,float availFund);
    unordered_map<ParentOrderKey,Ord*,hash_ParentOrderKey> orders;
	//map<string, float> selfbargin;
	vector<ParentOrderKey> unfinishedOrd;
private:      
    Communicator & comm;
    hfs_adaptor_mgr & adMgr;
	bool selfBarginCtl;

};