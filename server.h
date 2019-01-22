#ifndef __M_SRV_H__
#define __M_SRV_H__

#include<sys/epoll.h>
#include"common.h"

#define MAX_EPOLL 1000

class Server
{
private:
    char* _root_dir;
    char* _str_addr;
    char* _str_port;
    //登陆线程将登陆成功后的客户端信息
    //传递给传输线程开始文件传输
    BlockQueue _login_queue;
public:
    Server(char* root_dir,char* str_addr,char* str_port):
        _root_dir(root_dir),
        _str_addr(str_addr),
        _str_port(str_port)
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
    bool QueuePushData(void* cli)
    {
        _login_queue.PushData(cli);
        return true;
    }
    bool QueuePopData(void** cli)
    {
        _login_queue.PopData(cli);
        return true;
    }
    bool QueueIsEmpty()
    {
        return _login_queue.IsEmpty();
    }
    bool QueueIsFull()
    {
        return _login_queue.IsFull();
    }
};

class ClientInfo
{
//登陆验证模块，以及接受客户端文件的主体模块
private:
    int _cur_statu;     //客户端状态
    int _file_fd;       //当前操作文件的描述符
    uint64_t _cur_size; //文件总大小
    LoginInfo _user;    //用户信息
    FileInfo _file;     //文件信息
    TcpSocket _sock;    //套接字信息
    string _root_dir;   //监听的路径
    bool RecvComReq()
    {
        return true;
    }
    bool CreateDir()
    {
        //在服务器创建路径
        char tmp[MAX_PATH] = {0};
        sprintf(tmp,"%s/%s",_root_dir.c_str(),_user.GetName());
        printf("Create dir: %s\n",tmp);
        madir(tmp,0777);
        return true;
    }
    bool OpenFile()
    {
        char tmp[MAX_PATH] = {0};
        sprintf(tmp,"%s/%s/%s",_root_dir.c_str(),_user.GetName(),_file.GetName());
        _file_fd = open(tmp,O_CREAT|O_WRONLY|O_TRUNC,0664);
        if(_file_fd < 0)
        {
            printf("open file:%s error\n",tmp);
            return false;
        }
        return true;
    }
    bool WriteFile(void* buff,int len)
    {
        if(write(_file_fd,buff,len) < 0)
        {
            return false;
        }
        return true;
    }
    bool CloseFile()
    {
        if(_file_fd >= 0)
        {
            close(_file_fd);
            _file_fd = -1;
        }
        return true;
    }
public:
    ClientInfo(int sockfd):
        _sock(sockfd),_cur_statu(N_RCVLOGIN),_cur_size(0),_file_fd(-1)
    {}
    ClientInfo(int sockfd,string path):
        _sock(sockfd),_cur_statu(N_RCVLOGIN),_cur_size(0),
        _file_fd(-1),_root_dir(path)
    {}
    int GetFd()
    {
        return _sock.GetFd();
    }
    bool Stop()
    {
        CloseFile();
        _sock.Close();
    }
    bool CheckLogin()
    {
        //接收对端的登陆验证，并且用户信息存在user里
        //并且为该用户在服务器建立新目录
        CommReq req;
        //同步状态
        if(_sock.Recv(&req,sizeof(CommReq)) < 0)
        {
            printf("recv com req error\n");
            return false;
        }
        printf("recv com req\n");
        //改变当前状态，改为接受状态 
        if(!req.IsLoginReq())
        {
            printf("not login req,close\n");
            return false;
        }
        //接收登陆信息
        if(_sock.Recv(&_user,sizeof(LoginInfo)) < 0)
        {
            printf("recv user info error\n");
            return false;
        }
        printf("new user login:[%s]\n",_user.GetName());
        //为将要接收的文件创建目录
        if(!CreateDir())
        {
            printf("create client dir error\n");
            return false;
        }
        commReq rsp(RSP_LOGIN,0);
        //发送接收响应
        if(!_sock.Send((void*)&rsp,sizeof(CommReq)));
        {
            printf("send login rsp error\n");
            return false;
        }
        _cur_statu = N_RCVHDR;
        return true;
    }
    bool RecvFileHdr()
    {
        CommReq req;
        //建立验证连接
        if(_sock.Recv(&req,sizeof(CommReq)) < 0)
        {
            printf("recv com req error\n");
            return false;
        }
        //改变当前状态，改为接收协议状态
        if(!req.IsUpLoadReq())
        {
            printf("not up load req ,close\n");
            return false;
        }
        //接收头部信息并且存放在_file里
        if(_sock.Recv(&_file,sizeof(FileInfo)) < 0)
        {
            printf("recv file info error\n");
            return false;
        }
        printf("client upload filr:%s\n",_file.GetName());
        CHECK_RET(OpenFile());
        _cur_statu = N_RCVBODY;
        return true;
    }
    bool RecvFileBody()
    {
        char buff[1024];
        int rlen = 1024;
        if((_file.GetLen()-_cur_size)<1024)
        {
            rlen = _file.GetLen()-_cur_size;
        }
        CHECK_ERR((rlen = _sock.Recv(buff,1024)));
        CHECK_RET(WriteFile(buff,rlen));
        if(_cur_size == _file.GetLen())
        {
            _cur_statu = N_RCVOVER;
        }
        return true;
    }
    bool RecvFileOver()
    {
        CommReq rsp(RSP_UFILE,0);
        CHECK_RET(_sock.Send((void*)&rsp,sizeof(CommReq)));
        _cur_statu = N_RCVHDR;
        _cur_size = 0;
        CloseFile();
        return true;
    }
    bool ClientIsTransOver()
    {
        return (_cur_statu == N_RCVOVER);
    }
    bool Run()
    {
        switch(_cur_statu)
        {
            case N_RCVLOGIN:
                CHECK_RET(CheckLogin());
                break;
            case N_RCVHDR:
                CHECK_RET(RecvFileHdr());
                break;
            case N_RCVBODY:
                CHECK_RET(RecvFileBody());
            case N_RCVOVER:
                CHECK_RET(RecvFileOver());
                break;
            default:
                printf("cur client trans statu error -%d\n",_cur_statu);
                return false;
        }
        return true;
    }
};

class Epoll
{
private:
    int _epfd;
public:
    Epoll()
    {
        _epfd = epoll_create(10);
    }
    ~Epoll()
    {
        close(_epfd);
    }
    bool Add(ClientInfo* client)
    {
        int fd = client->GetFd();
        epoll_event ev;
        ev.data.ptr = (void*)client;
        ev.events = EPOLLIN;
        int ret = epoll_ctl(_epfd,EPOLL_CTL_ADD,fd,&ev);
        CHECK_ERR(ret);
        return true;
    }
    bool Del(int sockfd)
    {
        int ret = epoll_ctl(_epfd,EPOLL_CTL_DEL,sockfd,NULL);
        CHECK_ERR(ret);
        return true;
    }
    bool Del(ClientInfo* client)
    {
        int fd = client->GetFd();
        int ret = epoll_ctl(_epfd,EPOLL_CTL_DEL,fd,NULL);
        CHECK_ERR(ret);
        return true;
    }
    bool Wait(vector<ClientInfo*>* list)
    {
        //等待1000毫秒，一直阻塞，没有事件触发就超时
        int i;
        epoll_event evs[MAX_EPOLL];
        int nfds = epoll_wait(_epfd,evs,MAX_EPOLL,1000);
        CHECK_ERR(nfds);
        for(i = 0;i<nfds;i++)
        {
            list->push_back((ClientInfo*)evs[i].data.ptr);
        }
        return true;
    }
};

class LoginServer
{
private:
    string _addr;
    int _port;
    TcpSocket _sock;
    Epoll _epoll;
    Server* _server;
    vector<ClientInfo*> _nlogin_cli;
    string _root_dir;
public:
    LoginServer(Server* srv):_server(srv)
    {
        _addr = srv->GetStrAddr();
        _port = srv->GetStrPort();
        _root_dir = srv->GetRootDir();
    }
    bool Start()
    {
        int i;
        CHECK_RET(_sock.Socket());
        CHECK_RET(_sock.Bind(_addr,_port));
        CHECK_RET(_sock.Listen());
        ClientInfo lst(_sock.GetFd());
        CHECK_RET(_epoll.Add(&lst));
        while(1)
        {
            vector<ClientInfo*> _list;
            if(!_epoll.Wait(&_list))
            {
                continue;
            }
            for(i = 0;i<_list.size();i++)
            {
                //遍历监听队列
                if(_list[i]->GetFd()==_sock.GetFd())
                {
                    //监听套接字入口，监听套接字创建新的工作套接字然后添加到队列
                    TcpSocket sock;
                    if(!_sock.Accept(&sock))
                    {
                        continue;
                    }
                    ClientInfo *client = new ClientInfo(sock.GetFd(),_root_dir);
                    _epoll.Add(client);
                    _nlogin_cli.push_back(client);
                }
                else
                {
                    if(!_list[i]->Run())
                    {
                        _epoll.Del(_list[i]);
                        _list[i]->Stop();
                        for(auto it = _nlogin_cli.begin();it!= _nlogin_cli.end();)
                        {
                            if(*it == _list[i])
                            {
                                printf("login faild delete client\n");
                                _nlogin_cli.erase(it);
                            }
                         }
                        delete _list[i];
                     }
                    else
                    {
                        //从当前epoll删除
                        _epoll.Del(_list[i]);
                        for(auto it = _nlogin_cli.begin();it != _nlogin_cli.end();)
                        {
                            if(*it == _list[i])
                            {
                                printf("delete client from no login vector!!\n");
                                _nlogin_cli.erase(it);
                            }
                        }
                        //将客户端信息添加到队列，传输线程获取后开始传输文件
                        printf("insert client to trans thread\n");
                        _server->QueuePushData((void*)_list[i]);
                    }
                 }
             }
        }
        return true;
    }
};

class TransServer
{
    //服务器接收到文件后，将文件进行备份
private:
    Server* _server;
    Epoll _epoll;
public:
    TransServer(Server *srv):_server(srv)
    {}
    bool Start()
    {
        int i = 0;
        while(1)
        {
            while(!_server->QueueIsEmpty())
            {
                ClientInfo* client;
                _server->QueuePopData(void**)&client;
                _epoll.Add(client);
                printf("new client start treans\n");
            }
            vector<ClientInfo*> _trans_cli;
            if(!epoll.Wait(&_trans_cli))
            {
                continue;
            }
            for(i = 0;i<_trans_cli.size();i++)
            {
                if(!_trans_cli[i]->Run())
                {
                    _epoll.Del(_trans_cli[i]);
                    _trans_cli[i]->Stop();
                    for(auto it = _trans_cli.begin();it!=_trans_cli.end();)
                    {
                        if(*it == _trans_cli[i])
                        {
                            printf("trans file failed delete client\n");
                            delete _trans_cli[i];
                            _trans_cli.erase(it);
                        }
                    }
                }
                if(_trans_cli[i]->ClientIsTransOver())
                {
                    _trans_cli[i]->Run();
                }
            }
         }
        return true;
    }
};

#endif
