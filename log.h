#ifndef __LOG_H__
#define __LOG_H__

#include<iostream>
#include<string>
#include<sys/time.h>

#define INFO 0 
#define ERROR 1

uint64_t GetTimeStamp()
{
    struct timeval _time;
    gettimeofday(&_time,NULL);
    return _time.tv_sec;
}

string GetLogLevel(int _level)
{
    switch(_level)
    {
        //这里用switch是因为也许以后会补充级别
        //保持可扩展性
        case 0:
            return "INFO";
        case 1:
            return "ERROR";
        default:
            return "UNKNOW";
    }
}
void LOG(int _level,string _massage,string _file,int _line)
{
    cout<<"[ "<<GetTimeStamp() << " ]"/
        <<" [ "<<GetLogLevel(_level)<<" ]"/
        <<" [ "<<_file<<" : "<<_line<<" ] "<<_massage<<endl;
}
#define Log(_level,_massage) LOG(_level,_massage,__FILE__,__LINE__)

#endif
