// LootableComponent.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LootableComponent.generated.h"

class UBaseItemData;

/**
 * FLootSlot
 * 루팅 슬롯 데이터 구조
 */
USTRUCT(BlueprintType)
struct FLootSlotData
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 ItemId = -1;         // ItemPool 인덱스 (-1이면 빈 슬롯)

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 Count = 0;           // 아이템 개수 (0이면 빈 슬롯)
};

/**
 * ULootableComponent
 * 어떤 액터든 루팅 가능하게 만들어주는 컴포넌트
 * 사용처: 몬스터 시체, 아이템 박스, 플레이어 시체 등
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class PROJECTER_API ULootableComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULootableComponent();

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	// ========================================
	// 루팅 초기화
	// ========================================

	/**
	 * 랜덤 아이템으로 루트 테이블 생성
	 * 몬스터 사망 시 호출
	 */
	UFUNCTION(BlueprintCallable, Category = "Lootable")
	void InitializeRandomLoot();

	/**
	 * 특정 아이템 리스트로 루트 테이블 생성
	 * 커스텀 드롭 아이템용
	 */
	UFUNCTION(BlueprintCallable, Category = "Lootable")
	void InitializeWithItems(const TArray<UBaseItemData*>& Items);

	/**
	 * 루트 테이블 초기화 (빈 상태로)
	 */
	UFUNCTION(BlueprintCallable, Category = "Lootable")
	void ClearLoot();

	// ========================================
	// 루팅 상호작용
	// ========================================

	/**
	 * 루팅 UI 열기 요청
	 * PlayerController에서 호출
	 */
	UFUNCTION(BlueprintCallable, Category = "Lootable")
	void BeginLoot(class APawn* Looter);

	/**
	 * 특정 슬롯의 아이템 가져가기
	 * @return 성공 여부
	 */
	UFUNCTION(BlueprintCallable, Category = "Lootable")
	bool TakeItem(int32 SlotIndex, class APawn* Taker);

	/**
	 * 특정 슬롯의 아이템 데이터 가져오기
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Lootable")
	UBaseItemData* GetItemData(int32 SlotIndex) const;

	/**
	 * 현재 루트 슬롯 리스트 가져오기
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Lootable")
	TArray<FLootSlotData> GetCurrentLootSlots() const { return CurrentLootSlots; }

	/**
	 * 루트 가능한 아이템이 남아있는지 확인
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Lootable")
	bool HasLootRemaining() const;

	// ========================================
	// 델리게이트
	// ========================================

	/** 루트 테이블이 변경될 때 브로드캐스트 (UI 업데이트용) */
	DECLARE_MULTICAST_DELEGATE(FOnLootChanged);
	FOnLootChanged OnLootChanged;

	/** 모든 아이템이 루팅되었을 때 브로드캐스트 */
	DECLARE_MULTICAST_DELEGATE(FOnLootDepleted);
	FOnLootDepleted OnLootDepleted;

protected:
	// ========================================
	// 내부 함수
	// ========================================

	/**
	 * 아이템 압축 정렬 (빈 슬롯을 뒤로)
	 */
	void CompactLootSlots();

	/**
	 * 슬롯 개수 감소 처리
	 */
	void ReduceSlot(int32 SlotIndex);

	/**
	 * 리플리케이션 콜백
	 */
	UFUNCTION()
	void OnRep_CurrentLootSlots();

public:
	// ========================================
	// 설정 가능한 프로퍼티
	// ========================================

	/** 루팅 가능한 아이템 풀 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lootable|Setup")
	TArray<TObjectPtr<UBaseItemData>> ItemPool;

	/** 최대 슬롯 개수 (기본 10칸) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lootable|Setup")
	int32 MaxSlots = 10;

	/** 최소 드롭 아이템 개수 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lootable|Setup")
	int32 MinLootCount = 1;

	/** 최대 드롭 아이템 개수 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lootable|Setup")
	int32 MaxLootCount = 3;

	/** 루팅 완료 시 오너 액터 자동 삭제 여부 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lootable|Behavior")
	bool bDestroyOwnerWhenEmpty = false;

	/** 루팅 완료 시 컴포넌트 비활성화 여부 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lootable|Behavior")
	bool bDeactivateWhenEmpty = true;

protected:
	/** 현재 루트 슬롯 리스트 (리플리케이션) */
	UPROPERTY(ReplicatedUsing = OnRep_CurrentLootSlots, VisibleAnywhere, BlueprintReadOnly, Category = "Lootable|Runtime")
	TArray<FLootSlotData> CurrentLootSlots;

	/** 활성화 상태 (루팅 가능 여부) */
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Lootable|Runtime")
	bool bCanLoot = false;
};