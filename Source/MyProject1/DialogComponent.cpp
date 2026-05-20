#include "DialogComponent.h"
#include "Kismet/GameplayStatics.h"
#include "MyProject1Character.h"
#include "QuestComponent.h"
#include "GameFramework/Character.h"
#include "MyProject1GameInstance.h"
#include "WarpPortal.h"

UDialogComponent::UDialogComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UDialogComponent::StartDialog(FName RowName, UDataTable* DialogTable, AActor* InNPC)
{
	if (!DialogTable) return;

	CurrentTable = DialogTable;
	CurrentNPC = InNPC;

	// データテーブルから行を取得
	FDialogData* Data = DialogTable->FindRow<FDialogData>(RowName, TEXT("DialogContext"));
	if (Data)
	{
		CurrentDialogData = *Data;

		// --- 【追加】テキストの分割処理 ---
		FString RawText = CurrentDialogData.DialogText.ToString();
		CurrentDialogLines.Empty();

		// Windows等の改行コード(\r\n)のズレを防ぐため、\r を消去してから \n で分割する
		RawText = RawText.Replace(TEXT("\r"), TEXT(""));

		// \n（改行）でテキストを分割。true にすることで、連続した空行を無視します。
		RawText.ParseIntoArray(CurrentDialogLines, TEXT("\n"), true);

		// 万が一テキストが空だった場合の安全対策
		if (CurrentDialogLines.Num() == 0)
		{
			CurrentDialogLines.Add(TEXT(""));
		}

		// 最初は0行目からスタート
		CurrentLineIndex = 0;
		// ------------------------------------

		// --- 効果音(SE)が設定されていれば鳴らす ---
		if (CurrentDialogData.DialogSE)
		{
			UGameplayStatics::PlaySound2D(GetWorld(), CurrentDialogData.DialogSE);
		}

		// ボイスが設定されていれば鳴らす
		if (CurrentDialogData.DialogVoice)
		{
			UGameplayStatics::PlaySound2D(GetWorld(), CurrentDialogData.DialogVoice);
		}

		// エモートが設定されていれば、NPCに再生させる
		if (CurrentDialogData.DialogEmote && CurrentNPC)
		{
			ACharacter* NPCCharacter = Cast<ACharacter>(CurrentNPC);
			if (NPCCharacter)
			{
				NPCCharacter->PlayAnimMontage(CurrentDialogData.DialogEmote);
			}
		}

		// --- 【追加】最初の1行目をUIに表示する ---
		ShowCurrentLine();
	}
}

void UDialogComponent::SelectChoice(int32 ChoiceIndex)
{
	if (!CurrentDialogData.Choices.IsValidIndex(ChoiceIndex)) return;

	
	const FDialogChoice& SelectedChoice = CurrentDialogData.Choices[ChoiceIndex];

	// アクションを実行
	ExecuteAction(SelectedChoice);

	// 次の会話IDがあれば移動、なければ終了（またはCloseアクションに任せる）
	FString NextIDStr = SelectedChoice.NextDialogID.ToString().TrimStartAndEnd();
	bool bHasNext = !SelectedChoice.NextDialogID.IsNone() && !NextIDStr.IsEmpty() && NextIDStr.ToLower() != TEXT("none");

	OnHideChoices.Broadcast();

	if (bHasNext)
	{
		StartDialog(SelectedChoice.NextDialogID, CurrentTable, CurrentNPC);
	}
	else
	{
		// 次のセリフがないなら、アクションの種類に関わらず必ず閉じる
		CloseDialog();
	}
}

void UDialogComponent::ExecuteAction(const FDialogChoice& Choice)
{
	AMyProject1Character* Player = Cast<AMyProject1Character>(GetOwner());
	if (!Player) return;

	// 1. メインのアクションを実行（受注、報告、UIを閉じるなど）
	switch (Choice.ActionType)
	{
	case EDialogActionType::AcceptQuest:
		if (Player->QuestComp) Player->QuestComp->AcceptQuest(FName(*Choice.ActionPayload));
		break;

	case EDialogActionType::ReportQuest:
		if (Player->QuestComp) Player->QuestComp->ReportQuest(FName(*Choice.ActionPayload));
		break;

	case EDialogActionType::AddFlag:
		// Payload（文字列）に書かれた名前を FName に変換して渡す
		Player->AddFlag(FName(*Choice.ActionPayload));
		break;

	case EDialogActionType::RemoveFlag:
		Player->RemoveFlag(FName(*Choice.ActionPayload));
		break;

	case EDialogActionType::Warp:
		if (UMyProject1GameInstance* GameInst = Cast<UMyProject1GameInstance>(GetWorld()->GetGameInstance()))
		{
			// 1. まずはDT_Dialogs（選択肢）に設定されたPayload（行き先）を取得する
			FName WarpIDToUse = FName(*Choice.ActionPayload);

			// 2. もしPayloadが「空欄」だったら、話しかけた相手（ポータル）の行き先を調べる！
			if (Choice.ActionPayload.IsEmpty() || WarpIDToUse.IsNone())
			{
				// 話している相手（CurrentNPC）がポータルかどうかチェック
				if (AWarpPortal* Portal = Cast<AWarpPortal>(CurrentNPC))
				{
					// ポータルなら、ポータル側に設定されている行き先を上書きで採用する！
					WarpIDToUse = Portal->TargetWarpID;
				}
			}

			// 3. 行き先が確定していればワープ実行
			if (!WarpIDToUse.IsNone())
			{
				GameInst->RequestWarp(WarpIDToUse, Player);
			}
			else
			{
				// 万が一どちらにも設定されていなかった時のエラーログ
				UE_LOG(LogTemp, Warning, TEXT("ワープIDが設定されていません！"));
			}
		}
		break;

	case EDialogActionType::Close:
		CloseDialog();
		break;

		// ※ ChangeStat は下で独立して処理するので、ここには書かなくてOKです
	default:
		break;
	}

	// 2. ★独立処理：ステータス変化の設定があれば、上のアクションとは「別枠」で必ず実行する！
	if (Choice.StatToChange != ETargetStat::None && Choice.StatChangeAmount != 0.0f)
	{
		float ChangeVal = Choice.StatChangeAmount;
		FString StatName = TEXT("不明なステータス");

		switch (Choice.StatToChange)
		{
		case ETargetStat::Favor:
			Player->MyStats.Favor += ChangeVal;
			StatName = TEXT("好感度");
			break;
		case ETargetStat::Fame:
			Player->MyStats.Fame += ChangeVal;
			StatName = TEXT("名声");
			break;
		case ETargetStat::Charm:
			Player->MyStats.Charm += ChangeVal;
			StatName = TEXT("魅力");
			break;

			// 他のステータスも必要ならここに追加可能
		case ETargetStat::CustomExtraStat:
			if (!Choice.ExtraStatName.IsNone())
			{
				// ★ Find は「探すだけ」。見つからなければ nullptr（空っぽ）を返す
				float* CurrentVal = Player->MyStats.ExtraStats.Find(Choice.ExtraStatName);

				if (CurrentVal)
				{
					// 見つかった場合だけ数値を足す
					*CurrentVal += ChangeVal;
					StatName = Choice.ExtraStatName.ToString();
				}
				else
				{
					// ★ 見つからなかった場合（タイポなどのミス）
					// 開発者向けにアウトプットログに黄色い警告文字を出す！
					UE_LOG(LogTemp, Warning, TEXT("【Dialog Error】 ExtraStat '%s' が MyStats に登録されていません！"), *Choice.ExtraStatName.ToString());

					// 画面上のログやステータス更新をスキップさせるために0にする
					ChangeVal = 0.0f;
				}
			}
			else
			{
				ChangeVal = 0.0f;
			}
			break;
		default:
			break;
		}

		// ログに結果を表示
		if (ChangeVal != 0.0f)
		{
			FString Sign = (ChangeVal > 0) ? TEXT("上がった") : TEXT("下がった");
			FString LogMsg = FString::Printf(TEXT("%sが %.0f %s。"), *StatName, FMath::Abs(ChangeVal), *Sign);
			Player->OnReceiveLogMessage(LogMsg, ELogMessageType::System);

			// UI（ステータス画面など）を更新する合図
			Player->NotifyStatsChanged();
		}
	}
}

void UDialogComponent::CloseDialog()
{
	OnDialogClosed.Broadcast();
}

void UDialogComponent::AdvanceDialog()
{
	// --- まだ同じRowの中に次の改行テキストが残っている場合 ---
	if (CurrentLineIndex < CurrentDialogLines.Num() - 1)
	{
		CurrentLineIndex++;
		ShowCurrentLine(); // 次の行を表示
		return;            // ★ここで処理を終わらせて、次のRowへ行くのを防ぐ
	}

	// --- 全ての行を読み終わっている場合の処理（既存のまま） ---
	// bEndDialogがTrueになっている、または次の会話IDが設定されていなければ会話を終了する
	if (CurrentDialogData.bEndDialog || CurrentDialogData.NextDialogID.IsNone())
	{
		CloseDialog();
	}
	else
	{
		StartDialog(CurrentDialogData.NextDialogID, CurrentTable, CurrentNPC);
	}
}

bool UDialogComponent::CanSelectChoice(const FDialogChoice& Choice) const
{
	AMyProject1Character* Player = Cast<AMyProject1Character>(GetOwner());
	if (!Player) return false;

	// 1. 精神（Mental）のチェック
	if (Player->MyStats.Mental < Choice.RequiredMental)
	{
		return false; // 精神が足りない
	}

	// 2. フラグのチェック（RequiredFlagが設定されている場合のみ）
	if (!Choice.RequiredFlag.IsNone())
	{
		if (!Player->HasFlag(Choice.RequiredFlag))
		{
			return false; // フラグを持っていない
		}
	}

	return true; // 全ての条件をクリア！
}

// --- 1行分のテキストをUIへ送信する関数 ---
void UDialogComponent::ShowCurrentLine()
{
	if (!CurrentDialogLines.IsValidIndex(CurrentLineIndex)) return;

	// UIへ送る用の一時データを作成（元の CurrentDialogData 自体は書き換えない）
	FDialogData DisplayData = CurrentDialogData;

	// テキスト部分だけを「現在の行」の文字列に差し替える
	DisplayData.DialogText = FText::FromString(CurrentDialogLines[CurrentLineIndex]);

	// ★ポイント：最後の行に到達するまでは、選択肢を出さないように配列を空にする
	// これにより、UI側は「選択肢がない普通のセリフだ」と認識してくれます。
	if (CurrentLineIndex < CurrentDialogLines.Num() - 1)
	{
		DisplayData.Choices.Empty();
		// まだ改行が残っている場合は、UIが勝手に会話を終わらせないように
		// ダミーのIDを入れて「次へ進むボタン（クリック）」を有効にさせる
		DisplayData.NextDialogID = FName(TEXT("DummyNextPage"));
		DisplayData.bEndDialog = false;
		// ------------------------
	}

	// UIに「更新したよ！」と合図を送る
	OnDialogUpdated.Broadcast(DisplayData, CurrentNPC);
}