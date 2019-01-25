#include"main.h"
#include"client.h"
#include"server.h"
#include"ThreadPool.h"
#include<pthread>

void* LoginListen(void* arg)
{
    //登陆服务
    LoginServer server((Server*)arg);
    server.Start();
    return nullptr;
}
void* TransListen(void* arg)
{
    //传输服务
    TransServer server((Server*)arg);
    server.Start();
    return nullptr;
}
void* ThreadHandler(InitParam* init)
{
    Server _server(init->GetRootDir(),init->GetStrAddr(),GetStrPort());
    LoginListen(&_server);
    TransListen(&_server);
}
bool RunServer(InitParam* init)
{
    ThreadPool* tp;
    tp->InitThreadPool();
    Task t;
    t.SetTask(init,ThreadHandler);
    tp.PushTask(t);
    return true;
}
void* ScansFile(void* arg)
{
    //监听服务
    CScanFile scanfile((Client*)arg);
    scanfile.Start();
    return NULL;
}
void* TransFile(void* arg)
{
    //传输服务
    CTransFile((Client*)arg);
    transfile.Start();
    return NULL;
}
bool RunClient(InitParam* init)
{
    Client client(init->GetRootDir(),init->GetStrAddr(),\
                  init->GetStrPort(),init->GetUserName());
    pthread_t tid_s,tid_t;
    pthread_create(&tid_s,NULL,ScansFile,(void*)&client);
    pthread_create(&tid_t,NULL,TransFile,(void*)&client);
    pthread_join(tid_s,NULL);
    pthread_join(tid_t,NULL);
    return true;
}
int main(int argc,char* argv[])
{
    InitParma init(argc,argv);
    CHECK_RET(init.Init());
    init.Parmaprint();
    if(init.ModeIsServer())
    {
        CHECK_RET(RunServer(&init));
    }
    else
    {
        CHECK_RET(RunClient(&init));
    }
    return 0;
}
