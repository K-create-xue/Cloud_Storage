#pragma once


#define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING
#include"httplib.h"

#include<iostream>
#include<fstream>
#include<string>
#include<sstream>
#include<unordered_map>
#include<vector>
#include<utility>
#include<experimental/filesystem>
#ifdef _WIN32
#include<windows.h>
#else
#include<unistd.h>
#endif

namespace cloud_sys{
    namespace fs=std::experimental::filesystem;
    class ScanDir{
        private:
            std::string _path;
        public:
            ScanDir(const std::string& path):_path(path){
                //目录不存在则创建
	            if (!fs::exists(path)) {
		            fs::create_directories(path);
	            }
                if(_path.back()!='/'){
                    _path+='/';
                }
            }
            bool Scan(std::vector<std::string>* arry){
                for(auto& file:fs::directory_iterator(_path)){
                    std::string name;
                    name=file.path().filename().string();
                    std::string pathname = _path + name;
                    if (fs::is_directory(pathname)) {
                        continue;
                    }
                    arry->push_back(_path+name);
                }
                return true;
            }
    };

    class Util{
    public:
        static bool FileRead(const std::string &file,std::string* body){
            body->clear();
            std::ifstream infile;
            infile.open(file,std::ios::binary);
            if(infile.is_open()==false){
                std::cout<<"file open is failed!\n";
                return false;
            }
            uint64_t size=fs::file_size(file);
            body->resize(size);
            infile.read(&(*body)[0],size);
            if(infile.good()==false){
                std::cout<<"read file is failed!\n";
                return false;
            }
            infile.close();
            return true;
        }

        static bool FileWrite(const std::string &file,const std::string &body){
            std::ofstream outfile;
            outfile.open(file,std::ios::binary);
            if(outfile.is_open()==false){
                std::cout<<"open file is failed!\n";
                return false;
            }                
            outfile.write(&body[0],body.size());
            if(outfile.good()==false){
                std::cout<<"write file is failed!\n";
                return false;
            }
            outfile.close();
            return true;
        }
        
        static int Split(const std::string &str,const std::string &sp,std::vector<std::string>* arry){
            int count=0;
            size_t pos=0;
            size_t index=0;
            while(1){
                pos=str.find(sp,index);
                if(pos==std::string::npos){
                    break;
                }
                std::string temp=str.substr(index,pos-index);
                arry->push_back(temp);
                index=pos+sp.size();
                count++;
            }
            if(index!=str.size()){
            arry->push_back(str.substr(index));
            count++;
            }
            return count;
        }

        void unseria(){
            std::string file_name="test.dat";
            std::string body;
            std::ifstream infile;
            infile.open(file_name,std::ios::binary);
            if(!infile.is_open()){
                std::cout<<"open file is error\n";
                return;
            }
            uint64_t size=std::experimental::filesystem::file_size(file_name);
            body.resize(size);
            infile.read(&body[0],size);
            infile.close();

            std::vector<std::string> arry;
            Split(body,"\n",&arry);
            for(auto& e:arry){
                std::vector<std::string> kv;
                Split(e,"=",&kv);
                std::cout<<"["<<kv[0]<<"]=["<<kv[1]<<"]\n";
            }

        }

        void seria(){
            std::unordered_map<std::string,std::string> _map={
                {"main.txt","99123243324"},
                {"chile.cpp","99121312"},
                {"server.cpp","9913214"}
            };
            std::stringstream ss;
            ss.clear();

            for(auto& info:_map){
                ss<<info.first<<"="<<info.second<<std::endl;;
            }
            std::ofstream outfile;
            outfile.open("test.dat",std::ios::binary);
            outfile<<ss.str();
            outfile.close();
        }
    };


    class DataManager{
        private:
            std::string _path;
            std::unordered_map<std::string,std::string> _map;
        public:
            DataManager(const std::string& path):_path(path){}

            bool Read(){
                std::string body;
                //Util::FileRead(_path,&body);
                std::vector<std::string> arry;
                if (Util::FileRead(_path, &body) == false) {
                    std::cout << "read data set failed!\n";
                    return false;
                }
                Util::Split(body,"\n",&arry);
                for(auto& line:arry){
                    std::vector<std::string> kv;
                    Util::Split(line,"=",&kv);
                    _map[kv[0]] = kv[1];
                }
                return true;
            }


            bool Write(){
                std::stringstream ss;
                for(auto& e:_map){
                    ss<<e.first<<"="<<e.second<<"\n";
                }
                if(Util::FileWrite(_path,ss.str())==false){
                    std::cout<<"write data set failed!\n";
                    return false;
                }
                return true;
            }


            bool Exists(const std::string &key){
                auto it=_map.find(key);
                if(it==_map.end()){
                    return false;
                }
                return true;
            }
            
            bool AddOrMod(const std::string &key,const std::string &val){
                
                _map[key]=val;
                return true;
            }

            bool Del(const std::string &key,const std::string& val){
                auto it=_map.find(key);
                if(it==_map.end()){
                    std::cout<<"data bot exises!not to delete\n";
                    return false;
                }
                _map.erase(it);
                return true;
            }
            
            /*bool Mod(const std::string &key,const std::string &val){
                if(!Exists(key)){
                    std::cout<<"data not exists!\n";
                    return false;
                }
                _map[key]=val;
                return true;
            }*/

            bool Get(const std::string &key,std::string* ret){
                auto it=_map.find(key);
                if(it==_map.end()){
                    std::cout<<"data not exists!\n";
                    return false;
                }
                *ret=_map[key];
                return true;
            }
    };


#define LISTEN_DIR "./scandir"
#define CONFIG_FILE "./data.conf"
    class Client {
    private:
        ScanDir _scan;
        DataManager _data;
        httplib::Client* _client;
    public:
        Client(const std::string& host,int port):_scan(LISTEN_DIR),_data(CONFIG_FILE){
            _client = new httplib::Client(host, port);
        }

        std::string GetIndentifer(const std::string& path) {
            uint64_t mtime, fsize;
            fsize = fs::file_size(path);
            auto time_type = fs::last_write_time(path);
            mtime = decltype(time_type)::clock::to_time_t(time_type);
            std::stringstream ss;
            ss << fsize << mtime ;
            return ss.str();
        }
        

        bool Scan(std::vector<std::pair<std::string, std::string>>* arry) {

            std::vector<std::string> files;
            _scan.Scan(&files);
            for (auto& file : files) {
                std::string indentfier = GetIndentifer(file);
                if (_data.Exists(file) == false) {
                    arry->push_back(std::make_pair(file, indentfier));
                    continue;
                }
                std::string old;
                _data.Get(file, &old);
                if (old == indentfier) {
                    continue;
                }
                arry->push_back(std::make_pair(file, indentfier));
            }
            return true;
        }

        bool Upload(const std::string& path) {
            httplib::MultipartFormData file;
            file.name = "file";
            file.content_type = "application/octet-stream";
            fs::path p(path);
            file.filename = p.filename().string();
            Util::FileRead(path, &file.content);

            httplib::MultipartFormDataItems items;
            items.push_back(file);
            auto res=_client->Post("/multipart", items);
            if (res && res->status == 200) {
                std::cout << "upload success.\n";
                return true;
            }
            std::cout << "upload failed!\n";
            return false;
        }

        bool Start() {
            _data.Read();
            while (1) {
               
                std::vector<std::pair<std::string, std::string>>  arry;
                Scan(&arry);
                
                for (auto& file : arry) {
                    std::cout << file.first << "  need to storage infomation of file\n";
                    if (Upload(file.first) == false) {
                        continue;
                   }
                    _data.AddOrMod(file.first, file.second);
                    
                    _data.Write();
                }

                Sleep(1000);
            }
            return true;
        }
    };
}

