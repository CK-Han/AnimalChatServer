#include "framework.h"

namespace
{
	void AcceptThreadStart();
	void WorkerThreadStart();

	// TODO buf 유효성 검사(PacketHeaderSize 만큼 유효한지)
	decltype(NCommon::PktHeader::TotalSize) GetPacketSize(const void* buf)
	{
		if (buf == nullptr)	return 0;

		NCommon::PktHeader header{ 0 };
		std::memcpy(&header, buf, sizeof(header));

		return header.TotalSize;
	}

	// TODO buf 유효성 검사(PacketHeaderSize 만큼 유효한지)
	decltype(NCommon::PktHeader::Id) GetPacketId(const void* buf)
	{
		if (buf == nullptr)	return 0;

		NCommon::PktHeader header{ 0 };
		std::memcpy(&header, buf, sizeof(header));

		return header.Id;
	}

} //unnamed namespace

Framework::Framework()
{
	WSADATA	wsadata;
	::WSAStartup(MAKEWORD(2, 2), &wsadata);

	hIocp = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
	isShutDown = !Initialize();
}

Framework::~Framework()
{
	ShutDown();
}

bool Framework::Initialize()
{
	try
	{
		clients.reserve(MAX_CLIENT_COUNT);
		for (auto i = 0; i < MAX_CLIENT_COUNT; ++i)
			clients.emplace_back(new Client);

		sendOverlapExps.reserve(MAX_OVERLAPEXP_COUNT);
		for (auto i = 0; i < MAX_OVERLAPEXP_COUNT; ++i)
			sendOverlapExps.emplace_back(new Overlap_Exp);
	}
	catch (const std::bad_alloc&)
	{
		std::cout << "Initialize() - bad_alloc\n";
		return false;
	}

	for (auto i = 0; i < MAX_CLIENT_COUNT; ++i)
		validClientSerials.push(i);

	for (auto i = 0; i < MAX_OVERLAPEXP_COUNT; ++i)
		validOverlapExpSerials.push(i);

	packetProcedures[(decltype(NCommon::PktHeader::Id))NCommon::PACKET_ID::LOGIN_IN_REQ] = &Framework::Process_Login;
	packetProcedures[(decltype(NCommon::PktHeader::Id))NCommon::PACKET_ID::SEL_CHARECTER_REQ] = &Framework::Process_SelectCharacter;
	

	try
	{
		for (auto i = 0; i < NUM_WORKER_THREADS; ++i)
			workerThreads.emplace_back(new std::thread(WorkerThreadStart));

		acceptThread = std::unique_ptr<std::thread>(new std::thread(AcceptThreadStart));
	}
	catch (const std::bad_alloc&)
	{
		std::cout << "Initialize() - bad_alloc\n";
		return false;
	}

	return true;
}

void Framework::ShutDown()
{
	isShutDown = true;

	for (auto& th : workerThreads)
		th->join();
	acceptThread->join();

	::WSACleanup();
}

Framework::SERIAL_TYPE Framework::GetSerialForNewOne(SERIAL_GET_TYPE type)
{
	SERIAL_TYPE retSerial = SERIAL_ERROR;
	unsigned int timeout_milliseconds = CONCURRENT_TIMEOUT_MILLISECONDS;
	Serial_ConcurrentQueue* serialQueue = nullptr;

	switch (type)
	{
	case SERIAL_GET_TYPE::CLIENT:
		serialQueue = &validClientSerials;
		break;
	case SERIAL_GET_TYPE::OVERLAPEXP:
		serialQueue = &validOverlapExpSerials;
		break;
	default:
		std::cout << "GetSerialForNewOne() - invalid type\n";
		return retSerial;	//SERIAL_ERROR
	}

	using namespace std::chrono;

	auto beginTime = high_resolution_clock::now();
	while (false == serialQueue->try_pop(retSerial))
	{
		auto elapsedTime = duration_cast<milliseconds>(high_resolution_clock::now() - beginTime);
		if (timeout_milliseconds <= elapsedTime.count())
			return SERIAL_ERROR;
	}

	return retSerial;
}

void Framework::ReturnUsedOverlapExp(SERIAL_TYPE serial)
{
	if (MAX_OVERLAPEXP_COUNT <= serial) 
		return;

	validOverlapExpSerials.push(serial);
}

void Framework::SendPacket(Framework::SERIAL_TYPE serial, const void* packet, PacketSize size)
{
	if (IsValidClientSerial(serial) == false
		|| packet == nullptr) return;

	auto& client = clients[serial];
	if (client->IsConnect == false) return;

	SERIAL_TYPE overlapSerial = GetSerialForNewOne(SERIAL_GET_TYPE::OVERLAPEXP);
	if (SERIAL_ERROR == overlapSerial)
	{
		std::cout << "SendPacket() - GetSerialForUseOverlapExp timeout\n";
		return;
	}

	::ZeroMemory(&(*sendOverlapExps[overlapSerial]), sizeof(Overlap_Exp));
	sendOverlapExps[overlapSerial]->Serial = overlapSerial;
	sendOverlapExps[overlapSerial]->Operation = Overlap_Exp::OPERATION_SEND;
	sendOverlapExps[overlapSerial]->WsaBuf.buf = reinterpret_cast<CHAR *>(sendOverlapExps[overlapSerial]->Iocp_Buffer);
	sendOverlapExps[overlapSerial]->WsaBuf.len = size;
	std::memcpy(sendOverlapExps[overlapSerial]->Iocp_Buffer, packet, sendOverlapExps[overlapSerial]->WsaBuf.len);

	int ret = ::WSASend(client->ClientSocket, &sendOverlapExps[overlapSerial]->WsaBuf, 1, NULL, 0,
		&sendOverlapExps[overlapSerial]->Original_Overlap, NULL);

	if (0 != ret)
	{
		int error_no = ::WSAGetLastError();
		if (WSA_IO_PENDING != error_no
			&& WSAECONNRESET != error_no
			&& WSAECONNABORTED != error_no)
			std::cout << "SendPacket::WSASend Error : " << error_no << std::endl;
	}
}

void Framework::ProcessUserClose(SERIAL_TYPE serial)
{
	if (IsValidClientSerial(serial) == false) return;

	auto& client = GetClient(serial);
	if (client.IsConnect == false) return;

	::closesocket(client.ClientSocket);

	client.IsConnect = false;
	client.UserName.clear();
	validClientSerials.push(serial);
}

void Framework::ProcessPacket(Framework::SERIAL_TYPE serial, unsigned char* packet, PacketSize size)
{
	if (IsValidClientSerial(serial) == false
		|| packet == nullptr) return;

	auto type = GetPacketId(packet);

	auto procedure = packetProcedures.find(type);
	if (procedure == packetProcedures.end())
		std::cout << "ProcessPacket() - Unknown Packet Type : " << type << std::endl;
	else
		(this->*packetProcedures[type])(serial, packet, size);
}

void Framework::Process_Login(SERIAL_TYPE serial, unsigned char* buf, PacketSize size)
{
	if (IsValidClientSerial(serial) == false
		|| clients[serial]->IsConnect == false)
		return;

	NCommon::PktLogInReq from_packet = { 0 };
	std::memcpy(&from_packet, buf + NCommon::PacketHeaderSize, size - NCommon::PacketHeaderSize);
	
	//그냥 로그인...
	Client& client = GetClient(serial);
	client.UserName = from_packet.szID;
	
	std::cout << from_packet.szID << " login\n";
	
	NCommon::PktLogInRes to_packet;
	to_packet.Id = static_cast<unsigned short>(NCommon::PACKET_ID::LOGIN_IN_RES);
	to_packet.Reserve = 0;
	to_packet.ErrorCode = static_cast<short>(NCommon::ERROR_CODE::NONE);
	to_packet.TotalSize = sizeof(to_packet);
	
	SendPacket(serial, &to_packet, to_packet.TotalSize);
}

void Framework::Process_SelectCharacter(SERIAL_TYPE serial, unsigned char* buf, PacketSize size)
{
	if (IsValidClientSerial(serial) == false
		|| clients[serial]->IsConnect == false)
		return;

	NCommon::PktSelCharacterReq from_packet = { 0 };
	std::memcpy(&from_packet, buf + NCommon::PacketHeaderSize, size - NCommon::PacketHeaderSize);

	Client& client = GetClient(serial);
	client.CharCode = from_packet.CharCode; // 유효 검사 필요...?
	
	std::cout << client.UserName << " selected " << from_packet.CharCode << std::endl;

	NCommon::PktSelCharacterRes to_packet;
	to_packet.Id = static_cast<unsigned short>(NCommon::PACKET_ID::SEL_CHARECTER_RES);
	to_packet.Reserve = 0;
	to_packet.ErrorCode = static_cast<short>(NCommon::ERROR_CODE::NONE);
	to_packet.CharCode = from_packet.CharCode;
	to_packet.TotalSize = sizeof(to_packet);

	SendPacket(serial, &to_packet, to_packet.TotalSize);
}

namespace
{
	void AcceptThreadStart()
	{
		auto framework = Framework::GetInstance();
		SOCKADDR_IN listenAddr;

		SOCKET acceptSocket = ::WSASocketW(AF_INET, SOCK_STREAM,
			IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);

		std::memset(&listenAddr, 0, sizeof(listenAddr));
		listenAddr.sin_family = AF_INET;
		listenAddr.sin_addr.s_addr = htonl(ADDR_ANY);
		listenAddr.sin_port = htons(PORT);

		::bind(acceptSocket, reinterpret_cast<sockaddr *>(&listenAddr), sizeof(listenAddr));
		::listen(acceptSocket, 100);

		FD_SET acceptSet;
		timeval timeVal;
		timeVal.tv_sec = Framework::ACCEPT_TIMEOUT_SECONDS;
		timeVal.tv_usec = 0;

		while (false == framework->IsShutDown())
		{
			FD_ZERO(&acceptSet);
			FD_SET(acceptSocket, &acceptSet);
			int selectResult = ::select(0, &acceptSet, nullptr, nullptr, &timeVal);
			if (selectResult == 0)
			{	//timeout
				continue;
			}
			else if (selectResult == SOCKET_ERROR)
			{
				std::cout << "AcceptThreadStart() - select() error\n";
				continue;
			}

			SOCKADDR_IN clientAddr;
			int addrSize = sizeof(clientAddr);

			SOCKET newClientSocket = ::WSAAccept(acceptSocket,
				reinterpret_cast<sockaddr *>(&clientAddr), &addrSize,
				NULL, NULL);

			if (INVALID_SOCKET == newClientSocket)
			{
				int error_no = WSAGetLastError();
				std::cout << "AcceptThreadStart() - WSAAccept " << "error code : " << error_no << std::endl;
				continue;
			}

			Framework::SERIAL_TYPE newSerial = framework->GetSerialForNewOne(Framework::SERIAL_GET_TYPE::CLIENT);
			if (newSerial == Framework::SERIAL_ERROR)
			{
				::closesocket(newClientSocket);
				std::cout << "AcceptThreadStart() - WSAAccept " << ": too much people\n";
				continue;
			}

			//새 클라이언트 초기화, 등록
			Client& client = framework->GetClient(newSerial);

			client.IsConnect = true;
			client.Serial = newSerial;
			client.ClientSocket = newClientSocket;
			client.RecvOverlap.Operation = Overlap_Exp::OPERATION_RECV;
			client.RecvOverlap.WsaBuf.buf = reinterpret_cast<CHAR*>(client.RecvOverlap.Iocp_Buffer);
			client.RecvOverlap.WsaBuf.len = sizeof(client.RecvOverlap.Iocp_Buffer);

			CreateIoCompletionPort(reinterpret_cast<HANDLE>(newClientSocket), framework->GetIocpHandle(), newSerial, 0);

			DWORD flags = 0;
			int ret = ::WSARecv(newClientSocket, &client.RecvOverlap.WsaBuf, 1, NULL,
				&flags, &client.RecvOverlap.Original_Overlap, NULL);
			if (0 != ret)
			{
				int error_no = WSAGetLastError();
				if (WSA_IO_PENDING != error_no)
					std::cout << "AcceptThreadStart() - WSARecv " << "error code : " << error_no << std::endl;
			}
		}
	}

	void WorkerThreadStart()
	{
		auto framework = Framework::GetInstance();
		while (false == framework->IsShutDown())
		{
			DWORD iosize;
			unsigned long long serial;
			Overlap_Exp* overlapExp;

			BOOL result = GetQueuedCompletionStatus(framework->GetIocpHandle(), 
				&iosize, &serial, reinterpret_cast<LPOVERLAPPED*>(&overlapExp), Framework::GQCS_TIMEOUT_MILLISECONDS);

			if (overlapExp == nullptr
				&& result == false)
			{	//timeout
				continue;
			}
			if (overlapExp->Original_Overlap.Pointer != nullptr)
			{
				std::cout << "WorkerThreadStart() - GetQueuedCompletionStatus error\n";
				continue;
			}
			if (0 == iosize)
			{
				framework->ProcessUserClose(serial);
				continue;
			}

			if (Overlap_Exp::OPERATION_RECV == overlapExp->Operation)
			{
				Client& client = framework->GetClient(serial);
				unsigned char* ioBufCursor = overlapExp->Iocp_Buffer;
				int remained = 0;

				if ((client.SavedPacketSize + iosize) > BUF_SIZE)
				{
					int empty = BUF_SIZE - client.SavedPacketSize;
					std::memcpy(client.PacketBuff + client.SavedPacketSize, ioBufCursor, empty);

					remained = iosize - empty;
					ioBufCursor += empty;
					client.SavedPacketSize += empty;
				}
				else
				{
					std::memcpy(client.PacketBuff + client.SavedPacketSize, ioBufCursor, iosize);
					client.SavedPacketSize += iosize;
				}

				do
				{
					client.PacketSize = GetPacketSize(client.PacketBuff);

					if (client.PacketSize <= client.SavedPacketSize)
					{	//조립가능
						framework->ProcessPacket(serial, client.PacketBuff, static_cast<Framework::PacketSize>(client.PacketSize));
						std::memmove(client.PacketBuff, client.PacketBuff + client.PacketSize
							, client.SavedPacketSize - client.PacketSize);

						client.SavedPacketSize -= client.PacketSize;
						client.PacketSize = 0;

						if (remained > 0
							&& (client.SavedPacketSize + remained) <= sizeof(client.PacketBuff))
						{
							std::memcpy(client.PacketBuff + client.SavedPacketSize, ioBufCursor, remained);
							client.SavedPacketSize += remained;
							remained = 0;
						}
					}
				} while (client.PacketSize == 0);

				DWORD flags = 0;
				::WSARecv(client.ClientSocket,
					&client.RecvOverlap.WsaBuf, 1, NULL, &flags,
					&client.RecvOverlap.Original_Overlap, NULL);
			}
			else if (Overlap_Exp::OPERATION_SEND == overlapExp->Operation)
			{	//전송완료, overlap_exp 반환
				framework->ReturnUsedOverlapExp(overlapExp->Serial);
			}
			else
			{
				std::cout << "Unknown IOCP event!\n";
				return;
			}
		}
	}

} //unnamed namespace
