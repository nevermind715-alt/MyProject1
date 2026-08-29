#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MyProject1Types.h" // FDialogDataなどの構造体を使うため
#include "DialogComponent.generated.h"

// UIに通知するためのデリゲート（セリフデータと、話しているNPCの情報を送る）
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDialogUpdated, const FDialogData&, DialogData, AActor*, NPC);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDialogClosed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHideChoices);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MYPROJECT1_API UDialogComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UDialogComponent();

	// 会話を開始する関数（データテーブルの行名を指定）
	UFUNCTION(BlueprintCallable, Category = "Dialog")
	void StartDialog(FName RowName, UDataTable* DialogTable, AActor* InNPC);

	// 選択肢が選ばれた時にUIから呼ばれる関数
	UFUNCTION(BlueprintCallable, Category = "Dialog")
	void SelectChoice(int32 ChoiceIndex);

	// --- その選択肢の条件（数値・フラグ）を満たしているかチェックする ---
	UFUNCTION(BlueprintPure, Category = "Dialog")
	bool CanSelectChoice(const FDialogChoice& Choice) const;

	// 選択肢のない会話で、画面をクリックして「次へ進む」時に呼ばれる関数
	UFUNCTION(BlueprintCallable, Category = "Dialog")
	void AdvanceDialog();

	// 会話を終了させる
	UFUNCTION(BlueprintCallable, Category = "Dialog")
	void CloseDialog();

	// UI側でこれにイベントバインドする
	UPROPERTY(BlueprintAssignable, Category = "Dialog")
	FOnDialogUpdated OnDialogUpdated;

	UPROPERTY(BlueprintAssignable, Category = "Dialog")
	FOnDialogClosed OnDialogClosed;

	UPROPERTY(BlueprintAssignable, Category = "Dialog")
	FOnHideChoices OnHideChoices;

private:
	// 現在使用中のデータテーブルとNPC
	UPROPERTY()
	UDataTable* CurrentTable;

	UPROPERTY()
	AActor* CurrentNPC;

	// 現在の会話データ
	FDialogData CurrentDialogData;

	// アクションの実行本体（Choice経由でもセリフ単体経由でも共通で使う）
	void ExecuteActionCore(EDialogActionType ActionType, const FString& ActionPayload, FName GrantFlag, bool bFadeOnGrantFlag, FName FlagToRemove, bool bFadeOnRemoveFlag, ETargetStat StatToChange, EStatTargetActor StatTargetActor, FName ExtraStatName, float StatChangeAmount);

	// --- 逐次表示システム用の変数と関数 ---

	/** 分割されたテキストを保持する配列 */
	UPROPERTY()
	TArray<FString> CurrentDialogLines;

	/** 現在何行目を表示しているかのインデックス */
	int32 CurrentLineIndex = 0;

	/** 現在の行のテキストをUIへ送信する関数 */
	void ShowCurrentLine();
};
