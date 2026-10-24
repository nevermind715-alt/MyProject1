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

class UWBP_ChoiceMenu;
class UWBP_DialogAdvance;
class UWBP_FadeNarration;

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

	// 現在使用中の会話データテーブル。別マップへのワープでこのコンポーネントが破棄される前に、
	// GameInstanceが再開用に控えておくために使う（C++専用）
	UDataTable* GetCurrentTable() const { return CurrentTable; }

	// 現在の話者名（QuestNPC相手ならそのNPCName、別マップのワープ後に再開した会話なら引き継いだ名前）。
	// 別マップへのワープでは話しかけていたNPCが新しいレベルに存在しないため、名前だけを控えて引き継ぐのに使う（C++専用）
	FString GetCurrentSpeakerName() const;
	void SetResumedSpeakerName(const FString& SpeakerName) { ResumedSpeakerName = SpeakerName; bResumedAcrossLevel = true; }

	// 話しかけているNPCのEventDistributorComponentが持つ抽選プール情報（データテーブルとプール行名）。
	// 別マップへのワープ後はNPCが新しいレベルに存在せずTriggerEventのイベント抽選ができなくなるため、
	// ワープ前にGameInstanceがこれを控え、再開後にSetResumedEventPoolSourceで引き継ぐ（C++専用）。
	// NPCがいない（引き継ぎ済みの会話をさらに別マップへ移す場合）は引き継いだ値を返す。取得できなければfalse
	bool GetEventPoolSourceForResume(TSoftObjectPtr<UDataTable>& OutTable, FName& OutPoolID) const;
	void SetResumedEventPoolSource(const TSoftObjectPtr<UDataTable>& InTable, FName InPoolID) { ResumedEventPoolTable = InTable; ResumedEventPoolID = InPoolID; }

	// セリフ表示から選択肢表示までの遅延秒数（ほぼ同時に出ると不自然なため）。
	// 選択肢ボタンの表示自体はUI側（WBP_ChoiceMenu）が担当するため、ここではその際に使う秒数をBlueprintへ公開するだけ
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dialog")
	float ChoiceRevealDelay = 0.5f;

	// 会話UI（ログ出力・選択肢・クリックで進めるウィジェット・暗転ナレーション）はこのコンポーネント自身がC++で生成・管理する
	// （HUDの DialogChoiceMenuClass / DialogAdvanceClass / FadeNarrationClass が必要）。以下のデリゲートはその上で追加のUIを
	// 作りたい場合の通知用に引き続きBroadcastされるが、標準の会話UIをBP側でも購読すると二重に表示されるため購読しないこと
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

protected:
	virtual void BeginPlay() override;

private:
	// 現在使用中のデータテーブルとNPC
	UPROPERTY()
	UDataTable* CurrentTable;

	UPROPERTY()
	AActor* CurrentNPC;

	// 別マップのワープ後に再開した会話で、CurrentNPCの代わりにログへ出す話者名（SetResumedSpeakerName参照。CloseDialogで消える）
	FString ResumedSpeakerName;
	bool bResumedAcrossLevel = false;

	// 別マップのワープ後に再開した会話で、CurrentNPCの代わりにTriggerEventの抽選に使う抽選プール情報
	// （SetResumedEventPoolSource参照。CloseDialogで消える）
	TSoftObjectPtr<UDataTable> ResumedEventPoolTable;
	FName ResumedEventPoolID;

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
	bool ExecuteActionCore(EDialogActionType ActionType, const FString& ActionPayload, FName GrantFlag, bool bFadeOnGrantFlag, FName FlagToRemove, bool bFadeOnRemoveFlag, ETargetStat StatToChange, EStatTargetActor StatTargetActor, FName ExtraStatName, float StatChangeAmount, FName ItemID, int32 ItemAmount, bool bAdvanceDailySequence, EStatTargetActor AnimSequenceRowPlayTarget, const TSoftObjectPtr<class USkeletalMesh>& AnimSequenceNPCMeshOverride, const FText& FadeNarrationText, FName NextDialogIDForNarration, bool bHideTalkingNPCDuringAnimEvent, const TArray<FEventAction>& AdditionalActions);

	/** ActionType=TriggerEventの実行本体（会話行・選択肢のActionTypeとAdditionalActionsの両方から呼ぶ） */
	void TriggerEventForCurrentNPC(const FString& EventPoolIDPayload, const TSoftObjectPtr<class USkeletalMesh>& ExtraParticipantMeshOverride, bool bHideContextActorDuringAnimEvent);

	/** AdditionalActions（FDialogData/FDialogChoice）を上から順に実行する。話しかけている相手（CurrentNPC）がコンテキストになる */
	void ExecuteAdditionalActions(const TArray<FEventAction>& Actions);

	// --- 行ごとのループAnimEvent（FDialogData::LoopAnimEventID）用 ---

	/** ループ用AnimEventを、会話の進行を待たずに再生開始する。IDが空なら何もしない（再生中のものはそのまま継続）。
	 *  同じIDが既に再生中ならそのまま継続し、別のIDが再生中なら先に正規の後片付けで止めてから切り替える */
	void StartRowLoopAnimEvent(FName LoopAnimEventID);

	/** StartRowLoopAnimEventで開始したAnimEventが再生中なら、後片付け込みで即座に止める */
	void StopRowLoopAnimEvent();

	/** StartRowLoopAnimEventで再生を開始した、現在再生中のAnimEventID（なければNone）。
	 *  別のAnimEvent（PlayAnimSequenceアクション等）に置き換わった場合は、そちらを誤って止めないようNoneへ戻す */
	FName ActiveLoopAnimEventID;

	// --- 逐次表示システム用の変数と関数 ---

	/** 分割されたテキストを保持する配列（通常のセリフとShowTextDuringFadeのナレーションで共用） */
	UPROPERTY()
	TArray<FString> CurrentDialogLines;

	/** 現在何行目を表示しているかのインデックス */
	int32 CurrentLineIndex = 0;

	/** 現在の行のテキストをUIへ送信する関数 */
	void ShowCurrentLine();

	/** 選択肢なしのセリフ行に直接設定されたアクション（ActionType等）を実行する。戻り値はExecuteActionCoreと同じ */
	bool ExecuteRowAction();

	/** 選択肢なしの最終ページで、アクション付きの行がまだアクションを実行せずクリック待ちの間true。
	 *  最後のセリフのログと、アクションが出すログ（クエスト受注など）が同時に出て読みにくくなるのを避けるため、
	 *  セリフを表示した後、もう一度クリック/決定入力されてからAdvanceDialogが実行する */
	bool bRowActionPending = false;

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

	/** 暗転で真っ暗になった瞬間（BeginFadeNarration）に適用するステータス変化。ExecuteActionCoreが暗転要求時に保存する */
	ETargetStat PendingNarrationStatToChange = ETargetStat::None;
	EStatTargetActor PendingNarrationStatTargetActor = EStatTargetActor::Player;
	FName PendingNarrationExtraStatName;
	float PendingNarrationStatChangeAmount = 0.0f;

	/** 同じく真っ暗になった瞬間（BeginFadeNarration）に実行するフラグ付与／消去と日替わり会話の進行。ExecuteActionCoreが暗転要求時に保存する。
	 *  暗転は既に起きているため、bFadeOnGrantFlag／bFadeOnRemoveFlagの追加の暗転は挟まず即時に付与／消去する */
	FName PendingNarrationGrantFlag;
	FName PendingNarrationRemoveFlag;
	bool bPendingNarrationAdvanceDailySequence = false;

	/** 同じく真っ暗になった瞬間（BeginFadeNarration）に実行する追加アクション。ExecuteActionCoreが暗転要求時に保存する */
	TArray<FEventAction> PendingNarrationAdditionalActions;

	/** 現在のナレーション行をUIへ送信する（OnFadeNarrationLineをBroadcast） */
	void ShowFadeNarrationLine();

	/** ナレーション表示中、クリック/決定入力のたびにAdvanceDialogから呼ばれる */
	void AdvanceFadeNarrationLine();

	// --- 会話UI（旧BP_ThirdPersonCharacterのOnDialogUpdated/OnDialogClosed/OnHideChoicesイベントの移植） ---

	/** 選択肢一覧（会話中は使い回し、会話終了で消す） */
	UPROPERTY()
	UWBP_ChoiceMenu* ChoiceMenuWidget = nullptr;

	/** クリック／スペースで会話を進める全画面ウィジェット（会話中は1つだけ使い回し、会話終了で消す） */
	UPROPERTY()
	UWBP_DialogAdvance* DialogAdvanceWidget = nullptr;

	/** 暗転ナレーションの本文表示。最初に必要になった時に作り、以降は使い回す */
	UPROPERTY()
	UWBP_FadeNarration* FadeNarrationWidget = nullptr;

	/** ShowCurrentLineが1ページ分を表示するたびに呼ぶ。NPC名付きのログ出力、選択肢／「進む」ウィジェットの表示、入力ロックを行う */
	void UpdateDialogUI(const FDialogData& DisplayData, AActor* NPC);

	/** 会話UIを片付ける（選択肢と「進む」ウィジェットを消し、入力ロックとマウスカーソルを通常に戻す） */
	void CloseDialogUI();

	/** 選択肢が選ばれた直後に呼ぶ。選択肢を隠す */
	void HideChoicesUI();

	/** 会話用の入力モード（UI操作可＋カーソル表示）にして、指定ウィジェットへフォーカスを移し、プレイヤーを入力ロックする */
	void ApplyDialogInputMode(class UUserWidget* WidgetToFocus);

	/** 「進む」ウィジェットが無ければ（または画面から外れていれば）作って画面に出す。HUDにクラス指定が無ければnullptrのまま */
	void EnsureDialogAdvanceWidget();

	class APlayerController* GetOwnerPlayerController() const;
	class AMyProject1HUD* GetOwnerHUD() const;

	/** OnFadeNarrationLine/OnFadeNarrationClosedの購読先（イベント／睡眠ナレーションなどGameInstanceが直接Broadcastする分も含めて受ける） */
	UFUNCTION()
	void HandleFadeNarrationLineUI(const FText& Line);

	UFUNCTION()
	void HandleFadeNarrationClosedUI();
};
