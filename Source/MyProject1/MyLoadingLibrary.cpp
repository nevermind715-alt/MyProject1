#include "MyLoadingLibrary.h"
#include "Engine/World.h"
#include "Engine/Engine.h"

#pragma execution_character_set("utf-8")

bool UMyLoadingLibrary::IsWorldFullyLoaded(const UObject* WorldContextObject)
{
	// 現在の世界（ワールド）を取得
	if (UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
	{
		// 世界の基本データの読み込みが終わっているか？
		if (World->AreAlwaysLoadedLevelsLoaded())
		{
			// 念押し：ストリーミング（非同期読み込み）の残りも強制的に完了させる
			GEngine->BlockTillLevelStreamingCompleted(World);
			return true; // 読み込み完了！
		}
	}
	return false; // まだ読み込み中
}