#include "CharacterSystem/GAS/AttributeSet/BaseAttributeSet.h"
#include "CharacterSystem/Character/BaseCharacter.h"
#include "CharacterSystem/Player/BasePlayerState.h"

#include "GameModeBase/GameMode/ER_InGameMode.h"
#include "GameModeBase/State/ER_PlayerState.h"

#include "GameplayEffect.h"
#include "GameplayEffectExtension.h"
#include "CharacterSystem/GameplayTags/GameplayTags.h"
#include "Net/UnrealNetwork.h"

UBaseAttributeSet::UBaseAttributeSet()
{
	InitLevel(1.0f);
	InitMaxLevel(18.0f);
	InitXP(0.0f);
	InitMaxXP(0.0f);
	InitHealth(0.0f);
	InitMaxHealth(0.0f);
	InitHealthRegen(0.0f);
	InitStamina(0.0f);
	InitMaxStamina(0.0f);
	InitStaminaRegen(0.0f);
	InitAttackPower(0.0f);
	InitAttackSpeed(0.0f);
	InitAttackRange(150.0f);
	InitCriticalChance(0.0f);
	InitCriticalDamage(0.0f);
	InitDefense(0.0f);
	InitMoveSpeed(0.0f);
	InitCooldownReduction(0.0f);
	InitTenacity(0.0f);
	
}

void UBaseAttributeSet::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME_CONDITION_NOTIFY(UBaseAttributeSet, Level, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBaseAttributeSet, MaxLevel, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBaseAttributeSet, XP, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBaseAttributeSet, MaxXP, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBaseAttributeSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBaseAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBaseAttributeSet, HealthRegen, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBaseAttributeSet, Stamina, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBaseAttributeSet, MaxStamina, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBaseAttributeSet, StaminaRegen, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBaseAttributeSet, AttackPower, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBaseAttributeSet, AttackSpeed, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBaseAttributeSet, AttackRange, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBaseAttributeSet, CriticalChance, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBaseAttributeSet, CriticalDamage, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBaseAttributeSet, Defense, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBaseAttributeSet, MoveSpeed, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBaseAttributeSet, CooldownReduction, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UBaseAttributeSet, Tenacity, COND_None, REPNOTIFY_Always);
}

void UBaseAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);
	
	// 체력 범위 제한 (0~MaxHealth)
	if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHealth());
	}
	
	// 스테미나 범위 제한 (0~MaxStamina)
	if (Attribute == GetStaminaAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxStamina());
	}

	if (Attribute == GetMaxHealthAttribute())
	{
		NewValue = FMath::Max(1.f, NewValue);

		if (NewValue < GetHealth())
		{
			SetHealth(NewValue);
		}
	}
	else if (Attribute == GetAttackPowerAttribute())
	{
		NewValue = FMath::Max(0.0f, NewValue);
	}
	else if (Attribute == GetDefenseAttribute())
	{
		NewValue = FMath::Max(0.0f, NewValue);
	}
	else if (Attribute == GetAttackSpeedAttribute())
	{
		NewValue = FMath::Max(0.0f, NewValue);
	}
	else if (Attribute == GetMoveSpeedAttribute())
	{
		NewValue = FMath::Max(0.0f, NewValue);
	}
}

void UBaseAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);
	// Health 속성이 변경되었는지 확인
	if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		// UE_LOG(LogTemp, Warning, TEXT("!!! HP 변경 감지됨 !!! 현재 HP: %f / %f"), GetHealth(), GetMaxHealth());
	}
	// 데미지(Damage : Data.Amount.Damage) 처리
	if (Data.EvaluatedData.Attribute == GetIncomingDamageAttribute())
	{
		const float LocalDamage = GetIncomingDamage();
		SetIncomingDamage(0.0f); // Meta Data 초기화 
		
		// 공격 대상 설정
		const FGameplayEffectContextHandle& Context =
			Data.EffectSpec.GetEffectContext();
		
		//AActor* Target = Context.GetInstigator(); // 인스티게이터로
		AActor* Target = Context.GetEffectCauser(); 
		
		if (LocalDamage > 0.0f)
		{
			const float OldHealth = GetHealth();
			const float NewHealth = OldHealth - LocalDamage;
			
			SetHealth(FMath::Clamp(NewHealth, 0.0f, GetMaxHealth()));
			// UE_LOG(LogTemp, Warning, TEXT("Hp %f / %f "),  GetHealth(), GetMaxHealth());

			// [전민성] 어시스트, 사망 판정 추가
			if (!GetWorld() || GetWorld()->GetNetMode() == NM_Client)
				return;

			// 피격자 및 공격자 PlayerState 획득
			ABaseCharacter* TargetChar = Cast<ABaseCharacter>(Data.Target.GetAvatarActor());
			if (!TargetChar)
				return;

			AER_PlayerState* TargetPS = Cast<AER_PlayerState>(TargetChar->GetPlayerState());
			if (!TargetPS)
				return;

			APlayerState* AttackerPS = nullptr;
			if (const FGameplayEffectContext* Ctx = Data.EffectSpec.GetEffectContext().Get())
			{
				if (AActor* InstigatorActor = Ctx->GetOriginalInstigator())
				{
					// Instigator가 Pawn/Character라면 PlayerState로
					if (APawn* Pawn = Cast<APawn>(InstigatorActor))
					{
						AttackerPS = Pawn->GetPlayerState();
					}
					else
					{
						// 혹시 PlayerState가 직접 들어오는 이상한 케이스면
						AttackerPS = Cast<APlayerState>(InstigatorActor);
					}
				}

				// 2) 그래도 없으면 EffectCauser에서 한번 더 시도 (투사체/스킬 액터 등)
				if (!AttackerPS)
				{
					if (AActor* Causer = Ctx->GetEffectCauser())
					{
						if (APawn* Pawn = Cast<APawn>(Causer))
						{
							AttackerPS = Pawn->GetPlayerState();
						}
					}
				}
			}
			
			const float Now = GetWorld()->GetTimeSeconds();

			if (AttackerPS && TargetPS && TargetPS != AttackerPS)
			{
				TargetPS->AddDamageContributor(AttackerPS, LocalDamage, Now);
			}

			if (OldHealth > 0.0f && NewHealth <= 0.0f)
			{
				UAbilitySystemComponent* TargetASC = TargetChar->GetAbilitySystemComponent();
				
				// 이미 빈사 상태(Down)였다면 -> 진짜 사망(Death)
				if (TargetASC && TargetASC->HasMatchingGameplayTag(ProjectER::State::Life::Down))
				{
					TargetChar->HandleDeath(); 
        
					// 기존 킬 로그 및 어시스트 처리 유지
					auto InGameMode = Cast<AER_InGameMode>(GetWorld()->GetAuthGameMode());
					if (!InGameMode) return;

					TArray<APlayerState*> OutAssists;
					if (TargetPS)
					{
						// 8초 안에 데미지를 줬으면 어시스트 판정
						TargetPS->GetAssists(Now, 8.f, AttackerPS, OutAssists);

						// 죽으면 기여 기록 초기화
						TargetPS->ResetDamageContrib();
					}
					InGameMode->NotifyPlayerDied(TargetChar, AttackerPS, OutAssists);
				}
				else // 살아있는 상태(Alive)였다면 -> 빈사 상태(Down) 진입
				{
					// 빈사 상태 전용 체력으로 세팅 (GetMaxHealth() * 0.5f 설정)
					SetHealth(GetMaxHealth() * 0.5f); 

					// 빈사 로직 실행 (상태 변환 및 GE 적용)
					TargetChar->HandleDown();
				}
			}
			else
			{
				// [필요 시, 피격 처리 추가 예정]
			}
		}
	}
	
	// 경험치(XP : Data.Amount.IncomingXP) 처리
	if (Data.EvaluatedData.Attribute == GetIncomingXPAttribute())
	{
		float LocalIncomingXP = GetIncomingXP();
		SetIncomingXP(0.0f); // 메타 속성 초기화

		if (LocalIncomingXP > 0.0f)
		{
			float CurrentXP = GetXP();
			float NewXP = CurrentXP + LocalIncomingXP;
			float CurrentMaxXP = GetMaxXP();
			float CurrentLevel = GetLevel();
			
			int32 LevelUpCount = 0;
			
			if (CachedMaxXPCurve) 
			{
				if (CurrentMaxXP <= 0.0f) 
				{
					CurrentMaxXP = CachedMaxXPCurve->Eval(CurrentLevel);
				}

				while (NewXP >= CurrentMaxXP && CurrentMaxXP > 0.0f)
				{
					NewXP -= CurrentMaxXP;
					CurrentLevel += 1.0f;
					LevelUpCount++;
                    
					// 캐싱된 포인터에서 바로 계산 (LoadSynchronous 제거됨!)
					CurrentMaxXP = CachedMaxXPCurve->Eval(CurrentLevel);
				}
			}
			else
			{
				// [로그 추가] 캐싱된 커브가 없음
				UE_LOG(LogTemp, Error, TEXT("[AttributeSet] CachedMaxXPCurve is NULL! Level Up Logic Skipped."));
			}

			// 남은 XP 적용 및 레벨 업
			SetXP(NewXP);
			SetLevel(CurrentLevel);
			
			// 레벨업이 발생 시 스탯 갱신 요청
			if (LevelUpCount > 0)
			{
				if (ABaseCharacter* TargetChar = Cast<ABaseCharacter>(Data.Target.GetAvatarActor()))
				{
					TargetChar->HandleLevelUp(); 
				}
			}
		}
	}
}

void UBaseAttributeSet::SetMaxXPCurve(UCurveTable* InTable, FName InRowName)
{
	SafeMaxXPCurveTable = InTable;
	if (SafeMaxXPCurveTable)
	{
		CachedMaxXPCurve = SafeMaxXPCurveTable->FindCurve(InRowName, FString());
	}
}

void UBaseAttributeSet::OnRep_Level(const FGameplayAttributeData& OldLevel)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaseAttributeSet, Level, OldLevel);
}

void UBaseAttributeSet::OnRep_MaxLevel(const FGameplayAttributeData& OldMaxLevel)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaseAttributeSet, MaxLevel, OldMaxLevel);
}

void UBaseAttributeSet::OnRep_XP(const FGameplayAttributeData& OldXP)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaseAttributeSet, XP, OldXP);
}

void UBaseAttributeSet::OnRep_MaxXP(const FGameplayAttributeData& OldMaxXP)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaseAttributeSet, MaxXP, OldMaxXP);
}

void UBaseAttributeSet::OnRep_Health(const FGameplayAttributeData& OldHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaseAttributeSet, Health, OldHealth);
}

void UBaseAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaseAttributeSet, MaxHealth, OldMaxHealth);
}

void UBaseAttributeSet::OnRep_HealthRegen(const FGameplayAttributeData& OldHealthRegen)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaseAttributeSet, HealthRegen, OldHealthRegen);
}

void UBaseAttributeSet::OnRep_Stamina(const FGameplayAttributeData& OldStamina)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaseAttributeSet, Stamina, OldStamina);
}

void UBaseAttributeSet::OnRep_MaxStamina(const FGameplayAttributeData& OldMaxStamina)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaseAttributeSet, MaxStamina, OldMaxStamina);
}

void UBaseAttributeSet::OnRep_StaminaRegen(const FGameplayAttributeData& OldStaminaRegen)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaseAttributeSet, StaminaRegen, OldStaminaRegen);
}

void UBaseAttributeSet::OnRep_AttackPower(const FGameplayAttributeData& OldAttackPower)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaseAttributeSet, AttackPower, OldAttackPower);
}

void UBaseAttributeSet::OnRep_AttackSpeed(const FGameplayAttributeData& OldAttackSpeed)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaseAttributeSet, AttackSpeed, OldAttackSpeed);
}

void UBaseAttributeSet::OnRep_AttackRange(const FGameplayAttributeData& OldAttackRange)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaseAttributeSet, AttackRange, OldAttackRange);
}

void UBaseAttributeSet::OnRep_SkillAmp(const FGameplayAttributeData& OldSkillAmp)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaseAttributeSet, SkillAmp, OldSkillAmp);
}

void UBaseAttributeSet::OnRep_CriticalChance(const FGameplayAttributeData& OldCriticalChance)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaseAttributeSet, CriticalChance, OldCriticalChance);
}

void UBaseAttributeSet::OnRep_CriticalDamage(const FGameplayAttributeData& OldCriticalDamage)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaseAttributeSet, CriticalDamage, OldCriticalDamage);
}

void UBaseAttributeSet::OnRep_Defense(const FGameplayAttributeData& OldDefense)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaseAttributeSet, Defense, OldDefense);
}

void UBaseAttributeSet::OnRep_MoveSpeed(const FGameplayAttributeData& OldMoveSpeed)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaseAttributeSet, MoveSpeed, OldMoveSpeed);
}

void UBaseAttributeSet::OnRep_CooldownReduction(const FGameplayAttributeData& OldCooldownReduction)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaseAttributeSet, MoveSpeed, OldCooldownReduction);
}

void UBaseAttributeSet::OnRep_Tenacity(const FGameplayAttributeData& OldTenacity)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UBaseAttributeSet, MoveSpeed, OldTenacity);
}


