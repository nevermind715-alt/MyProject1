#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MyProject1Types.h" // FDialogDataなどの構造体を使うため
#include "DialogComponent.generated.h"

// UIに通知するためのデリゲート（セリフデータと、話しているNPCの情報を送る）
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDialogUpdated, const FDialogData&, DialogData, AActor*, NPC);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDialogClosed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnHideChoices);

// ActionType=ShowTextDuringFade用。暗転して画面が真っ暗になった後の1行分のテキストをUIへ送る
// （名前欄などを持つ通常のOnDialogUpdatedとは別に、ナレーション専用の見た目をUI側で作れるようにするため）
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFadeNarrationLine, const FText&, Line);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnFadeNarrationClosed);

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

	// UI側でこれにイベントバインドする（ActionType=ShowTextDuringFade用。暗転中の1行が更新されるたびに呼ばれる）
	UPROPERTY(BlueprintAssignable, Category = "Dialog")
	FOnFadeNarrationLine OnFadeNarrationLine;

	// ActionType=ShowTextDuringFadeの全行を読み終え、明転処理に移った瞬間に呼ばれる（UI側の非表示用）
	UPROPERTY(BlueprintAssignable, Category = "Dialog")
	FOnFadeNarrationClosed OnFadeNarrationClosed;

	// GameInstance側から、暗転して画面が真っ暗になった瞬間に呼ばれる（RequestFadeThenShowNarration用）
	void BeginFadeNarration();

	// GameInstance側から、ナレーション読了後の明転が完了した瞬間に呼ばれ、NextDialogIDへ会話を継続する
	void ResumeAfterFadeNarration(FName NextDialogID);

private:
	// 現在使用中のデータテーブルとNPC
	UPROPERTY()
	UDataTable* CurrentTable;

	UPROPERTY()
	AActor* CurrentNPC;

	// 現在の会話データ
	FDialogData CurrentDialogData;

	// アクションの実行本体（Choice経由でもセリフ単体経由でも共通で使う）。
	// AnimSequenceRowPlayTargetはActionType=PlayAnimSequenceRow専用（セリフ単体経由の場合はFDialogChoiceを経由しないため、
	// 呼び出し側でEStatTargetActor::Playerを渡す＝現状セリフ単体からはPlayAnimSequenceRowのNPC再生を指定できない）。
	// AnimSequenceNPCMeshOverrideはActionType=PlayAnimSequence/TriggerEvent専用（FDialogData::AnimSequenceNPCMeshOverride参照。
	// 再生されるDT_AnimEventsのExtra参加者[ExtraPairings]のメッシュを差し替える）。
	// FadeNarrationText/NextDialogIDForNarrationはActionType=ShowTextDuringFade専用。
	// 戻り値：ShowTextDuringFadeで暗転ナレーションを開始した場合はtrue。呼び出し側（SelectChoice/ShowCurrentLine）は
	// この場合、通常のNextDialogID遷移・CloseDialogを行わず、ResumeAfterFadeNarrationに継続を委ねる
	bool ExecuteActionCore(EDialogActionType ActionType, const FString& ActionPayload, FName GrantFlag, bool bFadeOnGrantFlag, FName FlagToRemove, bool bFadeOnRemoveFlag, ETargetStat StatToChange, EStatTargetActor StatTargetActor, FName ExtraStatName, float StatChangeAmount, FName ItemID, int32 ItemAmount, bool bAdvanceDailySequence, EStatTargetActor AnimSequenceRowPlayTarget, const TSoftObjectPtr<class USkeletalMesh>& AnimSequenceNPCMeshOverride, const FText& FadeNarrationText, FName NextDialogIDForNarration, bool bHideTalkingNPCDuringAnimEvent);

	// --- 逐次表示システム用の変数と関数 ---

	/** 分割されたテキストを保持する配列（通常のセリフとShowTextDuringFadeのナレーションで共用） */
	UPROPERTY()
	TArray<FString> CurrentDialogLines;

	/** 現在何行目を表示しているかのインデックス */
	int32 CurrentLineIndex = 0;

	/** 現在の行のテキストをUIへ送信する関数 */
	void ShowCurrentLine();

	// --- 暗転中ナレーション（ShowTextDuringFade）用の状態 ---

	/** ExecuteActionCoreが暗転を要求してから、BeginFadeNarrationで実際に表示が始まるまでの間true。
	 *  この間はまだ真っ暗になっておらず入力ロックの合間なので、AdvanceDialog等の誤動作を防ぐガードに使う */
	bool bIsFadeNarrationPending = false;

	/** BeginFadeNarrationで表示を開始してから、全行読み終えるまでtrue */
	bool bIsShowingFadeNarration = false;

	/** BeginFadeNarrationで表示する、改行分割済みのナレーション行（ExecuteActionCoreで先に分割しておく） */
	TArray<FString> PendingNarrationLines;

	/** ナレーション終了後、明転を経て継続する次の会話ID（NoneならCloseDialog） */
	FName PendingNarrationNextDialogID;

	/** 現在のナレーション行をUIへ送信する（OnFadeNarrationLineをBroadcast） */
	void ShowFadeNarrationLine();

	/** ナレーション表示中、クリック/決定入力のたびにAdvanceDialogから呼ばれる */
	void AdvanceFadeNarrationLine();
};
