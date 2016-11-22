// Fill out your copyright notice in the Description page of Project Settings.

#pragma once


#include "ue4_testhttp.h"
#include "Engine.h"
#include "MyScut.h"
#include "Http.h"
#include "zlib.h"

DECLARE_LOG_CATEGORY_EXTERN(MyLog, Verbose, All);
DEFINE_LOG_CATEGORY(MyLog);
void Debug(const FString& msg)
{
	UE_LOG(MyLog, Verbose, TEXT("%s"), *msg);
	const FString& FinalDisplayString = msg;
	GEngine->AddOnScreenDebugMessage((uint64)-1, 5, FColor::Red, FinalDisplayString);
}
namespace MyScut
{
	void ScutLog(const char* msg)
	{
		Debug(ANSI_TO_TCHAR(msg));
	}

	int inflateMemory_(unsigned char *in, unsigned int inLength, unsigned char **out, unsigned int *outLength)
	{
		const int BUFFER_INC_FACTOR = (2);

		/* ret value */
		int err = Z_OK;

		/* 256k initial decompress buffer */
		int bufferSize = 256 * 1024;
		if (inLength * 8 > 256 * 1024)
		{
			bufferSize = 8 * inLength;
		}
		*out = new unsigned char[bufferSize];

		z_stream d_stream; /* decompression stream */
		d_stream.zalloc = (alloc_func)0;
		d_stream.zfree = (free_func)0;
		d_stream.opaque = (voidpf)0;

		d_stream.next_in = in;
		d_stream.avail_in = inLength;
		d_stream.next_out = *out;
		d_stream.avail_out = bufferSize;

		/* window size to hold 256k */
		if ((err = inflateInit2(&d_stream, 15 + 32)) != Z_OK)
		{
			return err;
		}

		for (;;) {
			err = inflate(&d_stream, Z_NO_FLUSH);

			if (err == Z_STREAM_END)
			{
				break;
			}

			switch (err) {
			case Z_NEED_DICT:
				err = Z_DATA_ERROR;
			case Z_DATA_ERROR:
			case Z_MEM_ERROR:
				inflateEnd(&d_stream);
				return err;
			}

			// not enough memory ?
			if (err != Z_STREAM_END)
			{
				delete[] * out;
				*out = new unsigned char[bufferSize * BUFFER_INC_FACTOR];

				/* not enough memory, ouch */
				if (!*out)
				{
					ScutLog("ScutDataLogic: ZipUtils: realloc failed");
					inflateEnd(&d_stream);
					return Z_MEM_ERROR;
				}

				d_stream.next_out = *out + bufferSize;
				d_stream.avail_out = bufferSize;
				bufferSize *= BUFFER_INC_FACTOR;
			}
		}


		*outLength = bufferSize - d_stream.avail_out;
		err = inflateEnd(&d_stream);
		return err;
	}
	int ccInflateMemory(unsigned char *in, unsigned int inLength, unsigned char **out)
	{
		unsigned int outLength = 0;
		int err = inflateMemory_(in, inLength, out, &outLength);

		if (err != Z_OK || *out == NULL) {
			if (err == Z_MEM_ERROR)
			{
				ScutLog("ScutDataLogic: ZipUtils: Out of memory while decompressing map data!");
			}
			else
				if (err == Z_VERSION_ERROR)
				{
					ScutLog("ScutDataLogic: ZipUtils: Incompatible zlib version!\r\n");
				}
				else
					if (err == Z_DATA_ERROR)
					{
						ScutLog("ScutDataLogic: ZipUtils: Incorrect zlib compressed data!\r\n");
					}
					else
					{
						ScutLog("ScutDataLogic: ZipUtils: Unknown error while decompressing map data!\r\n");
					}

			delete[] * out;
			*out = NULL;
			outLength = 0;
		}

		return outLength;
	}

	void DecompressNetData(void * data, int size, TArray<uint8>& out)
	{
		unsigned char* pszOut;
		int nLen = ccInflateMemory((unsigned char *)data, size, &pszOut);
		if (nLen > 0)
		{
			for (int i = 0; i < nLen; i++)
			{
				out.Add(pszOut[i]);
			}
			delete[]pszOut;
		}
	}

	int ReadInt(FBufferReader& ar)
	{
		int ret = 0;
		ar << ret;
		return ret;
	}
	float ReadFloat(FBufferReader& ar)
	{
		float ret = 0;
		ar << ret;
		return ret;
	}
	FString ReadString(FBufferReader& ar)
	{
		if (!ar.IsLoading())
		{
			return TEXT("");
		}
		{
			TArray<ANSICHAR> str;

			int size;
			ar << size;

			if (size > 0)
			{
				ANSICHAR c;
				str.Empty(size);
				for (int i = 0; i < size; i++)
				{
					ar << c;
					str.Add(c);
				}
				str.Add('\0');
			}

			return ANSI_TO_TCHAR(str.GetData());
		}
	}
}

