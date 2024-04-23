#ifndef MESSAGETASK_H
#define MESSAGETASK_H

#include <iostream>
#include <queue>
#include <string>
#include <mutex>
#include "src/Log/Log.h"

class MessageQueue
{
public:
    MessageQueue()
    {
        this->Que = new std::queue<std::string>;
    }

    void push_queue(std::string task)
    {
#ifdef DEBUG
        cout << "入列消息：" << task << endl;
#endif
        std::lock_guard(this->_mutex);
        this->Que->push(task);
    }

    inline bool empty()
    {
        return this->Que->empty();
    }

    std::string front_queue()
    {
        if (this->Que->empty())
        {
            return "当前task为空!请判断队列是否存在数据再获取...";
        }

        std::string result;
        std::lock_guard(this->_mutex);
        result = this->Que->front();
        return result;
    }

    bool pop()
    {
        if (this->Que->empty())
        {
            LOG_WARNING("发送队列为空！或许是程序出了问题，请检查...");
            return false;
        }

        std::lock_guard(this->_mutex);
        this->Que->pop();
        return true;
    }

    ~MessageQueue()
    {
        if (this->Que != nullptr)
        {
            delete this->Que;
            this->Que = nullptr;
        }
    }

private:
    std::queue<std::string> *Que;
    std::mutex _mutex;
};

#endif // MESSAGETASK_H
