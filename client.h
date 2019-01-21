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

class CTransFile
{
    //这个模块用于文件传输
    //利用套接字将文件名队列中的文件传输到服务器
private:
    Client* _client;
    TcpSocket _socket;      //tcp套接字
    int _cur_statu;         //当前客户端状态
    int _file_fd;           //当前操作文件的文件描述符
    uint64_t _total_size;   //文件大小
    string _username;       //用户名
    string _passward;       //密码
    char* _root_dir;        //监控的路径
    
    bool StopCurFile
    {
        //停止传输文件
        if(_file_fd != -1)
        {
            close(_file_fd);
        }
        _file_fd = -1;
        _total_size = 0;
        _cur_statu = N_SNDHDR;
    }
    bool CommonReq(_req_type_t req_type,uint64_t len)
    {
        //状态同步：每一次进行登陆操作的时候同步状态
        //例如客户端是登陆验证状态的时候，
        //这个模块用send要保证对端是登陆接收状态
        CommonReq req(req_type,len);
        if(_socket.Send((void*)&req,sizeof(CommonReq))<0)
        {
            printf("send com req error\n");
            return false;
        }
        return true;
    }
    bool LoginReq()
    {
        //登陆的request，向服务器发送一个登陆请求
        CHECK_RET(CommonReq(REQ_LOGIN,sizeof(LoginInfo)));
        LoginInfo user(_username,_passward);
        if(_socket.Send((void*)&user,sizeof(LoginInfo))<0)
        {
            printf("send user info error!\n");
            return false;
        }
        //接收该登陆返回
        _cur_statu = N_RCVLOGIN;
        return true;
    }
    bool LoginRsp()
    {
        CommonReq rsp;
        if(_socket.Recv((void*)&rsp,sizeof(CommonReq))<0)
        {
            printf("rvcv login rsp error\n");
            return false;
        }
        if(!rsp.IsLoginRsp())
        {
            printf("not login rsp close!\n");
            return false;
        }
        if(rsp.GetLen() != 0)
        {
            char buff[1024] = {0};
            CHECK_ERR(_socket.Recv(Buff,rsp.GetLen()));
            printf("login faild:%s\n",buff);
            return false;
        }
        //接收到登陆的回复后开始传输文件协议（文件名和大小）
        _cur_statu = N_SNDHDR;
        return true;
    }
    bool SendFileHdr(string& file)
    {
        struct stat st;
        if(stat(file.c_str(),&st) < 0)
        {
            printf("get file stat error\n");
            return false;
        }
        CHECK_RET(CommonReq(REQ_UFILE,sizeof(FileInfo)));
        //传递去掉了rootdir之后的文件名
        string sflie(file,strlen(_root_dir));
        FileInfo freq(sfile,st.st_size);
        printf("send file name:%s len:%lu\n",freq.GetName(),Freq.GetLen());
        if(_socket.Send((void*)&freq,sizeof(FileInfo)) < 0)
        {
            printf("send file hdr error\n");
            return false;
        }
        if((_file_fd = open(file.c_str(),O_RDONLY))<0)
        {
            printf("open file %s error\n",file.c_str());
            return false;
        }
        _total_size = st.st_size;
        _cur_statu = N_SNDBODY;
        return true;
    }
    bool SendFileBody()
    {
        int slen = 0;
        int alen = 0;
        char buff[1024];
        while(alen < _total_size)
        {
            slen = read(_file_fd,buff,1024);
            CHECK_RET(_socket.Send(buff,slen));
            alen+=slen;
        }
        _cur_statu = N_SNDOVER;
        return true;
    }
    bool SendFileOver()
    {
        close(_file_fd);
        _file_fd = -1;
        _total_size = 0;
        //文件发送完毕，准备发送下一个文件
        _cur_statu = N_SNDHDR;
        //接受回复消息
        CommonReq rsp;
        CHECK_ERR(_socket.Recv((void*)&rsp,sizeof(CommonReq)));
        if(!rsp.IsUpLodeRsp())
        {
            printf("not file rsp\n");
            return false;
        }
        if(rsp.GetLen()!= 0)
        {
            char buff[1024] = {0};
            CHECK_ERR(socket.Recv(buff,rsp.GetLen()));
            prinf("send file failed:%s\n"buff);
            return true;
        }
        printf("file trans success!\n");
        return true;
    }
public:CTransFile(Client* client):
       _client(client),
       _total_size(0),
       _file_fd(-1),
       _cur_statu(N_START)
    {
        _root_dir = client -> GetRootDir();
        _username = client -> GetUserName();
        _passward = "111111";
    }
     bool Start()
    {
        string ip = _client -> GetStrAddr();
        uint16_t port = atoi(_client->GetStrPort());
        CHECK_RET(_socket.Socket());
        CHECK_RET(_socket.Connect(ip,port));
        while(1)
        {       
            switch(_cur_statu)
            {
                case N_START:
                    CHECK_RET(LoginReq());
                    break;
                case N_RCVLOGIN:
                    CHECK_RET(LoginRsp());
                    break;
                case N_SNDHDR:
                    string* file;
                    if(_client->QueuePopDate((void*)&file))
                    {
                        if(!SendFileHdr(*file))
                        {
                            printf("send file hdr error\n");
                            StopCurFile();
                        }
                        delete file;
                    }
                    break;
                case N_SNDBODY:
                    if(!SendFileBody())
                    {
                        printf("send file body error\n");
                        StopCurFile();
                    }
                    break;
                case N_SNDOVER:
                    if(!SendFileOver())
                    {
                        printf("recv file rsp error\n");
                        StopCurFile();
                    }
                    break;
                defaule:
                    printf("error status:%d\n",_cur_statu);
                    return false;
            }   
        }
        _socket.Close();
        return true;
    }
};

#endif
