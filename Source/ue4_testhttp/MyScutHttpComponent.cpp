// Fill out your copyright notice in the Description page of Project Settings.

#include "ue4_testhttp.h"
#include "MyScut.h"
#include "MyScutHttpComponent.h"
#include "Http.h"
#include "zlib.h"

// Sets default values for this component's properties
UMyScutHttpComponent::UMyScutHttpComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	bWantsBeginPlay = true;
	PrimaryComponentTick.bCanEverTick = true;

	// ...
	WebUrl = FString(TEXT("http://127.0.0.1:18013"));
}


// Called when the game starts
void UMyScutHttpComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	RegisterProtocol();
}


// Called every frame
void UMyScutHttpComponent::TickComponent( float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction )
{
	Super::TickComponent( DeltaTime, TickType, ThisTickFunction );

	// ...
}

void UMyScutHttpComponent::RegisterProtocol()
{
	DispatchMaps.Empty();

	DispatchMaps.Add(1001, &UMyScutHttpComponent::OnAction1001);
}

void UMyScutHttpComponent::ReqHttp(const FString& content)
{
	/* 
		格式为："MsgId=1&Sid=&Uid=0&St=&ActionId=1001&PageIndex=1&PageSize=30&sign=c86ae3e7e3cc05ab034f4d6cc18ecec1"
			其中，ActionId=1001&PageIndex=1&PageSize=30为content数据
			MsgId=1&Sid=&Uid=0&St= 为自动生成的
			&sign=c86ae3e7e3cc05ab034f4d6cc18ecec1 为数据的md5值
	*/

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
	
	// 发送http请求
	FString url = FString::Printf(TEXT("%s%s"), *WebUrl, TEXT("/Service.aspx?d="));
	TSharedRef<IHttpRequest> Request = FHttpModule::Get().CreateRequest();
	Request->OnProcessRequestComplete().BindUObject(this, &UMyScutHttpComponent::OnResponseReceived);
	Request->SetURL(url + msg);
	Request->SetVerb("GET");
 	Request->SetHeader(TEXT("User-Agent"), TEXT("unknown"));
	Request->SetHeader(TEXT("Accept-Encoding"), TEXT("gzip, deflate"));
 	//Request->SetHeader("Content-Type", TEXT("application/json"));
	Request->ProcessRequest();


	Debug(FString::Printf(TEXT("req url:%s"), *(url + msg)));
}
void UMyScutHttpComponent::OnResponseReceived(FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded)
{
	if (!HttpResponse.IsValid() || HttpResponse->GetContentLength() <= 0)
		return;
	if (!bSucceeded)
		return;

	void* pContent = (void*)HttpResponse->GetContent().GetData();
	int len = HttpResponse->GetContentLength();

	// 如果服务器发过来的是压缩数据，那么先解压缩
	TArray<uint8> out;
	if (HttpResponse->GetHeader(TEXT("Content-Encoding")) == TEXT("gzip"))
	{
		MyScut::DecompressNetData(pContent, len, out);
		pContent = out.GetData();
		len = out.Num();
	}

	// 如果数据非法，那么直接返回
	if (len == 0)
		return;

	FBufferReader ar(pContent, len, false);

	int buffersize = MyScut::ReadInt(ar);
	Debug(FString::Printf(TEXT("response, content size=%d, %d"), HttpResponse->GetContentLength(), buffersize));

	int result = MyScut::ReadInt(ar);
	int rmid = MyScut::ReadInt(ar);
	FString errmsg = MyScut::ReadString(ar);
	int actionid = MyScut::ReadInt(ar);
	FString st = MyScut::ReadString(ar);

	Debug(FString::Printf(TEXT("response, actionid=%d"), actionid));

	// 上面全是基础字段，下面是协议数据字段，不同协议序列化方式不同
	if (DispatchMaps.Contains(actionid))
	{
		NET_CALLBACK callback = DispatchMaps[actionid];
		(this->*callback)(ar);
	}
}  
void UMyScutHttpComponent::OnAction1001(FBufferReader& ar)
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
}