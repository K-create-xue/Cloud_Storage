#pragma noce

#define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING

#include"httplib.h"
#include"bundle.h"

#include<iostream>
#include<fstream>
#include<string>
#include<sstream>
#include<unordered_map>
#include<vector>
#include<experimental/filesystem>
#ifdef _WIN32
#include<windows.h>
#else
#include<unistd.h>
#endif

#include<pthread.h>
#include<sys/stat.h>
#include<time.h>


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
                    arry->push_back(_path+name);
                }
                return true;
            }
    };

    class Util{
    public:

        //Rang 断点续传的读取
        static bool RangeRead(const std::string& file,std::string* body,int64_t* begin,int64_t* end){
            body->clear();
            std::ifstream infile;
            infile.open(file,std::ios::binary);
            if(infile.is_open()==false){
                std::cout<<"file open is failed!\n";
                return false;
            }
            uint64_t size=fs::file_size(file);
            uint64_t rlen;
            if(*end==-1){
                *end=size-1;
            }
                rlen=*end-*begin+1;
            
            body->resize(rlen);
            infile.seekg(*begin,std::ios::beg);
            infile.read(&(*body)[0],rlen);
            if(infile.good()==false){
                std::cout<<"read file failed!\n";
                return false;
            }
            infile.close();
            return true;

        }

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
            std::cout<<file<<std::endl;
            if(outfile.is_open()==false){
                std::cout<<"open file is failed!!\n";
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
            pthread_rwlock_t _rwlock;
        public:
            DataManager(const std::string& path):_path(path){
                pthread_rwlock_init(&_rwlock,NULL);
            }
            ~DataManager(){
                pthread_rwlock_destroy(&_rwlock);
            }
            bool Read(){
                std::string body;
                std::vector<std::string> arry;
                if(Util::FileRead(_path,&body)==false){
                    std::cout<<"read data set failed!\n";
                    return false;
                }
                Util::Split(body,"\n",&arry);
                for(auto& line:arry){
                    std::vector<std::string> kv;
                    Util::Split(line,"=",&kv);
                    pthread_rwlock_wrlock(&_rwlock);
                    _map[kv[0]]=kv[1];
                    pthread_rwlock_unlock(&_rwlock);
                }
                return true;
            }

            bool Write(){
                std::stringstream ss;
                pthread_rwlock_rdlock(&_rwlock);
                for(auto& e:_map){
                    ss<<e.first<<"="<<e.second<<"\n";
                }
                pthread_rwlock_unlock(&_rwlock);
                if(Util::FileWrite(_path,ss.str())==false){
                    std::cout<<"write data set failed!\n";
                    return false;
                }
                return true;
            }

            bool Exists(const std::string &key){
                pthread_rwlock_rdlock(&_rwlock);
                auto it=_map.find(key);
                if(it==_map.end()){
                    pthread_rwlock_unlock(&_rwlock);
                    return false;
                }
                pthread_rwlock_unlock(&_rwlock);
                return true;
            }
            
            bool AddOrMod(const std::string &key,const std::string &val){
                pthread_rwlock_wrlock(&_rwlock);
                _map[key]=val;
                pthread_rwlock_unlock(&_rwlock);
                
                return true;
            }

            bool Del(const std::string &key,const std::string& val){
                pthread_rwlock_wrlock(&_rwlock);
                auto it=_map.find(key);
                if(it==_map.end()){
                pthread_rwlock_unlock(&_rwlock);
                    std::cout<<"data bot exises!not to delete\n";
                    return false;
                }
                _map.erase(it);
                pthread_rwlock_unlock(&_rwlock);
                return true;
            }
            

            bool Get(const std::string &key,std::string* ret){
                pthread_rwlock_rdlock(&_rwlock);

                auto it=_map.find(key);
                if(it==_map.end()){
                pthread_rwlock_unlock(&_rwlock);
                    std::cout<<"data not exists!\n";
                    return false;
                }
                *ret=_map[key];
                pthread_rwlock_unlock(&_rwlock);

                return true;
            }

            bool GetAllName(std::vector<std::string> *arry){
                arry->clear();
                pthread_rwlock_rdlock(&_rwlock);
                for(auto& file:_map){
                    arry->push_back(file.first);

                }
                pthread_rwlock_unlock(&_rwlock);
                return true;
            }
    };

#define BACKUP_PATH "./backup/"
#define CONFIG_PATH "./backup.conf"

    DataManager g_data(CONFIG_PATH);
    
    //压缩与解压缩的类
    class Compress{
        public:
            static bool Pack(const std::string& filename,const std::string& packname){
                std::string body;
                uint64_t fsize=fs::file_size(filename);
                body.resize(fsize);
                Util::FileRead(filename,&body);
                std::string str_pack=bundle::pack(bundle::LZIP,body);

                Util::FileWrite(packname,str_pack);
                return true;
            }

            static bool UnPack(const std::string& packname,const std::string& filename){
                std::string body;
                uint64_t fsize=fs::file_size(packname);
                body.resize(fsize);
                Util::FileRead(packname,&body);
                std::string str_unpack=bundle::unpack(body);
                Util::FileWrite(filename,body);
                return false;
            }
    };

    class Server{
        private:
            httplib::Server _srv;
        private:
            static void Upload(const httplib::Request &req,httplib::Response& rsp){
                std::cout<<"into Upload\n";
                auto ret=req.has_file("file");
                if(ret==false){
                    std::cout<<"No having file\n";
                    rsp.status=400;
                    return;
                }
                if(fs::exists(BACKUP_PATH)==false){
                    fs::create_directories(BACKUP_PATH);
                }
                const auto& file=req.get_file_value("file");
                std::string filename=BACKUP_PATH+file.filename;
                if(Util::FileWrite(filename,file.content)==false){
                    std::cout<<"write file data failed!\n";
                    rsp.status=500;
                    return;
                }
                g_data.AddOrMod(file.filename,file.filename);
                g_data.Write();
                return;
            }
            
            static void List(const httplib::Request& req,httplib::Response& rsp){
                std::cout<<"List inter.\n";
                std::stringstream ss;
                ss<<"<html><head><meta http-equiv='content-type' content='text/html;charset=utf-8'>";
                ss<<"</head><body>";
                std::vector<std::string> arry;
                g_data.GetAllName(&arry);
                for(auto& filename:arry){
                    ss<<"<hr />";
                    ss<<"<a href='/backup/"<<filename<<"'><strong>"<<filename<<"</strong></a>";
                }
                ss<<"<hr /></body></html>";
                rsp.body=ss.str();
                rsp.set_header("Content-Type","text/html");
                return;
            }
            
            static std::string GetIndentifer(const std::string& path) {
                uint64_t mtime, fsize;
                fsize = fs::file_size(path);
                //错误就在下面哪里，不知道卡在了哪里 就是一直不走了   找出它，，理解一下etag的值是由什么组织起来的
                auto time_type = fs::last_write_time(path);
                mtime = decltype(time_type)::clock::to_time_t(time_type);
                std::stringstream ss;
                ss << fsize << mtime ;
                std::cout<<ss.str()<<"\n";
                return ss.str();
            }

            static void Download(const httplib::Request& req,httplib::Response& rsp){
                std::cout<<"Download ......\n";
                std::string name=req.matches[1];
                std::cout<<name<<std::endl;
                std::string pathname=BACKUP_PATH+name;
                std::cout<<pathname<<std::endl;
                
                std::string newetag=GetIndentifer(name);
                uint64_t fsize=fs::file_size(name);
                //判断数据是否被压缩
                if(g_data.Exists(name)){
                    std::cout<<"存在文件\n";
                    std::string realname;
                    g_data.Get(name,&realname);
                    if(name!=realname){
                        std::cout<<"解压下载\n";
                        Compress::UnPack(realname,pathname);
                        unlink(realname.c_str());
                        g_data.AddOrMod(name,name);
                        g_data.Write();
                    }
                }
                
                std::cout<<newetag<<std::endl;
                if(req.has_header("If-Range")){
                    std::string oldetag=req.get_header_value("If-Range");
                    if(oldetag==newetag){
                        //断点区间，获取数据范围
                        std::cout<<req.ranges[0].first<<" - "<<req.ranges[0].second<<"\n";
                        int64_t start=req.ranges[0].first;
                        int64_t end=req.ranges[0].second;
                        Util::RangeRead(pathname,&rsp.body,&start,&end);
                        rsp.set_header("Content-Type","application/octet-stream");
                        rsp.set_header("ETag",newetag);
                        std::stringstream ss;
                        ss<<"bytes "<<start<<"-"<<end<<"/"<<fsize;//   /后面跟文件大小，不知道大小可以写 *
                        std::cout<<ss.str()<<std::endl;
                        rsp.set_header("Content-Range",ss.str());
                        rsp.status=206;
                        return;
                    }
                }
                if(Util::FileRead(pathname,&rsp.body)==false){
                    std::cout<<"read file "<<pathname<<" failed!\n";
                    rsp.status=500;
                    return;
                }
                std::cout<<"test string1\n";
                rsp.set_header("Content-Type","application/octet-stream");
                rsp.set_header("Accept-Range","bytes");
                rsp.set_header("ETag",newetag);
                rsp.status=200;

                return;
            }

        public:
            bool Start(int port=9003){
                g_data.Read();
                _srv.Post("/multipart",Upload);
                _srv.Get("/list",List);
                _srv.Get("/backup/(.*)",Download);
                _srv.listen("0.0.0.0",port);
                return true;
            }
    };


            //文件管理类
#define PACK_PATH "./packdir/"   //存放压缩包的文件
                    

            class FileManager{
                private:
                    ScanDir _scan;
                    time_t _hot_time=10;
                public:
                    time_t LastAccessTime(const std::string& filename){
                        struct stat st;
                        stat(filename.c_str(),&st);
                        return st.st_atime;
                    }
                public:
                    FileManager():_scan(BACKUP_PATH){

                    }
                    
                    bool Start(){
                        std::cout<<"FileManager inter.\n";
                        while(1){
                            std::vector<std::string> arry;
                            _scan.Scan(&arry);
                            for(auto& file:arry){
                                time_t atime=LastAccessTime(file);
                                time_t ctime=time(NULL);
                                if((ctime-atime)>_hot_time){

                                    fs::path fpath(file);
                                    std::string pack_filename=PACK_PATH+fpath.filename().string()+".pack";
                                    if(!fs::exists(PACK_PATH))
                                        fs::create_directory(PACK_PATH);
                                    Compress::Pack(file,pack_filename);
                                    unlink(file.c_str());
                                    g_data.AddOrMod(fpath.filename().string(),pack_filename);
                                    std::cout<<file<<"---"<<fpath.filename().string()<<std::endl;;
                                    g_data.Write();
                                    std::cout<<fpath<<"--bundle---->"<<pack_filename<<"\n";
                                }
                            }
                            
                            usleep(1000);
                        }
                    }
            };

    
}


