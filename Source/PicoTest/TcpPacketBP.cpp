#include "TcpPacketBP.h"

static FORCEINLINE int32 ReadInt32LE(const uint8* P)
{
	return (int32)(
		(uint32)P[0] |
		((uint32)P[1] << 8) |
		((uint32)P[2] << 16) |
		((uint32)P[3] << 24)
		);
}

static FORCEINLINE uint32 ReadUInt32LE(const uint8* P)
{
	return (uint32)(
		(uint32)P[0] |
		((uint32)P[1] << 8) |
		((uint32)P[2] << 16) |
		((uint32)P[3] << 24)
		);
}

static FORCEINLINE void WriteInt32LE(uint8* P, int32 V)
{
	P[0] = (uint8)(V & 0xFF);
	P[1] = (uint8)((V >> 8) & 0xFF);
	P[2] = (uint8)((V >> 16) & 0xFF);
	P[3] = (uint8)((V >> 24) & 0xFF);
}

static FORCEINLINE void WriteUInt32LE(uint8* P, uint32 V)
{
	P[0] = (uint8)(V & 0xFF);
	P[1] = (uint8)((V >> 8) & 0xFF);
	P[2] = (uint8)((V >> 16) & 0xFF);
	P[3] = (uint8)((V >> 24) & 0xFF);
}

TArray<uint8> UTcpPacketBP::BuildPacketFromJson(int32 Type, const FString& JsonString)
{
	// FString -> UTF8 bytes
	FTCHARToUTF8 Convert(*JsonString);
	const uint8* Utf8Data = (const uint8*)Convert.Get();
	const int32 Utf8Len = Convert.Length();

	// totalLength = 4(type) + jsonLen + 1(ending0)
	const int32 TotalLength = 4 + Utf8Len + 1;
	const int32 PacketLen = 4 + TotalLength; // 4(length自身) + totalLength

	TArray<uint8> Out;
	Out.SetNumUninitialized(PacketLen);

	uint8* W = Out.GetData();

	// length (int32 LE)
	WriteInt32LE(W, TotalLength);
	W += 4;

	// type (uint32 LE)
	WriteUInt32LE(W, (uint32)Type);
	W += 4;

	// json bytes
	if (Utf8Len > 0)
	{
		FMemory::Memcpy(W, Utf8Data, Utf8Len);
		W += Utf8Len;
	}

	// ending 0
	*W = 0;

	return Out;
}

bool UTcpPacketBP::TryPopPacket(TArray<uint8>& Buffer, int32& ReadOffset, int32& OutType, FString& OutJsonString)
{
	OutType = 0;
	OutJsonString.Reset();

	// 修正 ReadOffset 防御（避免外部误传）
	ReadOffset = FMath::Clamp(ReadOffset, 0, Buffer.Num());

	const int32 Available = Buffer.Num() - ReadOffset;
	if (Available < 4)
		return false;

	const uint8* Base = Buffer.GetData() + ReadOffset;

	const int32 TotalLength = ReadInt32LE(Base);

	// 基本合法性：至少 type(4) + ending0(1)
	if (TotalLength < 5)
	{
		// 协议头不合法：这里选择直接失败，不推进 offset
		// 若你希望“尝试重同步”，可改为 ReadOffset += 1; return true/false 继续循环
		return false;
	}

	// 最大包限制（防恶意/错误 length）
	const int32 MaxTotal = GetMaxPacketTotalLength();
	if (TotalLength > MaxTotal)
	{
		return false;
	}

	const int32 Need = 4 + TotalLength; // length自身(4) + totalLength
	if (Available < Need)
		return false;

	// Packet = [type(4)][content...][0]
	const uint8* Packet = Base + 4;

	const uint32 TypeU = ReadUInt32LE(Packet);
	OutType = (int32)TypeU;

	const uint8* Content = Packet + 4;
	int32 JsonLen = TotalLength - 4; // content + ending0

	if (JsonLen > 0 && Content[JsonLen - 1] == 0)
	{
		JsonLen -= 1; // 去掉末尾 0
	}

	// UTF8 -> FString
	// 注：JsonLen 是字节数，不包含末尾 0
	FUTF8ToTCHAR Back((const ANSICHAR*)Content, JsonLen);
	OutJsonString = FString(Back.Length(), Back.Get());

	// 关键：只推进 ReadOffset，不做 RemoveAt(0)
	ReadOffset += Need;

	return true;
}

void UTcpPacketBP::CompactBuffer(TArray<uint8>& Buffer, int32& ReadOffset, int32 MinReadOffsetToCompact, bool CompactIfMoreThanHalf)
{
	ReadOffset = FMath::Clamp(ReadOffset, 0, Buffer.Num());

	if (ReadOffset <= 0)
		return;

	const bool bOverThreshold = ReadOffset >= MinReadOffsetToCompact;
	const bool bOverHalf = CompactIfMoreThanHalf ? (ReadOffset >= Buffer.Num() / 2) : false;

	if (!bOverThreshold && !bOverHalf)
		return;

	const int32 Remaining = Buffer.Num() - ReadOffset;
	if (Remaining > 0)
	{
		FMemory::Memmove(Buffer.GetData(), Buffer.GetData() + ReadOffset, Remaining);
	}

	Buffer.SetNum(Remaining, false);
	ReadOffset = 0;
}

void UTcpPacketBP::ResetBuffer(TArray<uint8>& Buffer, int32& ReadOffset)
{
	Buffer.Reset();
	ReadOffset = 0;
}
