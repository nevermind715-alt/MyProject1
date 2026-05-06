#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MyProject1Types.h" // 先ほど作った構造体を使うため
#include "DialogComponent.generated.h"

// UIに通知するためのデリゲート（セリフデータと、喋っているNPCの情報を送る）
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

	// --- この選択肢の条件（精神値・フラグ）を満たしているかチェックする ---
	UFUNCTION(BlueprintPure, Category = "Dialog")
	bool CanSelectChoice(const FDialogChoice& Choice) const;

	// 選択肢がない会話で、画面をクリックして「次へ進む」時に呼ばれる関数 ---
	UFUNCTION(BlueprintCallable, Category = "Dialog")
	void AdvanceDialog();

	// 会話を強制終了する
	UFUNCTION(BlueprintCallable, Category = "Dialog")
	void CloseDialog();

	// UI側でこれにイベントをバインドする
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

	// アクションを実行する内部関数
	void ExecuteAction(const FDialogChoice& Choice);
};