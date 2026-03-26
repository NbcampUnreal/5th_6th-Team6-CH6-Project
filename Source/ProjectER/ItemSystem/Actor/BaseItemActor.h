#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ItemSystem/Interface/I_ItemInteractable.h"
#include "BaseItemActor.generated.h"

class USphereComponent;
class UBaseItemData;
class UStaticMeshComponent;
class UPrimitiveComponent;

UCLASS()
class PROJECTER_API ABaseItemActor : public AActor, public II_ItemInteractable
{
	GENERATED_BODY()

public:
	ABaseItemActor();

public:
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 상호작용 인터페이스 구현
	virtual void PickupItem(APawn* InHandler) override;

	// 인벤토리에서 떨어질 때 데이터 초기화
	void InitializeFromItemData(UBaseItemData* InItemData, APawn* InDropperPawn = nullptr);

protected:
	// ItemData가 복제되면 클라이언트에서도 메시 갱신
	UFUNCTION()
	void OnRep_ItemData();

	void RefreshVisualFromItemData();

	// 바닥 아이템 메시의 물리 충돌은 끄고,
	// 클릭/자동줍기는 InteractionSphere가 맡도록 설정
	void ApplyWorldItemCollisionSettings();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item|Interaction")
	TObjectPtr<USphereComponent> InteractionSphere;

	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

public:
	// [기존] 여기만 ReplicatedUsing으로 바꿉니다
	UPROPERTY(EditAnywhere, BlueprintReadWrite, ReplicatedUsing = OnRep_ItemData, Category = "Item|Data")
	TObjectPtr<UBaseItemData> ItemData;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item|Mesh")
	TObjectPtr<UStaticMeshComponent> ItemMesh;

private:
	// [추가] 드랍 직후 즉시 다시 먹는 버그 방지용
	UPROPERTY()
	TObjectPtr<APawn> LastDropperPawn = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Item|Drop")
	float InitialPickupIgnoreTime = 0.25f;

	float DroppedAtTime = 0.f;
};