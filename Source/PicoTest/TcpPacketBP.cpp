#include "TcpPacketBP.h"

static void AppendInt32LE(TArray<uint8>& Out, int32 V)
{
	Out.Add((uint8)(V & 0xFF));
	Out.Add((uint8)((V >> 8) & 0xFF));
	Out.Add((uint8)((V >> 16) & 0xFF));
	Out.Add((uint8)((V >> 24) & 0xFF));
}

static void AppendUInt32LE(TArray<uint8>& Out, uint32 V)
{
	Out.Add((uint8)(V & 0xFF));
	Out.Add((uint8)((V >> 8) & 0xFF));
	Out.Add((uint8)((V >> 16) & 0xFF));
	Out.Add((uint8)((V >> 24) & 0xFF));
}

static int32 ReadInt32LE(const uint8* P)
{
	return (int32)(
		(uint32)P[0] |
		((uint32)P[1] << 8) |
		((uint32)P[2] << 16) |
		((uint32)P[3] << 24)
		);
}

static uint32 ReadUInt32LE(const uint8* P)
{
	return (uint32)(
		(uint32)P[0] |
		((uint32)P[1] << 8) |
		((uint32)P[2] << 16) |
		((uint32)P[3] << 24)
		);
}

TArray<uint8> UTcpPacketBP::BuildPacketFromJson(int32 Type, const FString& JsonString)
{
	// FString -> UTF8 bytes
	FTCHARToUTF8 Convert(*JsonString);
	const uint8* Utf8Data = (const uint8*)Convert.Get();
	const int32 Utf8Len = Convert.Length();

	// totalLength = sizeof(uint32 type) + msgBytes.Length + 1
	const int32 TotalLength = 4 + Utf8Len + 1;

	TArray<uint8> Out;
	Out.Reserve(4 + TotalLength);

	// 写入总长度 int32（小端）——对齐 C# BinaryWriter.Write(int)
	AppendInt32LE(Out, TotalLength);

	// 写入 type uint32（小端）——对齐 C# BinaryWriter.Write(uint)
	AppendUInt32LE(Out, (uint32)Type);

	// 写入 json bytes
	Out.Append(Utf8Data, Utf8Len);

	// 结尾 0
	Out.Add(0);

	return Out;
}

bool UTcpPacketBP::TryPopPacket(TArray<uint8>& Buffer, int32& OutType, FString& OutJsonString)
{
	OutType = 0;
	OutJsonString.Reset();

	// 至少要有 4 字节 length
	if (Buffer.Num() < 4)
		return false;

	const int32 TotalLength = ReadInt32LE(Buffer.GetData());

	// 合法性：至少 type(4) + ending0(1)
	if (TotalLength < 5)
		return false;

	const int32 Need = 4 + TotalLength; // 4(length自身) + totalLength
	if (Buffer.Num() < Need)
		return false;

	// Packet = [type(4)][content...][0]
	const uint8* Packet = Buffer.GetData() + 4;

	const uint32 TypeU = ReadUInt32LE(Packet);
	OutType = (int32)TypeU;

	const int32 ContentLen = TotalLength - 4;     // content + 0
	const uint8* Content = Packet + 4;

	int32 JsonLen = ContentLen;
	if (JsonLen > 0 && Content[JsonLen - 1] == 0)
	{
		JsonLen -= 1; // 去掉末尾识别字节 0
	}

	// UTF8 -> FString
	FUTF8ToTCHAR Back((const ANSICHAR*)Content, JsonLen);
	OutJsonString = FString(Back.Length(), Back.Get());

	// 从 Buffer 移除已消费的字节（不缩容，效率更好）
	Buffer.RemoveAt(0, Need, false);

	return true;
}
