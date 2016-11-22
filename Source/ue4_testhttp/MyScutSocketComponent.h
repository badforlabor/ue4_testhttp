// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/ActorComponent.h"
#include "Networking.h"
#include "MyScutSocketComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class UE4_TESTHTTP_API UMyScutSocketComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UMyScutSocketComponent();

	// Called when the game starts
	virtual void BeginPlay() override;
	
	// Called every frame
	virtual void TickComponent( float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction ) override;

public:
	FSocket* Socket;
	FTimerHandle TCPReceiveTimerHandler;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "MyScut")
		FString IpAddress;
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "MyScut")
		int IpPort;

	UFUNCTION(BlueprintCallable, Category = "MyScut")
		int StartTCPConnect();

	UFUNCTION(BlueprintCallable, Category = "MyScut")
		int CloseTCPConnect();

	UFUNCTION(BlueprintCallable, Category = "MyScut")
	void SendData(const FString& content);

	void TCPSocketReceived();

	typedef void (UMyScutSocketComponent::*NET_CALLBACK)(FBufferReader& ar);
	TMap<int, NET_CALLBACK> DispatchMaps;

	void RegisterProtocol();
	void OnAction1001(FBufferReader& ar);
	void OnAction1002(FBufferReader& ar);
};
