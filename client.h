#ifndef _M_CLI_H__
#define _M_CLI_H__
#include<string>
#include<unordered_map>
#include<sys/stat.h>
#include<sys/inotify.h>
#include"common.h"

typedef struct inotify_event InotifyEvent;
using namespace std;

class Client
{
private:
    char* _root_dir;        //要监控的地址
    char* _str_addr;        //IP地址
    char* _str_port;        //网络端口
    char* _user_name;       //用户名
    BlockQueue _queue;      //上传队列
public:
    Client(char* root_dir,char* str_addr,char* str_port,char* user_name):
        _root_dir(root_dir),    
        _str_addr(str_addr),
        _str_port(str_port),
        _user_name(user_name),
        _queue()
    {}
    char* GetRootDir()
    {
        return _root_dir;
    }
    char* GetStrAddr()
    {
        return _str_addr;
    }
    char* GetStrPort()
    {
        return _str_port;
    }
    char* GetUserName()
    {
        return _user_name;
    }
    bool QueuePushDate(void* path)
    {
        _queue.PushDate(path);
        return true;
    }
    bool QueuePopDate(void** path)
    {
        _queue.PopDate(path)
        return true;
    }
};

class CScanFile
{
    //这个模块主要是对文件进行监控：
    //利用linux的inotify特性，对指定目录下的文件进行监控
    //当发生了写关闭的时候就将文件名加载到队列
private:
    Client* _client;
    int _inotify_fd;                       //inotify描述符
    char _event_buff[MAX_BUFF];            //事件信息
    uint64_t _total_size;                  //信息总长度
    uint64_t _cur_size;                    //文件名长度
    uint64_t _lst_event;                   //事件触发的条件（写关闭）
    unordered_map<int,string> _listen_dir; //描述符与目录对应
    
    bool InitInotify()
    {
        _inotify_fd=inotify_init();
        CHECK_ERR(_inotify_fd);
        return true;
    }
    bool AddListenDir(string& path)
    {
        //添加指定目录及进行监控
        int wd;
        wd = inotify_add_watch(_inotify_fd,path.c_str(),_lst_event);
        CHECK_ERR(wd);
        _listen_dir[wd]=path;
        return true;
    }
    bool DelListenDir(int wd)
    {
        //删除监控的目录
        CHECK_ERR(inotify_rm_watch(_inotify_fd,wd));
        _listen_dir.erase(wd);
        return true;
    }
    bool GetListenDir(int wd,string* path)
    {
        //验证path有没有对应的wd
        auto it = _listen_dir.find(wd);
        if(it == _listen_dir.end())
        {
            return false;
        }
        return true;
    }
    bool GetFullFile(InotifyEvent* evs,string* file)
    {
        CHECK_RET(evs);
        string path;
        CHECK_RET(GetListenDir(evs->wd,&path));
        char tmp[MAX_PATH]={0};
        int s = sprintf(tmp,"%s/%s",path.c_str(),evs->name);
        file->assign(tmp,s);
        return true;
    }
    bool CheckEvent()
    {
        fd_set rds;
        FD_ZERO(&rds);
        FD_SET(_inotify_fd,&rds);
        CHECK_ERR(select(_inotify_fd+1,&rds,NULL,NULL,NULL));
        return true;
    }
    InotifyEvent* NextEvent()
    {
        InotifyEvent* p_evs = NULL;
        if(_total_size == 0)
        {
            _cur_size = 0;
            _total_size = read(_inotify_fd,_event_buff,MAX_BUFF);
            if(_total_size <= 0)
            {
                return NULL;
            }
            p_evs = (InotifyEvent*) _event_buff;
            _cur_size += (sizeof(InotifyEvent) + p_evs->len);
            if(_cur_size == _total_size)
            {
                _total_size = 0;
            }
        }
        else if(_cur_size < (_total_size - sizeof(InotifyEvent)))
        {
            p_evs = (InotifyEvent*)(_event_buff + _cur_size);
            _cur_size += (sizeof(InotifyEvent) + p_evs->len);
            if(_cur_size == _total_size)
            {
                _total_size = 0;
            }
            else if(_cur_size > _total_size)
            {
                _total_size = 0;
                return NULL;
            }
        }
        return p_envs;
    }

public:
    CScanFile(Client* client):
        _client(client),
        _total_size(0),
        _cur_size(0),
        _lst_event(IN_CLOSE_WRITE)
    {}
    bool Start()
    {
        string root = _client -> GetRootDir();
        CHECK_RET(InitInotify());
        CHECK_RET(AddListenDir(root));
        while(1)
        {
            if(CheckEvent())
            {
                string* file = new string();
                if(GetFullFile(NextEvent(),file))
                {
                    _client ->QueuePushDate((void*)file);
                }
            }
        }
        return true;
    }
    ~CScanFile()
    {
        close(_inotify_fd);
    }

};


#endif






















