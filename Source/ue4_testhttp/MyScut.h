// Fill out your copyright notice in the Description page of Project Settings.

#pragma once


extern void Debug(const FString& msg);

namespace MyScut
{
	extern int ReadInt(FBufferReader& ar);
	extern float ReadFloat(FBufferReader& ar);
	extern FString ReadString(FBufferReader& ar);
	extern void DecompressNetData(void * data, int size, TArray<uint8>& out);
}