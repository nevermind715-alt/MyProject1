#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MyProject1Types.h" // FActiveTimeWindow
#include "AmbientWalkerNPC.generated.h"

/**
 * 会話もなく、ただ歩くだけの通行人NPC（雰囲気づくり用）。
 * レベルに置いたSplineの先頭(A)から末尾(B)へ歩き、Bに着くと消えて、待ち時間のあと再びAから出てくる。
 * WalkerCountを増やすと、同じスプラインに一定間隔（WalkerSpacing）でAから次々と出てくる複数体になる。
 *
 * 低負荷のため、AI・NavMesh・CharacterMovement・Capsule・AnimBPは一切使わない。
 * 位置は「ワールド時間から計算した、スプライン上の距離」で決まる（状態を持たない）。
 * そのため、プレイヤーから遠い間はTick・描画・アニメをすべて止めても、近づいた時に正しい位置から現れる。
 *
 * 使い方：
 * 1. レベルに配置し、Spline（Route）のポイントを置く（先頭=A、末尾=B。XYの道筋だけでよい。地面にはbSnapToGroundで合わせる）
 * 2. Meshに表示したいSkeletalMeshを設定し、WalkAnimationに歩行アニメを設定する
 * 3. WalkSpeedを歩行アニメの歩幅に合わせる（合わないと足が滑る）
 * 4. 複数体にしたい時はWalkerCountとWalkerSpacingを設定する（2体目以降はMeshと同じ見た目になる）
 */
UCLASS()
class MYPROJECT1_API AAmbientWalkerNPC : public AActor
{
	GENERATED_BODY()

public:
	AAmbientWalkerNPC();

	virtual void Tick(float DeltaSeconds) override;

protected:
	virtual void BeginPlay() override;

public:
	// --- コンポーネント ---

	/** 歩くルート。先頭のポイントがA（出現位置）、末尾のポイントがB（消える位置） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AmbientWalker")
	class USplineComponent* Route;

	/** 歩く見た目本体（1体目）。コリジョンなし（プレイヤーはすり抜ける）。レベルに置いたインスタンスごとにメッシュを設定できる。
	    WalkerCountが2以上の時は、2体目以降がゲーム開始時にこのメッシュ・マテリアル・スケールの複製として作られる */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AmbientWalker")
	class USkeletalMeshComponent* Mesh;

	// --- 設定 ---

	/** AnimBlueprintを使わず、単純にループ再生する歩行アニメーションシーケンス */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AmbientWalker")
	class UAnimSequence* WalkAnimation;

	/** WalkAnimationの再生速度（1.0=等倍）。歩幅は変わらず足の動きだけが速く／遅くなるので、足の滑りはWalkSpeedとこの値の両方で合わせる */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AmbientWalker", meta = (ClampMin = "0.1"))
	float WalkAnimationPlayRate = 1.0f;

	/** 歩く速さ（cm/秒）。歩行アニメの歩幅に合わせる。合わないと足が滑って見える */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AmbientWalker", meta = (ClampMin = "1.0"))
	float WalkSpeed = 150.0f;

	/** Bで消えてから、次にAから出てくるまでの時間（秒）。0なら消えた瞬間にAから再出現する。
	    WalkerCountが2以上の時は、各個体の1周の長さが「歩行時間＋この値」と「WalkerCount × WalkerSpacing」の長いほうになる */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AmbientWalker", meta = (ClampMin = "0.0"))
	float RespawnInterval = 5.0f;

	/** 歩行サイクルの開始位置をずらす時間（秒）。同じルートに別のアクターを置く時、アクターごとに変えて重ならないようにする */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AmbientWalker")
	float StartOffset = 0.0f;

	/** プレイヤーからこの距離（cm）より遠い間は、表示・Tick・アニメをすべて止める */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AmbientWalker", meta = (ClampMin = "0.0"))
	float ActiveDistance = 6000.0f;

	/** 進行方向に対するメッシュの向きの補正（Yaw、度）。メッシュが横を向いて歩く場合に調整する（UEの標準的なキャラは-90） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AmbientWalker")
	float MeshYawOffset = -90.0f;

	/** ゲーム内時間による出現切り替え（例：18時〜22時だけ歩いている）。OFFなら常に歩く。
	    時間帯の外では全員が消え、時間帯に入ると（経過時間に応じた位置から）歩き始める */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AmbientWalker|Time Visibility")
	FActiveTimeWindow ActiveTimeWindow;

	// --- 複数体 ---

	/** 同じスプラインを歩く人数。1なら今までどおり1体。2体目以降はMeshの複製として、ゲーム開始時に作られる */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AmbientWalker|Multiple", meta = (ClampMin = "1", UIMax = "20"))
	int32 WalkerCount = 1;

	/** WalkerCountが2以上の時、次の1人がAから出てくるまでの間隔（秒）。
	    人数 × この値 が1周の長さ以上なら等間隔に流れ続ける。短いと、全員が出た後に「RespawnInterval」の間だけ誰も居ない時間ができる */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AmbientWalker|Multiple", meta = (ClampMin = "0.1", EditCondition = "WalkerCount > 1"))
	float WalkerSpacing = 10.0f;

	// --- 地面追従 ---

	/** trueなら、歩いている間だけ約0.1秒ごとに足元から真下へトレースして、地面の高さに合わせる（起伏のある道路用）。
	    スプラインの高さは使わず、XYの道筋だけを使う。トレースが地面に当たらない間は、スプラインの高さのまま歩く */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AmbientWalker|Ground")
	bool bSnapToGround = true;

	/** 地面トレースの開始位置を、スプライン上の高さからどれだけ上にするか（cm）。道路がスプラインより高い所を通る時に大きくする */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AmbientWalker|Ground", meta = (EditCondition = "bSnapToGround", ClampMin = "0.0"))
	float GroundTraceUp = 200.0f;

	/** 地面トレースの終了位置を、スプライン上の高さからどれだけ下にするか（cm）。道路がスプラインより低い所を通る時に大きくする */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AmbientWalker|Ground", meta = (EditCondition = "bSnapToGround", ClampMin = "0.0"))
	float GroundTraceDown = 600.0f;

private:
	/** 1人ぶんの実行時の状態。Meshは1体目がMesh、2体目以降がCloneMeshesの要素（どちらもこのアクターが所有する） */
	struct FWalkerRuntime
	{
		USkeletalMeshComponent* MeshComp = nullptr;

		/** 表示中（歩行中かつプレイヤーが近い）か */
		bool bActive = false;

		/** 地面トレースの最後の実行時刻（ワールド時間）。トレースは約0.1秒ごとに間引く。
		    大きな負の値は「まだ一度も実行していない（次のPlaceMeshで必ずトレースする）」の意味 */
		float LastGroundTraceTime = -1000.0f;

		/** 直近のトレースで地面に当たったか。当たらなければスプラインの高さを使う */
		bool bGroundHit = false;

		/** 直近のトレースで当たった地面の高さ */
		float TargetGroundZ = 0.0f;

		/** 実際にメッシュへ使っている地面の高さ（TargetGroundZへなめらかに追従する） */
		float CurrentGroundZ = 0.0f;

		/** CurrentGroundZが有効か。表示を始める瞬間（または地面に当たっていなかった後）は補間せず、地面の高さへ即座に合わせる */
		bool bHasCurrentGroundZ = false;
	};

	/** 全員ぶんの状態。BeginPlayで作る（0番がMesh） */
	TArray<FWalkerRuntime> Walkers;

	/** 2体目以降のメッシュ（BeginPlayで作る複製）。GC対象から外れないよう保持する */
	UPROPERTY(Transient)
	TArray<TObjectPtr<USkeletalMeshComponent>> CloneMeshes;

	/** 表示中の人数。0になったらActorのTickを止める */
	int32 ActiveWalkerCount = 0;

	/** WalkerIndex番目の今のワールド時間での歩行状態を求める。歩行中ならtrueを返し、OutDistanceにスプライン上の進行距離を入れる（Bで消えている間はfalse） */
	bool EvaluateWalk(int32 WalkerIndex, float& OutDistance) const;

	/** WalkerIndex番目のメッシュを、スプライン上の進行距離に合わせて配置する。bSnapToGround時は高さを地面に合わせる（DeltaSecondsは高さの補間に使う） */
	void PlaceMesh(int32 WalkerIndex, float Distance, float DeltaSeconds);

	/** WalkerIndex番目のメッシュの表示を切り替える。表示中の人数に応じてActorのTickも切り替える（遠い・Bで消えている間はfalse） */
	void SetWalkerActive(int32 WalkerIndex, bool bNewActive);

	/** Meshの設定を引き継いだ2体目以降のメッシュを作る */
	USkeletalMeshComponent* CreateCloneMesh();

	/** 一定間隔で呼ばれ、時間帯の内外と、全員の「歩行中かつプレイヤーが近いか」を判定して、止まっているメッシュの表示/Tick再開を行う */
	void UpdateActivation();

	FTimerHandle ActivationTimerHandle;
};
