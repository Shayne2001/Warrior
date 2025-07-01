#pragma once

namespace Debug
{
	static void Print(const FString& Msg, const FColor& Color = FColor::MakeRandomColor(), int32 InKey = -1)
	{
		if (GEngine) // 检查全局引擎指针是否有效，确保只在引擎正常运行状态下执行
		{
			GEngine->AddOnScreenDebugMessage(InKey, 7.F, Color, Msg);

			UE_LOG(LogTemp, Warning, TEXT("%s"), *Msg);
		}
	}
}