#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "CharacterSystem/Interface/TargetableInterface.h"
#include "BaseCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UAbilitySystemComponent;
class UBaseAttributeSet;
class UGameplayEffect;
class UCharacterData;

UCLASS()
class PROJECTER_API ABaseCharacter : public ACharacter,  public IAbilitySystemInterface, public ITargetableInterface
{
	GENERATED_BODY()

public:
	ABaseCharacter();
	
	FORCEINLINE UCameraComponent* GetTopDownCameraComponent() const { return TopDownCameraComponent.Get(); }
	
	FORCEINLINE USpringArmComponent* GetCameraBoom() const { return CameraBoom.Get(); }

protected:
	virtual void BeginPlay() override;

	virtual void Tick( float DeltaTime ) override;
	
	virtual void PossessedBy(AController* NewController) override;
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
#pragma region Debug
public:
	UPROPERTY(EditAnywhere, Category = "Debug")
	bool bShowDebug = true;
	
#pragma endregion
	
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

#pragma region TargetableInterface
public:
	// [인터페이스 구현] 팀 정보 반환
	virtual ETeamType GetTeamType() const override;

	// [인터페이스 구현] 타겟팅 가능 여부 반환
	virtual bool IsTargetable() const override;
    
	// [인터페이스 구현] 하이라이트 (나중에 포스트 프로세스로 구현)
	// virtual void HighlightActor(bool bIsHighlight) override;

protected:
	// 팀 변수
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Team")
	ETeamType TeamID;
	
#pragma endregion
	
#pragma region AbilitySystemInterface
public:
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	
protected:
	virtual void OnRep_PlayerState() override;
	
#pragma endregion
	
#pragma region GAS
public:
	UFUNCTION(BlueprintCallable, Category = "GAS")
	float GetCharacterLevel() const;
	
	UFUNCTION(BlueprintCallable, Category = "GAS")
	float GetAttackRange() const;
	
public:
	UPROPERTY(ReplicatedUsing=OnRep_HeroData, EditAnywhere, BlueprintReadWrite, Category = "Data", meta = (ExposeOnSpawn = true))
	TObjectPtr<UCharacterData> HeroData;
	
	UPROPERTY(EditDefaultsOnly,Category = "GAS") 
	TSubclassOf<UGameplayEffect> InitStatusEffectClass;
	
protected:
	UFUNCTION()
	void OnRep_HeroData();

	void InitAbilitySystem(); // ASC 초기화
	void InitAttributes(); // AttributeSet 초기화
	void InitVisuals(); // 메시, 애니메이션 로드
	
#pragma endregion 

#pragma region MoveToLocation
public:
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_MoveToLocation(FVector TargetLocation); 
	
	UFUNCTION(Server, Reliable)
	void Server_StopMove();
	
	void MoveToLocation(FVector TargetLocation);
	void StopMove();
	
protected:
	void UpdatePathFollowing();
	void StopPathFollowing();

private:
	// 현재 추적 중인 경로 포인트들
	UPROPERTY()
	TArray<FVector> PathPoints;

	// 현재 목표 웨이포인트 인덱스
	int32 CurrentPathIndex;

	// 도착 판정 임계값 (제곱)
	const float ArrivalDistanceSq = 2500.f; 
	
	// 갱신 타이머
	float PathfindingTimer = 0.0f;
	
#pragma endregion
	
#pragma region Combat
public:
	// 타겟 설정 함수
	void SetTarget(AActor* NewTarget);
	
	// 공격 가능 사거리 확인
	void CheckCombatTarget(float DeltaTime);
	
protected:
	// 현재 타겟 (적)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	TObjectPtr<AActor> TargetActor;
#pragma endregion

#pragma region UI
public:
	UFUNCTION()
	void InitUI();

protected:
	// 미니맵용 씬 캡처 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI|Minimap")
	class USceneCaptureComponent2D* MinimapCaptureComponent;
#pragma endregion
};
