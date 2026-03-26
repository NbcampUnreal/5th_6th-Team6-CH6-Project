#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "CharacterSystem/Interface/TargetableInterface.h"
#include "GameplayEffectTypes.h"
#include "LineOfSight/VisionData.h"
#include "BaseCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UNiagaraSystem;
class UAbilitySystemComponent;
class UBaseAttributeSet;
class UGameplayEffect;
class UCharacterData;
class UWidgetComponent; // 체력 바 머리 위에 띄우는 용
class UUI_HP_Bar; // 체력 바 머리 위에 띄우는 용

class UTopDownCameraComp;//main camera comp

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDeath);

UCLASS()
class PROJECTER_API ABaseCharacter : public ACharacter,  public IAbilitySystemInterface, public ITargetableInterface
{
	GENERATED_BODY()

public:
	ABaseCharacter();

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

	//replacement for camera comp
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Components", meta = (AllowPrivateAccess="true"))
	TObjectPtr<UTopDownCameraComp> TopDownCameraComp=nullptr;
	
	UPROPERTY()
	TWeakObjectPtr<UAbilitySystemComponent> AbilitySystemComponent=nullptr;
	
#pragma endregion

#pragma region TargetableInterface
public:
	// 팀 정보 반환
	virtual ETeamType GetTeamType() const override;

	// 타겟팅 가능 여부 반환
	UFUNCTION(BlueprintCallable, Category = "Targetable")
	virtual bool IsTargetable() const override;
    
	// [인터페이스 구현] 하이라이트 (나중에 포스트 프로세스로 구현)
	virtual void HighlightActor(bool bIsHighlight, int32 StencilValue = 0) override;
	
	UFUNCTION(BlueprintCallable, Server, Reliable)
	void Server_SetTeamID(ETeamType NewTeamID);
	
protected:
	UFUNCTION()
	void OnRep_TeamID();
	

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Team")
	EVisionChannel ConvertTeamToVisionChannel(ETeamType InTeamType);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Team")
	EVisionChannel GetVisionChannelFromPlayerStateComp();
	
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

	//BP Exposed Function
	UFUNCTION(BlueprintImplementableEvent)
	void OnPlayerStateChosen();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Ability")
	bool IsLocalPlayerPawn();
	
#pragma endregion
	
#pragma region GAS
public:
	// 레벨업 시 AttributeSet에서 호출
	UFUNCTION(BlueprintCallable, Category = "GAS")
	virtual void HandleLevelUp();
	
	UFUNCTION(BlueprintCallable, Category = "GAS")
	float GetCharacterLevel() const;
	
	UFUNCTION(BlueprintCallable, Category = "GAS")
	float GetAttackRange() const;
	
	// Tag 기반 몽타주 검색 및 반환
	UFUNCTION(BlueprintCallable, Category = "GAS")
	UAnimMontage* GetCharacterMontageByTag(FGameplayTag MontageTag);
	
	// 몽타주 Pre-Load
	void PreloadMontages();
	
	// 스킬(Gameplay Ability) 레벨업 호출
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "GAS|Skill")
	void Server_UpgradeSkill(FGameplayTag SkillTag);
	
protected:
	// 이동 속도 스탯 변경 감지 델리게이트
	virtual void OnMoveSpeedChanged(const FOnAttributeChangeData& Data);
	
	UFUNCTION()
	void OnRep_HeroData();

	void InitAbilitySystem(); // ASC 초기화
	void InitAttributes(); // AttributeSet 초기화
	void InitVisuals(); // 메시, 애니메이션 로드
	void InitPlayer(); // 플레이어(카메라 등) 로컬 초기화 통합 함수
	
public:
	UPROPERTY(ReplicatedUsing=OnRep_HeroData, EditAnywhere, BlueprintReadWrite, Category = "Data", meta = (ExposeOnSpawn = true))
	TObjectPtr<UCharacterData> HeroData;
	
	// 스탯 초기화 이펙트 클래스
	UPROPERTY(EditDefaultsOnly,Category = "GAS") 
	TSubclassOf<UGameplayEffect> InitStatusEffectClass;
	
	// 패시브 재생(Regen) 이펙트 클래스
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Life")
	TSubclassOf<UGameplayEffect> RegenEffectClass;
	
	// 기본 상태(Alive) 이펙트 클래스
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Life")
	TSubclassOf<UGameplayEffect> AliveStateEffectClass;

	// 빈사 상태(Down) 이펙트 클래스
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Life")
	TSubclassOf<UGameplayEffect> DownStateEffectClass;
	
	// 사망 상태(Death) 이펙트 클래스
	UPROPERTY(EditDefaultsOnly, Category = "GAS|Life")
	TSubclassOf<UGameplayEffect> DeathStateEffectClass;
	
	// 이동 상태(Moving) 이펙트 클래스
	UPROPERTY(EditDefaultsOnly, Category = "GAS|State")
	TSubclassOf<UGameplayEffect> MovingStateEffectClass;
	
	// 전민성 추가
	UPROPERTY(EditAnywhere, Category = "GAS")
	TSubclassOf<class UGameplayAbility> OpenAbilityClass;

	UPROPERTY(EditAnywhere, Category = "GAS")
	TSubclassOf<class UGameplayAbility> TeleportAbilityClass;
	
protected:
	// Anim Montage 하드 참조 캐싱 맵
	UPROPERTY(Transient)
	TMap<FGameplayTag, TObjectPtr<UAnimMontage>> CachedMontages;
	
	// GE_Moving 핸들 (추적용)
	FActiveGameplayEffectHandle MovingEffectHandle;
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
	
	// 몽타주 섹션 이름 반환 및 인덱스 관리
	UFUNCTION(BlueprintCallable, Category = "Combat|Combo")
	FName GetNextAutoAttackSectionName();
	
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
	
	// 평타 순환용 인덱스 (0, 1, 2)
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Combat|Combo")
	int32 AutoAttackIndex = 0;
	
	// 피격 이펙트 캐싱용 변수
	UPROPERTY(Transient)
	TObjectPtr<UNiagaraSystem> CachedBasicHitVFX;
	
#pragma endregion

#pragma region DeathRevive
public:
	UFUNCTION(Server, Reliable)
	void Server_Revive(FVector RespawnLocation);
	
	// 사망 처리 호출
	virtual void HandleDeath();
	
	// 부활 처리 호출
	UFUNCTION(BlueprintCallable, Category = "Revive")
	virtual void Revive(FVector RespawnLocation);
	
	// 빈사 상태 진입
	virtual void HandleDown();
	
	UPROPERTY()
	FOnDeath OnDeath;

protected:
	// 사망 동기화
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_Death();
	
	// 부활 상태 동기화
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_Revive(FVector RespawnLocation);
	
	// 빈사 상태 동기화
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_HandleDown();
	
protected:
	
#pragma endregion
	
#pragma region UI
public:
	UFUNCTION()
	void InitUI();

	UFUNCTION()
	void UpdateOverheadUI();

	UFUNCTION()
	void OnHealthChanged();
	UFUNCTION()
	void OnStaminaChanged();
	UFUNCTION()
	void OnLevelChanged();
protected:
	// 미니맵용 씬 캡처 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI|Minimap")
	class USceneCaptureComponent2D* MinimapCaptureComponent;
	
	// 미니맵용 얼굴 아이콘
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Minimap")
	class UStaticMeshComponent* MinimapIconMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI|Minimap")
	class UStaticMeshComponent* MinimapLineMesh;

	// 미니맵용 얼굴 마테리얼
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> MinimapIconMaterial;

	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> MinimapLineMaterial;

	// HP Bar를 담을 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI|Bar")
	UWidgetComponent* HP_MP_BarWidget;

	UPROPERTY()
	UUI_HP_Bar* HPBarWidgetInstance;

	FTimerHandle UILoadTimerHandle;


public:
	// 팀 구분해서 아이콘 색상 업데이트
	void UpdateMinimapVisuals(FLinearColor n_teamColor);

#pragma endregion

#pragma region Vision

	UFUNCTION(BlueprintPure, BlueprintCallable, Category = "Vision")//Helper for getting the vision channel from the PlayerStateComp
	EVisionChannel GetVisionChannelFromVisionPlayerStateComp();

	
	
#pragma endregion
	
};
