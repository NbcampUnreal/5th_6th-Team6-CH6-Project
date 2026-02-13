#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ItemSystem/Actor/BaseBoxActor.h"
#include "ItemSystem/Interface/I_ItemInteractable.h"
#include "LootableComponent.generated.h"

class UBaseItemData;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class PROJECTER_API ULootableComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULootableComponent();

protected:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void PickupItem();


public:
	// ========================================
	// BaseBoxActor 호환 인터페이스
	// ========================================

	/**
	 * 현재 루트 슬롯 리스트 가져오기
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Lootable")
	TArray<FLootSlot> GetCurrentItemList() const { return CurrentItemList; }

	/**
	 * 특정 슬롯의 아이템 데이터 가져오기
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Lootable")
	UBaseItemData* GetItemData(int32 SlotIndex) const;

	/**
	 * 슬롯 개수 감소 (BasePlayerController에서 호출)
	 */
	UFUNCTION(BlueprintCallable, Category = "Lootable")
	void ReduceItem(int32 SlotIndex);

	// ========================================
	// 루팅 초기화
	// ========================================

	/**
	 * 랜덤 아이템으로 루트 테이블 생성
	 */
	UFUNCTION(BlueprintCallable, Category = "Lootable")
	void InitializeRandomLoot();

	/**
	 * 특정 아이템 리스트로 루트 테이블 생성
	 */
	UFUNCTION(BlueprintCallable, Category = "Lootable")
	void InitializeWithItems(const TArray<UBaseItemData*>& Items);

	/**
	 * 루트 테이블 초기화 (빈 상태로)
	 */
	UFUNCTION(BlueprintCallable, Category = "Lootable")
	void ClearLoot();

	/**
	 * 루트 가능한 아이템이 남아있는지 확인
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Lootable")
	bool HasLootRemaining() const;

	// ========================================
	// 아이템 가져가기
	// ========================================

	/**
	 * 특정 슬롯의 아이템 가져가기
	 * @return 성공 여부
	 */
	UFUNCTION(BlueprintCallable, Category = "Lootable")
	bool TakeItem(int32 SlotIndex, class APawn* Taker);

	// ========================================
	// 델리게이트
	// ========================================

	/** 루트 테이블이 변경될 때 브로드캐스트 */
	DECLARE_MULTICAST_DELEGATE(FOnLootChanged);
	FOnLootChanged OnLootChanged;

	/** 모든 아이템이 루팅되었을 때 브로드캐스트 */
	DECLARE_MULTICAST_DELEGATE(FOnLootDepleted);
	FOnLootDepleted OnLootDepleted;

protected:
	/**
	 * 아이템 압축 정렬 (빈 슬롯을 뒤로)
	 */
	void CompactItemList();

	/**
	 * 리플리케이션 콜백
	 */
	UFUNCTION()
	void OnRep_CurrentItemList();

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

	/** 자동 초기화 여부 (BeginPlay 시 InitializeRandomLoot 자동 호출) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lootable|Setup")
	bool bAutoInitialize = false;

	/** 루팅 완료 시 오너 액터 자동 삭제 여부 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Lootable|Behavior")
	bool bDestroyOwnerWhenEmpty = false;

protected:
	/** 현재 루트 슬롯 리스트 (리플리케이션) */
	UPROPERTY(ReplicatedUsing = OnRep_CurrentItemList, VisibleAnywhere, BlueprintReadOnly, Category = "Lootable|Runtime")
	TArray<FLootSlot> CurrentItemList;
};