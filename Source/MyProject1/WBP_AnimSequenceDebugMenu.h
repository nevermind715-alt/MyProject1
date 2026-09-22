#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MyProject1Types.h"
#include "WBP_AnimSequenceDebugMenu.generated.h"

// DT_AnimSequencesの行を一覧表示し、クリックした行をその場でPlayer側に再生して
// MeshLocationOffset/MeshRotationOffsetの位置調整を確認するためのデバッグメニュー。
// WBP_WarpDebugMenuと同じ構造・同じ開き方（BP側のキー入力からCreate Widget）を想定している
UCLASS()
class MYPROJECT1_API UWBP_AnimSequenceDebugMenu : public UUserWidget
{
	GENERATED_BODY()

public:
	// 一覧のボタンから呼ぶ。常にPlayer側でRowNameのMontageを再生する（Player自身の位置調整確認用のため）。
	// 再生中は見やすさのためメニューを閉じる（WarpDebugMenuと同じ挙動）。別の行を試す時はデバッグキーで開き直す
	UFUNCTION(BlueprintCallable, Category = "AnimEvent Debug")
	void PlayDebugRow(FName RowName);

	// メニューを閉じる（マウスカーソルと入力モードを元に戻し、ビューポートから外す）
	UFUNCTION(BlueprintCallable, Category = "AnimEvent Debug")
	void CloseDebugMenu();

protected:
	virtual void NativeConstruct() override;

	// DT_AnimSequencesの全行を渡すので、BP側でこれを受けてScrollBox等にボタンを並べる
	UFUNCTION(BlueprintImplementableEvent, Category = "AnimEvent Debug")
	void OnAnimSequenceListReady(const TArray<FAnimSequenceRowInfo>& Rows);
};
