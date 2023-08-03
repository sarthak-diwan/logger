#include "logger.h"
#include <vector>

int main(){
    DEBUG_LOG("sarthak ", 10, " ", 20, " ", 1);
    std::string s("testing...");
    INFO_LOG(s.c_str(), 123);
    std::vector<std::thread> v;
    for(int i=0; i<100; i++){
        v.push_back(std::thread([i](){
            for(int j=0; j<100; j++){
                ERROR_LOG("Log By Thread: ", i, ", ", j);
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }));
    }
    for(auto &t: v){
        t.join();
    }
    return 0;
}