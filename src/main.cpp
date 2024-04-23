#include <iostream>
#include <sstream>
#include <random>
#include <thread>
#include "JsonParse/JsonParse.h"
#include "ConfigManager/ConfigManager.h"
#include "Network/Network.h"
#include "Message/Message.h"
#include "Log/Log.h"
#include "MessageQueue.hpp"
#include "src/Network/MyReverseWebSocket.h"
#include "src/Network/MyWebSocket.h"
#include "TimingTast/TimingTast.h"

using namespace std;

ConfigManager &CManager = *new ConfigManager("config.json");
JsonParse &JParsingClass = *new JsonParse;
TimingTast &TTastClass = *new TimingTast;
Message &messageClass = *new Message;
MessageQueue &originalMessageQueue = *new MessageQueue;
MessageQueue &pendingDataQueue = *new MessageQueue;

// 子线程
void pollingThread()
{
	string message;
	string content;
	string JsonFormatData;
	string getGOCQJsonData;
	UINT64 user_id;
	bool tag = false;

	while (true)
	{
		std::time_t now = std::time(nullptr);
		// 转换为本地时间
		std::tm *local_time = std::localtime(&now);
		if (local_time->tm_hour == 8 && local_time->tm_min == 0 && local_time->tm_sec < 4) // 每日定时
		{
			content = "早上好，请跟我打招呼的同时来一句元气满满的句子，让我一整天都有活力（直接说就好，不要在前面加上语气词例如“好的”）";
			user_id = stoi(CManager.configVariable("MANAGER_QQ")); // 当前版本仅限于管理员账号，后期可选择对外提供服务
			tag = true;
		}
		else if (!TTastClass.Event->empty() && TTastClass.Event->begin()->first <= TTastClass.getPresentTime())
		{
			content = JParsingClass.toJson(TTastClass.Event->begin()->second.second);
			user_id = TTastClass.Event->begin()->second.first;
			TTastClass.Event->erase(TTastClass.Event->begin()); // 删除定时事件
			tag = true;
		}

		if (tag)
		{
			// 数据处理&发送
			message = "“";
			message += content;
			JsonFormatData = "[\n\t";
			JsonFormatData += R"({"role": "user", "content": ")" + message + "\"}\n]";
			OpenAI::send_request_chat(JsonFormatData, CManager.configVariable("OPENAI_GPT4"));
			if (JsonFormatData.size() > 100)
			{
				JsonFormatData = JParsingClass.getAttributeFromChoices(JsonFormatData, "content");
			}
			JsonFormatData = JParsingClass.toJson(JsonFormatData);
			getGOCQJsonData = messageClass.privateGOCQFormat(JsonFormatData, user_id);
			// Network::getInstance().setTaskMessage(getGOCQJsonData, CManager.configVariable("PRIVATE_API"));
			pendingDataQueue.push_queue(getGOCQJsonData);
			tag = false;
		}
		sleep(3);
	}
}

void send_message(JsonData &data, bool isErrorTransfer)
{
	// 数据处理 && 封装
	string getCQCodeData;		// 用于接收cq码数据
	string JsonFormatData;		// 用于接收格式化的json数据
	string getGOCQJsonData;		// 用于接收装有cq码的Json数据
	string webSocketDataPakage; // 用于接收websocket的数据包格式

	if (isErrorTransfer)
	{
		if (data.message_type == "group")
		{
			// getCQCodeData = messageClass.atUserMassage(data->message, data->private_id);
			JsonFormatData = JParsingClass.toJson(getCQCodeData);
			getGOCQJsonData = messageClass.gourpGOCQFormat(JsonFormatData, data.group_id);
			webSocketDataPakage = MyReverseWebSocket::messageEncapsulation(getGOCQJsonData, CManager.configVariable("GROUP_API"));
			pendingDataQueue.push_queue(webSocketDataPakage);
		}
		else
		{
			JsonFormatData = JParsingClass.toJson(data.message);
			getGOCQJsonData = messageClass.privateGOCQFormat(JsonFormatData, data.private_id);
			webSocketDataPakage = MyReverseWebSocket::messageEncapsulation(getGOCQJsonData, CManager.configVariable("PRIVATE_API"));
			pendingDataQueue.push_queue(webSocketDataPakage);
		}
	}
	else
	{
		if (strcmp(data.message_type.c_str(), "group") == 0)
		{
			cout << data.private_id << ":" << data.message << endl;
			getCQCodeData = messageClass.handle_Message(data.private_id, data.message);
			// sgetCQCodeData = messageClass.atUserMassage(getCQCodeData, data->private_id);
			JsonFormatData = JParsingClass.toJson(getCQCodeData);
			getGOCQJsonData = messageClass.gourpGOCQFormat(JsonFormatData, data.group_id);
			webSocketDataPakage = MyReverseWebSocket::messageEncapsulation(getGOCQJsonData, CManager.configVariable("GROUP_API"));
			pendingDataQueue.push_queue(webSocketDataPakage);
		}
		else
		{
			cout << data.private_id << ":" << data.message << endl;
			getCQCodeData = messageClass.handle_Message(data.private_id, data.message);
			JsonFormatData = JParsingClass.toJson(getCQCodeData);
			getGOCQJsonData = messageClass.privateGOCQFormat(JsonFormatData, data.private_id);
			webSocketDataPakage = MyReverseWebSocket::messageEncapsulation(getGOCQJsonData, CManager.configVariable("PRIVATE_API"));
			pendingDataQueue.push_queue(webSocketDataPakage);
		}
	}
}

void workingThread(string originalJsonData)
{
	string getSendHTTP_Package; // 用于发送获取HTTP包，并且发送数据

	// json数据提取
	try
	{
		originalJsonData.erase(0, originalJsonData.find_first_of('{'));
		originalJsonData.erase(originalJsonData.find_last_not_of('}'), originalJsonData.size());
	}
	catch (const std::exception &e)
	{
		LOG_ERROR(e.what());
		return;
	}

	// 当受到的内容非聊天消息时，丢弃消息
	if (originalJsonData.substr(originalJsonData.find(":") + 2, 7) != "message")
	{
		return;
	}

	// 内容提取
	JsonData *data = new JsonData(JParsingClass.jsonReader(originalJsonData));

	// 超出最大限度
	if (originalJsonData.size() > stoi(CManager.configVariable("OPENAI_SIGLE_TOKEN_MAX")))
	{
		data->message = "系统提示：消息长度超过最大限度，请减少单次发送的字符数量...";
		send_message(*data, true);

		delete data;
		return;
	}

	// 其他内容过滤
	if (!messageClass.messageFilter(data->message_type, data->message))
	{
		return;
	}

	// 正常发送
	send_message(*data, false);

	delete data;
}

void createTimingTastThread()
{
	// 创建子线程
	thread t(pollingThread);
	t.detach(); // 线程分离
	return;
}

int connectGOCQ()
{
	/*
	// 设置服务器的IP地址和端口
	string host = CManager.configVariable("REPORTUP_MESSAGE_IP");
	string port = CManager.configVariable("REPORTUP_MESSAGE_PORT");


	将websocket分离，这里使用以下逻辑：
	1.将下面代码模块化，函数命名为“establishWebSocketConnection”；
	2.函数内部必须高度简洁，保证速度；
	3.函数为子线程并且一直执行，不存在退出条件；
	4.函数接收内容时，存放在公共消息队列



	try
	{
		// 创建IO上下文
		io_context ioc;

		// 从IO上下文创建WebSocket流
		websocket::stream<tcp::socket> ws(ioc);

		// 解析服务器地址和端口
		tcp::resolver resolver(ioc);
		auto const results = resolver.resolve(host, port);

		// 连接到服务器
		connect(ws.next_layer(), results.begin(), results.end());

		// 握手以升级到WebSocket连接
		ws.handshake(host, "/");

		LOG_INFO("已连接至GO-CQHTTP...");

		// 持续监听消息
		while (true)
		{
			// 准备接收消息的缓冲区
			multi_buffer buffer;

			// 读取消息到缓冲区
			ws.read(buffer);

			// 将数据放到string中
			std::string message = boost::beast::buffers_to_string(buffer.data());
#ifdef DEBUG
			// std::cout << "原始数据：" << message << std::endl;
			LOG_INFO("原始数据：" + message);
#endif

			// 创建子线程
			thread t(workingThread, message);
			t.detach(); // 线程分离
		}
	}
	catch (std::exception const &e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
	*/

	return EXIT_SUCCESS;
}

// 资源释放
void resourceCleanup()
{
	if (&CManager != nullptr)
	{
		delete &CManager;
	}

	if (&JParsingClass != nullptr)
	{
		delete &JParsingClass;
	}

	if (&TTastClass != nullptr)
	{
		delete &TTastClass;
	}

	if (&messageClass != nullptr)
	{
		delete &messageClass;
	}

	if (&originalMessageQueue != nullptr)
	{
		delete &originalMessageQueue;
	}

	if (&pendingDataQueue != nullptr)
	{
		delete &pendingDataQueue;
	}
}

int main()
{
	Database::getInstance()->databaseEmpty();
	createTimingTastThread(); // 创建子线程

	// 连接至GOCQ

	// 正向WebSocket连接
	std::thread t1(MyWebSocket::connectWebSocket, "/");
	t1.detach();

	sleep(1);
	LOG_INFO("3秒后连接反向WebSocket...");
	sleep(3);

	// 反向WebSocket连接
	std::thread t2(MyReverseWebSocket::connectReverseWebSocket);
	t2.detach();

	// 轮询originalMessageQueue
	while (true)
	{
		if (!originalMessageQueue.empty())
		{
			std::thread t(workingThread, originalMessageQueue.front_queue());
			t.detach();
			originalMessageQueue.pop();
		}
		else
		{
			// 休眠算法...
			sleep(1);
		}
	}
	/*
	while (true)
	{
		try
		{
			connectGOCQ();
		}
		catch (const std::exception &e)
		{
			std::cerr << "捕获到异常: " << e.what() << std::endl;
			LOG_FATAL("连接至GO-CQHTTP时出现错误，正在重新连接...");
		}
		sleep(1); // 延迟一秒
	}
	*/

	// 判断是否退出 ...

	// 资源回收
	resourceCleanup();

	return 0;
}