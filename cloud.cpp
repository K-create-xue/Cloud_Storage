/*************************************************************************
    > File Name: cloud.cpp
    > Author: Black_K
    > Mail: xzp01@foxmail.com 
    > Created Time: Thu 14 Apr 2022 07:50:08 PM CST
 ************************************************************************/

#include<thread>
#include<iostream>
#include"util.hpp"
using namespace std;

void thread_start(){
    cloud_sys::FileManager fm;
    fm.Start();
}

int main(){
    
    std::thread scan_thread(thread_start);
    scan_thread.detach();

    cloud_sys::Server _srv;
    _srv.Start();
    return 0;
}
