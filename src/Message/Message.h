#include <iostream>
#include <fstream>
#include <string>
#include <random>
#include <sstream>
#include <map>
#include <vector>
#include <string>
#include <cstring>
#include <thread>
#include <ctime>
#include <iomanip>
#include "../TimingTast/TimingTast.h"
#include "../Database/Database.h"
#include "../JsonParse/JsonParse.h"
#include "../ConfigManager/ConfigManager.h"
#include "../ComputerStatus/ComputerStatus.h"
#include "OpenAI.hpp"

using namespace std;

extern ConfigManager &CManager;
extern JsonParse &JParsingClass;
extern TimingTast &TTastClass;

// 用户类
class Person
{
public:
	vector<pair<string, time_t>> user_chatHistory; // 用户聊天信息和时间戳
	string user_models;							   // 用户使用的模型
	bool isOpenVoiceMode = false;				   // 是否开启语音模式，默认为否
};

// 消息类
class Message
{
public:
	Message();

	/**
	 * @brief 消息过滤，对某些消息进行过滤
	 *
	 * @param param1 	消息类型
	 * @param param2	具体消息
	 *
	 */
	bool messageFilter(string message_type, string message);

	/**
	 * @brief 处理私聊消息
	 *
	 * @param param1 	用户QQ
	 * @param param2 	该用户所发送的信息
	 */
	string handle_Message(const UINT64 private_id, string message);

	/**
	 * @brief 封装go-cqhttp私聊消息
	 *
	 * @param param1 	具体消息
	 * @param param2 	用户QQ
	 *
	 */
	string privateGOCQFormat(string message, const UINT64 user_id);

	/**
	 * @brief 封装go-cqhttp群聊格式的数据报
	 *
	 * @param param1 	具体消息
	 * @param param2 	群号
	 *
	 */
	string gourpGOCQFormat(string message, const UINT64 group_id);

	~Message();

private:
	/**
	 * @brief 添加用户，用于添加新的用户
	 *
	 * @param param1 	用户QQ
	 * @return 			若返回true则表示创建成功
	 */
	bool addUsers(UINT64 user_id);

	/**
	 * @brief 添加用户
	 *
	 * @param param1 	用户QQ
	 *
	 */
	void questPictureID(string &message);

	/**
	 * @brief 语音，发送语音
	 *
	 * @param param1 	具体消息
	 *
	 */
	void SpeechSound(string &message);

	/**
	 * @brief 个性化聊天（对接人工智能模块）
	 *
	 * @param param1 	具体消息
	 * @param param2	用户QQ
	 *
	 */
	void characterMessage(UINT64 &user_id, string &message);

	/**
	 * @brief 音乐分享
	 *
	 * @param param1 	具体消息
	 * @param param2 	音乐来自哪个平台，1为网易云...
	 *
	 */
	void musicShareMessage(string &message, short platform);

	/**
	 * @brief 表情包
	 *
	 * @param param2	具体消息
	 *
	 */
	void facePackageMessage(string &message);

	/**
	 * @brief 艾特群友
	 *
	 * @param param1 	具体消息
	 * @param param2 	群友QQ
	 *
	 */
	string atUserMassage(string message, const UINT64 user_id);

	/**
	 * @brief 艾特所有人
	 *
	 * @param param1 	具体消息
	 *
	 */
	void atAllMessage(string &message);

	/**
	 * @brief 设置人格,可设置不同人格
	 *
	 * @param param1 	人格名称
	 * @param param2 	用户QQ
	 *
	 */
	void setPersonality(string &roleName, const UINT64 user_id);

	/**
	 * @brief 设置人格(重载版本)
	 *
	 * @param param1 	人格名称
	 * @param param2 	用户QQ
	 * @param param2	int类型占位符
	 *
	 */
	void setPersonality(string &roleName, const UINT64 user_id, int);

	/**
	 * @brief 重置对话，将会清空所有上下文对话
	 *
	 * @param param1 	重置结果
	 * @param param2 	用户QQ
	 *
	 */
	void resetChat(string &resetResult, UINT64 user_id);

	/**
	 * @brief 管理员终端，设置管理员命令
	 *
	 * @param param1 	具体消息
	 * @param param2 	用户QQ
	 *
	 */
	bool adminTerminal(string &message, const UINT64 user_id);

	/**
	 * @brief 管理员权限验证
	 *
	 * @param param1 	管理员QQ
	 *
	 */
	bool permissionVerification(const UINT64 user_id);

	/**
	 * @brief 切换人工智能模型
	 *
	 * @param param1 	具体消息
	 * @param param2 	用户QQ
	 *
	 */
	void switchModel(string &message, const UINT64 user_id);

	/**
	 * @brief 对用户发来的图片进行4K修复
	 *
	 * @param param1 	具体消息
	 * @return 			当修复成功则返回true
	 *
	 */
	bool fixImageSizeTo4K(string &message);

	/**
	 * @brief 调用GPT4-VISION模型
	 *
	 * @param param1 	具体消息
	 *
	 */
	bool provideImageRecognition(const UINT64 user_id, string &message);

	/**
	 * @brief 将传入进去的数据转为bash64编码，最后data保存base64编码
	 *
	 * @param param1 	数据流
	 *@return 			返回处理完毕后的base64编码
	 */
	string dataToBase64(const string &input);

	/**
	 * @brief 将传入进去的文本转为URL编码
	 *
	 * @param param1 	文本数据
	 *@return 			返回处理完毕后的URL编码
	 */
	string encodeToURL(const string &input);

	/**
	 * @brief 将文本转为语音
	 *
	 * @param param1 	文本
	 */
	void textToVoice(string &text);

	/**
	 * @brief 使用dall-e-3模型生成图片
	 *
	 * @param param1 	文本
	 */
	bool provideImageCreation(const UINT64 user_id, string &text);

	// 在下面添加新的函数用于拓展其他内容...

private:
	string help_message;
	string default_personality;
	string users_message_format;
	string bot_message_format;
	string system_message_format;
	short default_message_line;
	bool accessibility_chat; // true为开启
	bool global_Voice;		 // true为开启
	vector<std::pair<string, string>> PersonalityList;
	vector<std::pair<string, string>> LightweightPersonalityList;

private:
	map<UINT64, Person> *user_messages; // key = QQ
	std::mutex mutex_message;
	ComputerStatus *PCStatus;
};
