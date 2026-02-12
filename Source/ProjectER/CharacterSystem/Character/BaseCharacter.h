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

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDeath);

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
	
#pragma endregion

#pragma region TargetableInterface
public:
	// 팀 정보 반환
	virtual ETeamType GetTeamType() const override;

	// 타겟팅 가능 여부 반환
	virtual bool IsTargetable() const override;
    
	// [인터페이스 구현] 하이라이트 (나중에 포스트 프로세스로 구현)
	// virtual void HighlightActor(bool bIsHighlight) override;
	
	UFUNCTION(BlueprintCallable, Server, Reliable)
	void Server_SetTeamID(ETeamType NewTeamID);
	
protected:
	UFUNCTION()
	void OnRep_TeamID();
	
protected:
	// 팀 변수
	UPROPERTY(ReplicatedUsing = OnRep_TeamID, EditAnywhere, BlueprintReadWrite, Category = "Team")
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
	// [추가] 레벨업 시 AttributeSet에서 호출
	virtual void HandleLevelUp();
	
	UFUNCTION(BlueprintCallable, Category = "GAS")
	float GetCharacterLevel() const;
	
	UFUNCTION(BlueprintCallable, Category = "GAS")
	float GetAttackRange() const;
	
protected:
	UFUNCTION()
	void OnRep_HeroData();

	void InitAbilitySystem(); // ASC 초기화
	void InitAttributes(); // AttributeSet 초기화
	void InitVisuals(); // 메시, 애니메이션 로드
	
public:
	UPROPERTY(ReplicatedUsing=OnRep_HeroData, EditAnywhere, BlueprintReadWrite, Category = "Data", meta = (ExposeOnSpawn = true))
	TObjectPtr<UCharacterData> HeroData;
	
	UPROPERTY(EditDefaultsOnly,Category = "GAS") 
	TSubclassOf<UGameplayEffect> InitStatusEffectClass;

	// 전민성 추가
	UPROPERTY(EditAnywhere, Category = "GAS")
	TSubclassOf<class UGameplayAbility> OpenAbilityClass;
	
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
	// 소켓 위치에서 타겟(혹은 정면) 방향 회전값 반환
	UFUNCTION(BlueprintCallable, Category = "Combat")
	FRotator GetCombatGazeRotation(FName SocketName);
	
	// 소켓의 위치를 반환하는 함수 (편의용)
	UFUNCTION(BlueprintCallable, Category = "Combat")
	FVector GetMuzzleLocation(FName SocketName);
	
	// 타겟 설정 함수
	void SetTarget(AActor* NewTarget);
	
	// 공격 가능 사거리 확인
	void CheckCombatTarget(float DeltaTime);
	
	//  공격 명령 (A키 입력)
	UFUNCTION(Server, Reliable)
	void Server_AttackMoveToLocation(FVector TargetLocation);
	
protected:
	UFUNCTION()
	void OnRep_TargetActor();
	
	UFUNCTION(Server, Reliable)
	void Server_SetTarget(AActor* NewTarget);
	
	// 이동 중 주변 적 검색 함수
	void ScanForEnemiesWhileMoving();
	
protected:
	// 현재 타겟 (적)
	UPROPERTY(ReplicatedUsing = OnRep_TargetActor, VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	TObjectPtr<AActor> TargetActor;
	
	//  공격 명령 (A키 입력) 상태 확인 플래그
	uint8 bIsAttackMoving : 1;
	
#pragma endregion

#pragma region DeathRevive
public:
	UFUNCTION(Server, Reliable)
	void Server_Revive(FVector RespawnLocation);
	
	// 사망 처리 호출
	virtual void HandleDeath();
	
	// 부활 처리 호출
	virtual void Revive(FVector RespawnLocation);
	
	UPROPERTY()
	FOnDeath OnDeath;

protected:
	// 사망 동기화
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_Death();
	
	// 부활 상태 동기화
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_Revive(FVector RespawnLocation);
	
protected:
	// 사망 모션 몽타주 : 내부 캐싱용
	UPROPERTY()
	TObjectPtr<UAnimMontage> DeadAnimMontage;
	
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
