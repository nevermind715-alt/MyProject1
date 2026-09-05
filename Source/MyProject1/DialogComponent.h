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

	// StartDialogの本体。指定行が見つかって実際に会話を開始できた場合のみtrueを返す。
	// 呼び出し側が「会話を開始できた時だけ入力ロックする」等の判定に使えるようにするためのC++専用版。
	// StartDialogはBlueprint互換のためvoid・BlueprintCallableのまま維持し、この関数へ委譲する
	bool TryStartDialog(FName RowName, UDataTable* DialogTable, AActor* InNPC);

	// 選択肢が選ばれた時にUIから呼ばれる関数
	UFUNCTION(BlueprintCallable, Category = "Dialog")
	void SelectChoice(int32 ChoiceIndex);

	// --- その選択肢の条件（数値・フラグ）を満たしているかチェックする ---
	UFUNCTION(BlueprintPure, Category = "Dialog")
	bool CanSelectChoice(const FDialogChoice& Choice) const;

	// 選択肢のない会話で、画面をクリックして「次へ進む」時に呼ばれる関数
	UFUNCTION(BlueprintCallable, Category = "Dialog")
	void AdvanceDialog();

	// 今この瞬間、選択肢の入力待ち（最終ページかつ選択肢が1件以上）かどうか。
	// 「クリック/スペースで進む」系の全画面ウィジェット（WBP_DialogAdvance）を、
	// 選択肢表示中だけCollapsedにして選択肢へのクリックを通すために使う
	UFUNCTION(BlueprintPure, Category = "Dialog")
	bool AreChoicesActive() const;

	// 会話を終了させる
	UFUNCTION(BlueprintCallable, Category = "Dialog")
	void CloseDialog();

	// セリフ表示から選択肢表示までの遅延秒数（ほぼ同時に出ると不自然なため）。
	// 選択肢ボタンの表示自体はUI側（WBP_ChoiceMenu）が担当するため、ここではその際に使う秒数をBlueprintへ公開するだけ
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dialog")
	float ChoiceRevealDelay = 0.5f;

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
	void ExecuteActionCore(EDialogActionType ActionType, const FString& ActionPayload, FName GrantFlag, bool bFadeOnGrantFlag, FName FlagToRemove, bool bFadeOnRemoveFlag, ETargetStat StatToChange, EStatTargetActor StatTargetActor, FName ExtraStatName, float StatChangeAmount, FName ItemID, int32 ItemAmount, bool bAdvanceDailySequence);

	// --- 逐次表示システム用の変数と関数 ---

	/** 分割されたテキストを保持する配列 */
	UPROPERTY()
	TArray<FString> CurrentDialogLines;

	/** 現在何行目を表示しているかのインデックス */
	int32 CurrentLineIndex = 0;

	/** 現在の行のテキストをUIへ送信する関数 */
	void ShowCurrentLine();
};
