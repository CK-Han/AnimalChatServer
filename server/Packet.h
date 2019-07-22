#pragma once

#include "PacketID.h"
#include "ErrorCode.h"

namespace NCommon
{
#pragma pack(push, 1)
	struct PktHeader
	{
		unsigned short TotalSize;
		unsigned short Id;
		unsigned char Reserve;
	};


	//- �α��� ��û
	const int MAX_USER_ID_SIZE = 16;
	const int MAX_USER_PASSWORD_SIZE = 16;
	struct PktLogInReq
	{
		char szID[MAX_USER_ID_SIZE + 1] = { 0, };
		char szPW[MAX_USER_PASSWORD_SIZE + 1] = { 0, };
	};

	struct PktLogInRes : PktHeader
	{
		short ErrorCode = (short)ERROR_CODE::NONE;
	};


	//- ĳ���� ����. �� 23 * 7�� ĳ���� 
	struct PktSelCharacterReq
	{
		unsigned short CharCode;
	};

	struct PktSelCharacterRes : PktHeader
	{
		short ErrorCode = (short)ERROR_CODE::NONE;
		unsigned short CharCode;
	};

	//- �� ���� - ���ȣ ����
	//- �� ������
	//- ä��
	//- �̵� (ȭ�� ��ġ ���, ���� Ű���� �̵�)
	//- ����
	//- ���߱� (?)
	/*
	//- �뿡 ���� ��û
	const int MAX_ROOM_TITLE_SIZE = 16;
	struct PktRoomEnterReq
	{
	bool IsCreate;
	short RoomIndex;
	wchar_t RoomTitle[MAX_ROOM_TITLE_SIZE + 1];
	};
	struct PktRoomEnterRes : PktBase
	{
	};

	//- �뿡 �ִ� �������� ���� ���� ���� ���� �뺸
	struct PktRoomEnterUserInfoNtf
	{
	char UserID[MAX_USER_ID_SIZE + 1] = { 0, };
	};
	//- �� ������ ��û
	struct PktRoomLeaveReq {};
	struct PktRoomLeaveRes : PktBase
	{
	};
	//- �뿡�� ������ ���� �뺸(�κ� �ִ� ��������)
	struct PktRoomLeaveUserInfoNtf
	{
	char UserID[MAX_USER_ID_SIZE + 1] = { 0, };
	};

	//- �� ä��
	const int MAX_ROOM_CHAT_MSG_SIZE = 256;
	struct PktRoomChatReq
	{
	wchar_t Msg[MAX_ROOM_CHAT_MSG_SIZE + 1] = { 0, };
	};
	struct PktRoomChatRes : PktBase
	{
	};
	struct PktRoomChatNtf
	{
	char UserID[MAX_USER_ID_SIZE + 1] = { 0, };
	wchar_t Msg[MAX_ROOM_CHAT_MSG_SIZE + 1] = { 0, };
	};
	//- �κ� ä��
	const int MAX_LOBBY_CHAT_MSG_SIZE = 256;
	struct PktLobbyChatReq
	{
	wchar_t Msg[MAX_LOBBY_CHAT_MSG_SIZE + 1] = { 0, };
	};
	struct PktLobbyChatRes : PktBase
	{
	};
	struct PktLobbyChatNtf
	{
	char UserID[MAX_USER_ID_SIZE + 1] = { 0, };
	wchar_t Msg[MAX_LOBBY_CHAT_MSG_SIZE + 1] = { 0, };
	};
	// ������ ���� ���� ��û
	struct PktRoomMaterGameStartReq
	{};
	struct PktRoomMaterGameStartRes : PktBase
	{};
	struct PktRoomMaterGameStartNtf
	{};
	// ������ �ƴ� ����� ���� ���� ��û
	struct PktRoomGameStartReq
	{};
	struct PktRoomGameStartRes : PktBase
	{};
	struct PktRoomGameStartNtf
	{
	char UserID[MAX_USER_ID_SIZE + 1] = { 0, };
	};
	const int DEV_ECHO_DATA_MAX_SIZE = 1024;
	struct PktDevEchoReq
	{
	short DataSize;
	char Datas[DEV_ECHO_DATA_MAX_SIZE];
	};
	struct PktDevEchoRes : PktBase
	{
	short DataSize;
	char Datas[DEV_ECHO_DATA_MAX_SIZE];
	};
	*/
#pragma pack(pop)


	const short PacketHeaderSize = sizeof(PktHeader);
}