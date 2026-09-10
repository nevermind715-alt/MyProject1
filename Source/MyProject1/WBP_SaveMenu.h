#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MyProject1Types.h"
#include "WBP_SaveMenu.generated.h"

/**
 * セーブ／ロード兼用画面のC++基底クラス。
 * 他のサブメニュー（ステータス／装備／クエスト）と同じく、開閉はAMyProject1HUD::ToggleSaveMenuが管理する。
 * このクラスはスロット一覧の取得と、セーブ／ロードの実行、閉じる要求の中継だけを担当する。
 * 一覧の並べ方・上書き確認ダイアログの見た目はBP側（WBP_SaveMenu）で組む。
 */
UCLASS()
class MYPROJECT1_API UWBP_SaveMenu : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 一覧に並べるスロット情報（オートセーブ＋手動1〜5）を取得する。BPのConstructやセーブ後の再描画で呼ぶ。 */
	UFUNCTION(BlueprintCallable, Category = "Save")
	TArray<FSaveSlotDisplayInfo> GetSlotList() const;

	/** 指定スロットへセーブする。上書き確認はBP側で済ませてから呼ぶこと。
	 *  成功すると OnSlotSaved が新しいスロット情報付きで呼ばれる（画面は閉じない）。 */
	UFUNCTION(BlueprintCallable, Category = "Save")
	void ExecuteSaveToSlot(const FString& SlotName);

	/** 指定スロットからロードする。内部でOpenLevelされるため、成功時はこの画面ごと破棄される。
	 *  データが無いスロットを指定した場合は何もしない。 */
	UFUNCTION(BlueprintCallable, Category = "Save")
	void ExecuteLoadFromSlot(const FString& SlotName);

	/** 閉じる要求。HUDのToggleSaveMenuへ中継し、背後のコマンドメニューへ操作を戻す。 */
	UFUNCTION(BlueprintCallable, Category = "Save")
	void CloseSaveMenu();

protected:
	virtual void NativeConstruct() override;

	/** ウィジェット生成直後に、初期のスロット一覧を渡して呼ばれる。BP側でこれを受けて行を並べる。 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Save")
	void OnSlotListReady(const TArray<FSaveSlotDisplayInfo>& Slots);

	/** ExecuteSaveToSlotでのセーブが成功した直後に、そのスロットの最新情報付きで呼ばれる。
	 *  BP側でその行だけ更新し、「セーブしました」等のトーストを出す用。 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Save")
	void OnSlotSaved(const FSaveSlotDisplayInfo& UpdatedSlot);
};
