// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/ActorComponent.h"
#include "Http.h"
#include "MyScutHttpComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class UE4_TESTHTTP_API UMyScutHttpComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UMyScutHttpComponent();

	// Called when the game starts
	virtual void BeginPlay() override;
	
	// Called every frame
	virtual void TickComponent( float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction ) override;

public:
	UFUNCTION(BlueprintCallable, Category = "MyScut")
		void ReqHttp(const FString& content);

public:
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "MyScut")
		FString WebUrl;
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "MyScut")
		FString SessionID;
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "MyScut")
		FString UserID;	// 64λ����

private:
	int Counter;
	FString St;

	// Э���ɷ���
protected:
	void OnResponseReceived(FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded);
	void RegisterProtocol();

	typedef void (UMyScutHttpComponent::*NET_CALLBACK)(FBufferReader& ar);
	TMap<int, NET_CALLBACK> DispatchMaps;

protected:
	void OnAction1001(FBufferReader& ar);
};
