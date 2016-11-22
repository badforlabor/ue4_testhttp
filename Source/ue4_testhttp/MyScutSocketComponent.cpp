// Fill out your copyright notice in the Description page of Project Settings.

#include "ue4_testhttp.h"
#include "MyScut.h"
#include "MyScutSocketComponent.h"
#include "http.h"
#include <string>

void VShow(const char*, ...)
{

}
// void AYourClass::Laaaaaauuuunch()
// {
// 	//IP = 127.0.0.1, Port = 8890 for my Python test case
// 	if (!StartTCPReceiver("RamaSocketListener", "127.0.0.1", 8890))
// 	{
// 		//UE_LOG  "TCP Socket Listener Created!"
// 		return;
// 	}
// 
// 	//UE_LOG  "TCP Socket Listener Created! Yay!"
// }

// Sets default values for this component's properties
UMyScutSocketComponent::UMyScutSocketComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	bWantsBeginPlay = true;
	PrimaryComponentTick.bCanEverTick = true;

	// ...

	IpAddress = TEXT("127.0.0.1");
	IpPort = 9001;
}


// Called when the game starts
void UMyScutSocketComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...

	Socket = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateSocket(NAME_Stream, TEXT("default"), false);

	this->GetOwner()->GetWorldTimerManager().SetTimer(TCPReceiveTimerHandler, this,
		&UMyScutSocketComponent::TCPSocketReceived, 0.1f, true);

	RegisterProtocol();
}
void UMyScutSocketComponent::RegisterProtocol()
{
	DispatchMaps.Empty();

	DispatchMaps.Add(1001, &UMyScutSocketComponent::OnAction1001);
	DispatchMaps.Add(1002, &UMyScutSocketComponent::OnAction1002);
}

// Called every frame
void UMyScutSocketComponent::TickComponent( float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction )
{
	Super::TickComponent( DeltaTime, TickType, ThisTickFunction );

	// ...
}

int UMyScutSocketComponent::StartTCPConnect()
{
	if (Socket == NULL)
		return 0;

	FString address = IpAddress;
	int32 port = IpPort;
	FIPv4Address ip;
	FIPv4Address::Parse(address, ip);


	TSharedRef<FInternetAddr> addr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
	bool isvalid = false;
	addr->SetIp(*ip.ToString(), isvalid);
	addr->SetPort(port);

	bool connected = Socket->Connect(*addr);
	
	return 1;
}
int UMyScutSocketComponent::CloseTCPConnect()
{
	if (Socket == NULL)
		return 0;

	Socket->Close();
	return 0;
}
void UMyScutSocketComponent::SendData(const FString& content)
{
	if (Socket == NULL)
		return;

	FString msg;
	//msg += FString::Printf(TEXT("MsgId=%d&Sid=%s&Uid=%s&St=%s&"), Counter, *SessionID, *UserID, *St);
	msg += FString(TEXT("MsgId=1&Sid=&Uid=0&St=&"));

	// 比如：ActionId=1001&PageIndex=1&PageSize=30
	msg += content;

	// 加密秘钥，与服务器约定的
	FString MD5Key(TEXT("44CAC8ED53714BF18D60C5C7B6296000"));
	FString strTemp = msg + MD5Key;

	FString md5value = FMD5::HashAnsiString(*strTemp);

	msg += FString::Printf(TEXT("&sign=%s"), *md5value);

	msg = FGenericPlatformHttp::UrlEncode(msg);

	msg = FString::Printf(TEXT("?d=%s"), *msg);

	FString serialized = msg;
	TCHAR *serializedChar = serialized.GetCharArray().GetData();
	int32 size = FCString::Strlen(serializedChar);
	int32 sent = 0;

	TArray<uint8> data;

	int nDataLen = size;
	uint8 first = (nDataLen & 0x000000ff);
	data.Add(first);
	uint8 second = (nDataLen & 0x0000ff00) >> 8;
	data.Add(second);
	uint8 third = (nDataLen & 0x00ff0000) >> 16;
	data.Add(third);
	uint8 forth = nDataLen >> 24;
	data.Add(forth);

	for (int i = 0; i < size; i++)
	{
		TCHAR wc = serializedChar[i];
		uint8 a = *((uint8*)(&wc));
		data.Add(a);
	}

	bool successful = Socket->Send(data.GetData(), data.Num(), sent);
}
void UMyScutSocketComponent::TCPSocketReceived()
{
	TArray<uint8> ReceivedData;

	FSocket* ConnectionSocket = Socket;

	uint32 Size;
	while (ConnectionSocket->HasPendingData(Size))
	{
		ReceivedData.Init(0, FMath::Min(Size, 65507u));

		int32 Read = 0;
		ConnectionSocket->Recv(ReceivedData.GetData(), ReceivedData.Num(), Read);
	}
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	if (ReceivedData.Num() <= 0)
	{
		//No Data Received
		return;
	}

	if (ReceivedData.Num() < 8)
		return;

	void* pContent = (void*)ReceivedData.GetData();
	int len = ReceivedData.Num();
	FBufferReader ar(pContent, len, false);

	// 前4位是buffer的大小
	int size1 = MyScut::ReadInt(ar);

	// 紧接着的4位有可能是gzip标志(16进制)：1F 8B 08 00
	TArray<uint8> out;
	if (ReceivedData[4] == 0x1F && ReceivedData[5] == 0x8B && ReceivedData[6] == 0x08 && ReceivedData[7] == 0x00)
	{
		char* pchar = (char*)pContent;
		int skipLen = sizeof(int);
		pchar += skipLen;
		len -= skipLen;

		MyScut::DecompressNetData((void*)pchar, len, out);

		pContent = out.GetData();
		len = out.Num();
		size1 = len;
		ar = FBufferReader(pContent, len, false);
	}

	int size2 = MyScut::ReadInt(ar);
	if (size2 != size1)
	{
		Debug(TEXT("size invalid."));
		return;
	}

	// 协议头
	Debug(TEXT("read head."));
	int result = MyScut::ReadInt(ar);
	int rmid = MyScut::ReadInt(ar);
	FString errmsg = MyScut::ReadString(ar);
	int actionid = MyScut::ReadInt(ar);

	Debug(FString::Printf(TEXT("response, actionid=%d"), actionid));

	FString tempST = MyScut::ReadString(ar);

	// 上面全是基础字段，下面是协议数据字段，不同协议序列化方式不同
	if (DispatchMaps.Contains(actionid))
	{
		NET_CALLBACK callback = DispatchMaps[actionid];
		(this->*callback)(ar);
	}
}

void UMyScutSocketComponent::OnAction1001(FBufferReader& ar)
{
	FString RAnkName = MyScut::ReadString(ar);
	// 排行榜内容处理
	int PageCount = MyScut::ReadInt(ar);
	int RankListCount = MyScut::ReadInt(ar);
	for (int i = 0; i < RankListCount; i++)
	{
		// scut的数据结构第一个字段是数据结构的大小
		int structsize = MyScut::ReadInt(ar);

		FString str = MyScut::ReadString(ar);
		int score = MyScut::ReadInt(ar);
		FString strSteamID = MyScut::ReadString(ar);
		float accuarcy = MyScut::ReadFloat(ar);
		int rate = MyScut::ReadInt(ar);

		Debug(FString::Printf(TEXT("response, name=%s, score=%d, accuracy=%f, rate=%d"), *(str), score, accuarcy, rate));
	}
	Debug(FString::Printf(TEXT("response, PageCount=%d"), PageCount));
}
void UMyScutSocketComponent::OnAction1002(FBufferReader& ar)
{
	
}