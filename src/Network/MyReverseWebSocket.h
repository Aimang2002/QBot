#ifndef MYREVERSEWEBSOCKET_H
#define MYREVERSEWEBSOCKET_H
#include "WebSocketHead.h"

extern ConfigManager &CManager;
extern MessageQueue &originalMessageQueue;
extern MessageQueue &pendingDataQueue;

class MyReverseWebSocket
{
public:
    static void connectReverseWebSocket();
    static std::string messageEncapsulation(const string message, const string messageEndpoint);

private:
};

#endif // REVERSEWEBSOCKET_H