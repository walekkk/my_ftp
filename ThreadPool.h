#ifndef _THREAD_POOL_H_
#define _THREAD_POOL_H_

#define NUM 5

#include<iostream>
#include<queue>
#include<pthread.h>
#include"log.h"

typedef int (*handler_t)(void*);
class Task
{
private:
    void* _parma;
    handler_t _handler;
public:
    Task():_parma(NULL),_handldr(NULL)
    {}
    void SetTask(int parma,handler_t handler)
    {
        _parma = parma;
        _handldr = handler;
    }
    void Run()
    {
        _handldr(_sock);
    }
};

class ThreadPool
{
private:
    int _thread_total_num;  //线程池中线程总数
    int _thread_idle_num;   //线程池中空闲线程数
    queue<Task> _task_queue;//任务队列
    pthread_mutex_t _lock;  
    pthread_cond_t _cond;
    volatile bool _is_quit;
public:
    void LockQueue()
    {
        pthread_mutex_lock(&_lock);
    }
    void UnlockQueue()
    {
        pthread_mutex_unlock(&_lock);
    }
    bool IsEmpty()
    {
        return _task_queue.size()==0;
    }
    void ThreadIdle()
    {
        if(_is_quit)
        {
            //如果要关闭线程池了
            //这个就退出
            //防止死锁
            UnlockQueue();
            _thread_total_num--;
            log(INFO,"thread quit!");
            pthread_exit((void*)0);
        }
        //由于任务队列为空，所以这个线程变为空闲线程
        _thread_idle_num++;
        pthread_cond_wait(&_cond,&_lock);
        //空闲队列等待在条件变量
        //往下走的条件是被唤醒
        _thread_idle_num--;

    }
    void WakeUpOneThread()
    {
        pthread_cond_wait(&_cond,&_lock);
    }
    void WakeUpAllThread()
    {
        pthread_cond_broadcast(&cond);
    }
    static void* thread_routine(void* arg)
    {
        //工作线程入口
        ThreadPool* tp = (ThreadPool*)arg;
        pthread_detach(pthread_self());
        for( ; ; )
        {
            tp->LockQueue();
            while(tp->IsEmpty())
            {
                tp->ThreadIdle();
            }
            Task t;
            tp->PopTask(t);
            tp->UnlockQueue();
            log(INFO,"task has be tacked");
            cout<<"thread id is : "<<pthread_self()<<endl;
            t.Run();
        }
    }
public:
    ThreadPool(int num = NUM):_thread_total_num(num),_thread_idle_num(0),
                              _is_quit(false)
    {
        pthread_mutex_init(&_lock,NULL);
        pthread_cond_init(&_cond,NULL);
    }
    void InitThreadPool()
    {
        for(int i=0;i<_thread_total_num;i++)
        {
            pthread_t tid;
            pthread_create(&tid,NULL,thread_routine,this);
        }
    }
    void PushTask(Task& _t)
    {
        LockQueue();
        if(_is_quit)
        {
            UnlockQueue();
            return;
        }
        _task_queue.push(_t);
        WakeUpOneThread();
        UnlockQueue();
    }
    void PopTask(Task& _t)
    {
        _t = _task_queue.front();
        _task_queue.pop();
    }
    void Stop();
    {
        LockQueue();
        _is_quit = true;
        UnlockQueue();
        while(_thread_idle_num>0)
        {
            WakeUpAllThread();
        }
    }
    ~ThreadPool()
    {
        pthread_mutex_destory(&_lock);
        pthread_cond_destory(&_cond);
    }
}
#endif
