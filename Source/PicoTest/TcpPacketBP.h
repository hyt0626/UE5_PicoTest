#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "TcpPacketBP.generated.h"


UCLASS()
class PICOTEST_API UTcpPacketBP : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// 按协议打包：
	// [int32 totalLength][uint32 type][utf8 json bytes][0]
	// totalLength = 4(type) + jsonLen + 1(ending 0)
	UFUNCTION(BlueprintPure, Category = "TCP|Packet")
	static TArray<uint8> BuildPacketFromJson(int32 Type, const FString& JsonString);

	// 尝试从 Buffer 中拆出一条完整包（成功则从 Buffer 移除该包字节）
	UFUNCTION(BlueprintCallable, Category = "TCP|Packet")
	static bool TryPopPacket(UPARAM(ref) TArray<uint8>& Buffer, int32& OutType, FString& OutJsonString);
};
