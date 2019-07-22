#pragma once

#pragma comment(lib, "ws2_32.lib")

#include "utils.h"
#include "define.h"
#include "Packet.h"
#include <WinSock2.h>
#include <Windows.h>
#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <memory>
#include <thread>
#include <numeric>
#include <chrono>
#include <concurrent_queue.h>

struct Overlap_Exp
{
	enum Overlap_Operation {
		OPERATION_RECV = 1
		, OPERATION_SEND
	};

	::WSAOVERLAPPED Original_Overlap;
	int Operation;
	size_t Serial;
	WSABUF WsaBuf;
	unsigned char Iocp_Buffer[BUF_SIZE];

	Overlap_Exp()
		: Operation(0)
		, Serial((std::numeric_limits<size_t>::max)())
	{
		::ZeroMemory(&Original_Overlap, sizeof(Original_Overlap));
		::ZeroMemory(Iocp_Buffer, sizeof(Iocp_Buffer));

		WsaBuf.buf = reinterpret_cast<CHAR*>(Iocp_Buffer);
		WsaBuf.len = sizeof(Iocp_Buffer);
	}
};

struct Client
{
	size_t				Serial;
	SOCKET				ClientSocket;
	bool				IsConnect;
	Overlap_Exp			RecvOverlap;

	int					PacketSize;
	int					SavedPacketSize;
	unsigned char		PacketBuff[BUF_SIZE];

	std::mutex			clientMutex;
	std::string			UserName;
	unsigned short		CharCode;

	Client()
		: Serial((std::numeric_limits<size_t>::max)())
		, ClientSocket(INVALID_SOCKET)
		, IsConnect(false)
		, PacketSize(0)
		, SavedPacketSize(0)
	{
		::ZeroMemory(PacketBuff, sizeof(PacketBuff));
	}
};

/**
	@todo	DBConn, AcceptEx, GQCSEx, send overlap, 패킷조립, 코드 정리 등
*/
class Framework
	: public TemplateSingleton<Framework>
{
public:
	friend class TemplateSingleton<Framework>;
	using SERIAL_TYPE = unsigned long long;
	using PacketSize = decltype(NCommon::PktHeader::TotalSize);

	static const SERIAL_TYPE					SERIAL_ERROR = (std::numeric_limits<SERIAL_TYPE>::max)();
	static const unsigned int					GQCS_TIMEOUT_MILLISECONDS = 3000;
	static const unsigned int					ACCEPT_TIMEOUT_SECONDS = 3;

	enum class SERIAL_GET_TYPE {
		CLIENT = 0
		, OVERLAPEXP
	};

private:
	using Packet_Procedure = void(Framework::*)(SERIAL_TYPE, unsigned char*, PacketSize);
	using Serial_ConcurrentQueue = concurrency::concurrent_queue<SERIAL_TYPE>;

	static const unsigned int					NUM_WORKER_THREADS = 16;
	static const unsigned int					MAX_CLIENT_COUNT = 3000;
	static const unsigned int					MAX_OVERLAPEXP_COUNT = 80000;
	static const unsigned int					CONCURRENT_TIMEOUT_MILLISECONDS = 3000;

public:
	bool			IsShutDown() const { return isShutDown; }
	bool			IsValidClientSerial(SERIAL_TYPE serial) const { return (serial < MAX_CLIENT_COUNT) ? true : false; }

	HANDLE			GetIocpHandle() const { return hIocp; }
	SERIAL_TYPE		GetSerialForNewOne(SERIAL_GET_TYPE type);
	Client&			GetClient(SERIAL_TYPE serial) { return *clients[serial]; }
	
	void			ReturnUsedOverlapExp(SERIAL_TYPE serial);

	void			SendPacket(SERIAL_TYPE serial, const void* packet, PacketSize size);
	void			ProcessUserClose(SERIAL_TYPE serial);	//disconnect
	void			ProcessPacket(SERIAL_TYPE serial, unsigned char* packet, PacketSize size);

private:
	Framework();
	virtual ~Framework();

	bool Initialize();
	void ShutDown();

	void		Process_Login(SERIAL_TYPE, unsigned char*, PacketSize);
	void		Process_SelectCharacter(SERIAL_TYPE, unsigned char*, PacketSize);

private:
	HANDLE		hIocp;
	bool		isShutDown;

	std::vector<std::unique_ptr<std::thread>>		workerThreads;
	std::unique_ptr<std::thread>					acceptThread;

	std::vector<std::unique_ptr<Client>>			clients;
	std::vector<std::unique_ptr<Overlap_Exp>>		sendOverlapExps;

	Serial_ConcurrentQueue							validClientSerials;
	Serial_ConcurrentQueue							validOverlapExpSerials;

	std::map<decltype(NCommon::PktHeader::Id) /*type*/, Packet_Procedure>		packetProcedures;
};