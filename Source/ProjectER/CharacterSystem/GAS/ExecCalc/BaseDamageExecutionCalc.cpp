#include "CharacterSystem/GAS/ExecCalc/BaseDamageExecutionCalc.h"
#include "CharacterSystem/GAS/AttributeSet/BaseAttributeSet.h"
#include "CharacterSystem/GameplayTags/GameplayTags.h"

// 캡처할 속성 정의 (헬퍼 구조체)
struct FDamageStatics
{
	// 캡처할 속성 정의 (매크로 활용 가능)
	DECLARE_ATTRIBUTE_CAPTUREDEF(Defense);
	DECLARE_ATTRIBUTE_CAPTUREDEF(AttackPower); 
	DECLARE_ATTRIBUTE_CAPTUREDEF(IncomingDamage);
	DECLARE_ATTRIBUTE_CAPTUREDEF(CriticalChance);
	DECLARE_ATTRIBUTE_CAPTUREDEF(CriticalDamage);

	FDamageStatics()
	{
		// Target의 방어력을 가져옴 (Snapshot: false -> 적용 시점의 실시간 값 사용)
		DEFINE_ATTRIBUTE_CAPTUREDEF(UBaseAttributeSet, Defense, Target, false);
        
		// Source의 공격력을 가져옴
		DEFINE_ATTRIBUTE_CAPTUREDEF(UBaseAttributeSet, AttackPower, Source, true);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UBaseAttributeSet, CriticalChance, Source, true);
		DEFINE_ATTRIBUTE_CAPTUREDEF(UBaseAttributeSet, CriticalDamage, Source, true);
	}
};

static const FDamageStatics& DamageStatics()
{
	static FDamageStatics DStatics;
	return DStatics;
}

UBaseDamageExecutionCalc::UBaseDamageExecutionCalc()
{
	// 캡처할 속성 등록
	RelevantAttributesToCapture.Add(DamageStatics().DefenseDef);
	RelevantAttributesToCapture.Add(DamageStatics().AttackPowerDef);
	RelevantAttributesToCapture.Add(DamageStatics().CriticalChanceDef);
	RelevantAttributesToCapture.Add(DamageStatics().CriticalDamageDef);
}

void UBaseDamageExecutionCalc::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams,
	FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	Super::Execute_Implementation(ExecutionParams, OutExecutionOutput);
	
	// ASC 캐싱
    UAbilitySystemComponent* TargetASC = ExecutionParams.GetTargetAbilitySystemComponent();
    UAbilitySystemComponent* SourceASC = ExecutionParams.GetSourceAbilitySystemComponent();

    // 태그 집계
    const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();
    const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
    const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

    FAggregatorEvaluateParameters EvaluationParameters;
    EvaluationParameters.SourceTags = SourceTags;
    EvaluationParameters.TargetTags = TargetTags;

    // Attribute 값 추출 (Capture)
    float Defense = 0.f;
    ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().DefenseDef, EvaluationParameters, Defense);
    Defense = FMath::Max<float>(Defense, 0.0f); // 방어력은 음수가 되지 않도록
	
    float AttackPower = 0.f;
    ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().AttackPowerDef, EvaluationParameters, AttackPower);
	AttackPower = FMath::Max<float>(AttackPower, 0.0f);
	
	float CriticalChance = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().CriticalChanceDef, EvaluationParameters, CriticalChance);
	CriticalChance = FMath::Clamp<float>(CriticalChance, 0.0f, 100.0f); // 0~100% 제한

	float CriticalDamage = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().CriticalDamageDef, EvaluationParameters, CriticalDamage);
	CriticalDamage = FMath::Max<float>(CriticalDamage, 0.0f);
	
    // 기본 데미지 가져오기 (SetByCaller로 전달된 값 : ProjectER::Data::Amount::Damage)
    // 값이 없을 시 0.0f를 기본값으로 사용
    float BaseDamage = 0.f;
    BaseDamage = Spec.GetSetByCallerMagnitude(ProjectER::Data::Amount::Damage, false, 0.0f);

	// 공격력 계수
	float AttackCoefficient = 1.0f;
	// float AttackCoefficient = Spec.GetSetByCallerMagnitude(ProjectER::Data::Amount::Coefficient, false, 1.0f);
	
    // 데미지 계산 공식 적용 
    // 데미지 = (기본데미지 + 공격력 * 계수) * (100 / (100 + 방어력))
    // *계수 처리는 여기서 하거나, BaseDamage에 이미 포함해서 넘길 수 있음.
	
	// 공식: (기본 데미지 + 공격력 * 계수)
    float TotalDamage = BaseDamage + (AttackPower * AttackCoefficient);
    
	const bool bIsCritical = FMath::FRandRange(0.0f, 100.0f) < CriticalChance;
	
	if (bIsCritical)
	{
		// 치명타 공식 적용
		// 예: 기본 1.5배(150%) + 추가 치명타 피해량(%)
		// 만약 CriticalDamage가 50이라면 -> 1.5 + 0.5 = 2.0배
		float CritMultiplier = 1.5f + (CriticalDamage / 100.0f);
        
		TotalDamage *= CritMultiplier;
        
		// [심화] UI에 "치명타!"를 띄우고 싶다면?
		// 방법 A: 로그 출력 (간단 확인용)
		// UE_LOG(LogTemp, Warning, TEXT("CRITICAL HIT! Damage: %f"), TotalDamage);
        
		// 방법 B: 커스텀 Context나 Tag 활용 (고급 구현 필요)
		// 여기서는 수치만 증폭시킵니다.
	}
	
    // 방어력 적용 (100 / (100 + 방어력)) -> 방어력이 높을수록 데미지 감소
    float Mitigation = 100.0f / (100.0f + Defense);
    float FinalDamage = TotalDamage * Mitigation;
	
	/*UE_LOG(LogTemp, Warning, TEXT("[DamageCalc] Attacker: %s / Target: %s"), 
	*SourceASC->GetAvatarActor()->GetName(), 
	*TargetASC->GetAvatarActor()->GetName());

	UE_LOG(LogTemp, Warning, TEXT("[DamageCalc] AttackPower: %f / BaseDamage: %f / DEFENSE: %f (Must be 28?)"), 
		AttackPower, BaseDamage, Defense);*/
	
    // 결과 출력 (IncomingDamage Attribute에 적용)
    if (FinalDamage > 0.f)
    {
        OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(UBaseAttributeSet::GetIncomingDamageAttribute(), EGameplayModOp::Additive, FinalDamage));
    }
}