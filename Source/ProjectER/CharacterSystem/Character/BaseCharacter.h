#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "BaseCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UAbilitySystemComponent;
class UBaseAttributeSet;
class UCharacterData;

UCLASS()
class PROJECTER_API ABaseCharacter : public ACharacter,  public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ABaseCharacter();
	
	FORCEINLINE UCameraComponent* GetTopDownCameraComponent() const { return TopDownCameraComponent.Get(); }
	
	FORCEINLINE USpringArmComponent* GetCameraBoom() const { return CameraBoom.Get(); }

protected:
	virtual void BeginPlay() override;
	
	virtual void PossessedBy(AController* NewController) override;
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
#pragma region Component
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCameraComponent> TopDownCameraComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USpringArmComponent> CameraBoom;
	
	UPROPERTY()
	TWeakObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;
	
	// [Inventory] 아이템 데이터 관리 (재사용 가능)
	// UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	// TObjectPtr<UMOBAInventoryComponent> InventoryComponent;

	// [Equipment] 무기/방어구 외형 부착 관리
	// UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	// TObjectPtr<UMOBAEquipmentComponent> EquipmentComponent;

	// [Camera] 탑뷰 카메라 제어 로직 분리
	// UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	// TObjectPtr<UMOBACameraComponent> MobaCameraComponent;
	
#pragma endregion

#pragma region Interface
public:
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	
protected:
	virtual void OnRep_PlayerState() override;
	
#pragma endregion
	
#pragma region Character GAS
public:
	UPROPERTY(ReplicatedUsing=OnRep_HeroData, EditAnywhere, BlueprintReadWrite, Category = "Data", meta = (ExposeOnSpawn = true))
	TObjectPtr<UCharacterData> HeroData;
	
protected:
	UFUNCTION()
	void OnRep_HeroData();

	void InitAbilitySystem(); // ASC 초기화
	void InitVisuals(); // 메시, 애니메이션 로드
	
#pragma endregion 


};
