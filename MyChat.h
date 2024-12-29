#ifndef MY_CHAT_H
#define MY_CHAT_H
#include <winsock2.h>
#include <iostream>
#include <chrono>
#include <vector>


enum MessageType {
	TEXT = 1,
	COMMAND,
	FILEE
};

enum MessageColor {
	BLUE = 9,
	GREEN,
	LIGHT_BLUE,
	RED,
	PINCK,
	YELLOW,
	WHITE
};



typedef struct NameAndSock {
	std::string name;
	SOCKET sock;
};


void setColor(int textColor);


class Message {
public:
	Message(std::string sender, std::string content, std::chrono::time_point<std::chrono::system_clock> timestamp, MessageType type, MessageColor color);
	std::string getSender();
	std::string getContent();
	std::chrono::time_point<std::chrono::system_clock> getTimeStamp();
	MessageType getType();
	MessageColor getColor();

private:
	std::string sender;
	std::string content;
	std::chrono::time_point<std::chrono::system_clock> timestamp;
	MessageType type;
	MessageColor color;
};



void sendMessage(SOCKET socket, Message& msg);
Message reciveMessage(SOCKET socket);

class Client {
public:
	Client(std::string ipAddress, int portNumber);
	~Client();
	void runClient();
	void receiveMsg();
	void sendMsg();
	void getInput();
	void getSenderName();
	void sendFile();
	void recvFile(Message &msg);
	std::string getFileName(std::string filePath);
	bool checkIfCommad();
	void handelCommand();
	void showUserInstructions();
	void getUserColor();

private:
	WSADATA wsaData;
	SOCKET clientSocket;
	sockaddr_in serverAddr;
	std::string input;
	std::string senderName;
	MessageColor color;
};

class Server {
public:
	Server(int portNumber);
	~Server();
	
	bool checkIfFile(Message &msg);
	void reciveAndSendFile(fd_set master, SOCKET sendingSocket);
	bool checkIfCommand(Message& msg);
	void handaleCommand(SOCKET sock, Message& msg1);
	bool checkUser(std::string name);
	void acceptclients();
	void handelMessages();
	void runServer();
	void valideteClientName(SOCKET client);
	/*
	void valideteClientName(SOCKET client);
	
	void acceptNewClient();
	*/

private:
	WSADATA wsaData;
	SOCKET serverSocket;
	sockaddr_in serverAddr;
	std::vector<NameAndSock> clients;
	fd_set master;
};



class Serializer {
public:
	static std::vector<char> serialize(Message msg);
	static Message deserialize(std::vector<char> msg);
	static void intToBinary(int inputNumber, std::vector<char>& vec, int numberSize);
	static void stringToBinary(const std::string& inputString, std::vector<char>& vec);
	static std::string binaryTostring(std::vector<char>& vec, int lenOfString, int index);
	static int binaryToInt(std::vector<char>& vec, int index, int numberSize);
};

#endif 
