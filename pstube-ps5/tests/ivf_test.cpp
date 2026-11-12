#include "ivf.h"
#include <assert.h>
#include <vector>
int main() {
    std::vector<uint8_t> data(45,0);memcpy(data.data(),"DKIF",4);data[6]=32;
    memcpy(data.data()+8,"VP90",4);data[13]=4;data[15]=4;data[16]=60;data[20]=1;data[24]=1;data[32]=1;data[44]=42;
    probe::IvfView view;assert(view.open(data.data(),data.size()));
    const uint8_t* payload=nullptr;size_t length=0;assert(view.next(payload,length)&&length==1&&payload[0]==42);assert(!view.next(payload,length));
    for(size_t length=0;length<data.size();++length) assert(!view.open(data.data(),length));
    assert(!view.next(payload,length));
    data[32]=2;assert(!view.open(data.data(),data.size()));data[32]=1;
    data[24]=2;assert(!view.open(data.data(),data.size()));data[24]=1;
    data[16]=30;assert(!view.open(data.data(),data.size()));data[16]=60;
    data.push_back(0);assert(!view.open(data.data(),data.size()));
    assert(!view.open(nullptr,45));
}
