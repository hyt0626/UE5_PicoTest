#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "TcpPacketBP.generated.h"

UCLASS()
class PICOTEST_API UTcpPacketBP : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// 协议：
	// [int32 totalLength][uint32 type][utf8 json bytes][0]
	// totalLength = 4(type) + jsonLen + 1(ending 0)
	UFUNCTION(BlueprintPure, Category = "TCP|Packet")
	static TArray<uint8> BuildPacketFromJson(int32 Type, const FString& JsonString);

	// 尝试从 Buffer 中拆出一条完整包（成功则推进 ReadOffset，不做 RemoveAt）
	// - Buffer：持续 Append 接收字节的缓存
	// - ReadOffset：读指针（必须由调用方持久保存）
	// 成功时返回 true，并输出 Type/Json；失败返回 false（可能是数据不够或包不完整）
	UFUNCTION(BlueprintCallable, Category = "TCP|Packet")
	static bool TryPopPacket(
		UPARAM(ref) TArray<uint8>& Buffer,
		UPARAM(ref) int32& ReadOffset,
		int32& OutType,
		FString& OutJsonString);

	// 可选：压缩 Buffer，回收前面已消费空间（建议在每次接收后或每帧调用一次）
	// 参数：
	// - MinReadOffsetToCompact：ReadOffset 超过该阈值才压缩（避免频繁 memmove）
	// - CompactIfMoreThanHalf：ReadOffset 超过 Buffer 一半也触发压缩
	UFUNCTION(BlueprintCallable, Category = "TCP|Packet")
	static void CompactBuffer(
		UPARAM(ref) TArray<uint8>& Buffer,
		UPARAM(ref) int32& ReadOffset,
		int32 MinReadOffsetToCompact = 64 * 1024,
		bool CompactIfMoreThanHalf = true);

	// 可选：清空缓冲（断线/重连/协议错误时用）
	UFUNCTION(BlueprintCallable, Category = "TCP|Packet")
	static void ResetBuffer(UPARAM(ref) TArray<uint8>& Buffer, UPARAM(ref) int32& ReadOffset);

	// 可选：最大包体限制（防止恶意/错误 length 造成内存暴涨）
	// 你也可以改成项目配置项
	UFUNCTION(BlueprintPure, Category = "TCP|Packet")
	static int32 GetMaxPacketTotalLength() { return 16 * 1024 * 1024; } // 16MB，可按需调
};
