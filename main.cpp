#include"main.h"
#include"client.h"
#include"server.h"
#include<pthread>

void* LoginServer(void* arg)
{
    LoginServer server((Server*)arg);
    server.Start();
    return nullptr;
}
void* TransListen(void* arg)
{
    TransServer server((Server*)arg);
    server.Start();
    return nullptr;
}
bool RunServer(InitParam* init)
{
    //这里可以优化成线程池，后面在做优化
    pthread_t tid_l,tid_t;
    Server server(init->GetRootDir(),init->>GetStrAddr(),init->GetStrPort());
    pthread_create(&tid_l,NULL,LoginListen,(void*)&server);
    pthread_create(&tid_t,NULL,TransListen,(void*)&server);
    pthread_join(tid_l,NULL);
    pthread_join(tid_t,NULL);
    return true;
}
void* ScansFile(void* arg)
{
    CScanFile scanfile((Client*)arg);
    scanfile.Start();
    return NULL;
}
void* TransFile(void* arg)
{
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
