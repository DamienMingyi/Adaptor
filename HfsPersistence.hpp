#ifndef __HFS_PERSISTENCE_H__
#define __HFS_PERSISTENCE_H__

#include <boost/interprocess/file_mapping.hpp>
#include <boost/interprocess/mapped_region.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/filesystem.hpp>
#include <fstream>
#include <string>
#include <iostream>

using namespace std;
using namespace boost::interprocess;

template<class T> 
class InfoStore {
private:
    boost::interprocess::file_mapping  *mfile;
    boost::interprocess::mapped_region *region;
    boost::mutex                        rwmutex;

    int *pteid;
    int *nelem;
    T   *data;
    
    int ElemCount;

private:
    InfoStore();
    int today();

public:
    InfoStore(int store_size, string fname);
    ~InfoStore();

public:
    T   *getData() { return data; }
    int  getCount() { return *nelem; }
    T   *append(const T *elem);
    boost::mutex &getMutex() { return rwmutex; }
};


//======================== implementation ==============================

template<class T>
int InfoStore<T>::today() {
    time_t t;
    struct tm *tmp;
    t = time(NULL);
    tmp = localtime(&t);
    char buf[64];
    strftime(buf, 64, "%Y%m%d", tmp);
    return atoi(buf);
}

template<class T>
InfoStore<T>::InfoStore(int store_size, string fname):mfile(0),region(0),nelem(0),data(0), ElemCount(store_size) {
    if(!boost::filesystem::exists(fname)) { //如果不存在，则创建该文件，并将值赋为0
        std::filebuf fbuf;
        fbuf.open(fname.c_str(), std::ios_base::in | std::ios_base::out | std::ios_base::trunc | std::ios_base::binary);
        fbuf.pubseekoff(sizeof(int) + ElemCount * sizeof(T), std::ios_base::beg);
        fbuf.sputc(0);
        fbuf.close();   
    }

    try {
        mfile = new file_mapping(fname.c_str(), read_write);
        region = new mapped_region(*mfile, read_write);

        char * addr       = (char*)region->get_address();

        nelem = (int *)(addr);
        data  = (T*)(addr + sizeof(int));
    }
    catch(interprocess_exception &ex) {
        cerr << "Initialize InfoStore error! fname:" << fname << '\t' << ex.what() << endl;
        return;
    }
    fprintf(stderr, "mmapped file: [ %s ]\n", fname.c_str());
}

template<class T>
InfoStore<T>::~InfoStore() {
    if(region) {
        delete region; region = NULL;
    }
    if(mfile) {
        delete mfile; mfile = NULL;
    }
}

template<class T>
T *InfoStore<T>::append(const T *elem) {
    memcpy(data + *nelem, elem, sizeof(T));
    (*nelem)++;
    return data + *nelem - 1;
}

#endif
