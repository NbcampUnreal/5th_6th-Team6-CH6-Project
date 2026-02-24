// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillSystem/GameplyeEffect/SkillEffectDataAsset.h"
#include "AbilitySystemComponent.h"
#include "SkillSystem/GameAbility/SkillBase.h"
#include "GameplayEffect.h"
#include "GameplayEffectTypes.h"
#include "GameplayEffectComponent.h"
#include "SkillSystem/SkillConfig/BaseSkillConfig.h"
#include "SkillSystem/GameplayEffectComponent/BaseGEC.h"
#include "SkillSystem/GameplayEffectComponent/BaseGECConfig.h"


USkillEffectDataAsset::USkillEffectDataAsset()
{
	IndexTag = FGameplayTag::RequestGameplayTag(FName("Skill.Data.EffectIndex"));
}

TArray<FGameplayEffectSpecHandle> USkillEffectDataAsset::MakeSpecs(UAbilitySystemComponent* InstigatorASC, USkillBase* InstigatorSkill, AActor* InEffectCauser, const FGameplayEffectContextHandle InEffectContextHandle) const
{
    TArray<FGameplayEffectSpecHandle> OutSpecs;

    if (!IsValid(InstigatorASC) || !IsValid(InstigatorSkill)) {
        return OutSpecs;
    }

    // 2. 새로운 컨텍스트 핸들 준비 (전달받은 게 유효하면 복사본 생성, 없으면 신규 생성)
    FGameplayEffectContextHandle SharedContextHandle;

    if (InEffectContextHandle.IsValid())
    {
        // Duplicate()를 직접 호출하지 마세요. 핸들을 대입하면 엔진이 안전하게 복제합니다.
        SharedContextHandle = InEffectContextHandle;
    }
    else
    {
        SharedContextHandle = InstigatorASC->MakeEffectContext();
    }

    // 3. 공통 정보 설정
    AActor* AvatarActor = InstigatorASC->GetAvatarActor();
    AActor* FinalCauser = (InEffectCauser != nullptr) ? InEffectCauser : AvatarActor;

    SharedContextHandle.AddSourceObject(this);
    SharedContextHandle.AddInstigator(AvatarActor, FinalCauser);

    int32 Level = InstigatorSkill->GetAbilityLevel();

    for (int32 i = 0; i < Data.SkillEffectDefinition.Num(); ++i)
    {
        const FSkillEffectDefinition& Def = Data.SkillEffectDefinition[i];
        if (Def.SkillEffectClass == nullptr) continue;

        // 4. 준비된 핸들로 Spec 생성
        FGameplayEffectSpecHandle SpecHandle = InstigatorASC->MakeOutgoingSpec(Def.SkillEffectClass, Level, SharedContextHandle);

        if (SpecHandle.IsValid())
        {
            const FGameplayEffectContextHandle& SpecContextHandle = SpecHandle.Data->GetContext();
            SpecHandle.Data->SetSetByCallerMagnitude(IndexTag, static_cast<float>(i));
            OutSpecs.Add(SpecHandle);
        }
    }

    return OutSpecs;
}

#if WITH_EDITOR
void USkillEffectDataAsset::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    // 1. 변경된 프로퍼티의 이름을 가져옵니다.
    const FName PropertyName = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;
    const FName MemberPropertyName = (PropertyChangedEvent.MemberProperty != nullptr) ? PropertyChangedEvent.MemberProperty->GetFName() : NAME_None;

    // 2. SkillEffectDefinition 구조체 내부의 SkillEffectClass가 변경되었는지 확인합니다.
    // MemberPropertyName은 구조체 배열 자체(Data)를, PropertyName은 그 안의 변수명을 체크합니다.
    if (PropertyName == GET_MEMBER_NAME_CHECKED(FSkillEffectDefinition, SkillEffectClass))
    {
        UE_LOG(LogTemp, Log, TEXT("GE Class가 변경되어 Config를 자동으로 갱신합니다."));

        // 우리가 만든 스캔 함수 호출
        RefreshConfigsFromGE();
    }
}
#endif

void USkillEffectDataAsset::RefreshConfigsFromGE()
{
#if WITH_EDITOR
    for (auto& Def : Data.SkillEffectDefinition)
    {
        if (!Def.SkillEffectClass) continue;

        // 2. 기존 Config 보관 (타입이 같으면 재사용하기 위함)
        TObjectPtr<UBaseGECConfig> OldConfig = Def.Config;
        Def.Config = nullptr;

        // 3. 리플렉션으로 GEComponents 프로퍼티 접근
        FArrayProperty* ArrayProp = CastField<FArrayProperty>(UGameplayEffect::StaticClass()->FindPropertyByName(TEXT("GEComponents")));
        // 1. GE 에셋의 CDO 가져오기
        const UGameplayEffect* GECDO = Def.SkillEffectClass->GetDefaultObject<UGameplayEffect>();
        if (ArrayProp)
        {
            const void* ArrayPtr = ArrayProp->ContainerPtrToValuePtr<void>(GECDO);
            FScriptArrayHelper ArrayHelper(ArrayProp, ArrayPtr);

            // 4. GE에 박혀있는 컴포넌트들을 순회
            for (int32 i = 0; i < ArrayHelper.Num(); ++i)
            {
                // 배열 요소(TObjectPtr)를 실제 포인터로 변환
                UGameplayEffectComponent* GEC = *reinterpret_cast<UGameplayEffectComponent**>(ArrayHelper.GetRawPtr(i));

                // 우리가 만든 UBaseGEC 계열인지 확인
                UBaseGEC* BaseGEC = Cast<UBaseGEC>(GEC);
                if (!BaseGEC) continue;

                // 5. 해당 GEC가 요구하는 Config 클래스 확인
                TSubclassOf<UBaseGECConfig> RequiredClass = BaseGEC->GetRequiredConfigClass();
                if (!RequiredClass) continue;

                // 6. 기존 데이터 복구 또는 신규 생성
                // 기존에 쓰던 Config가 있고, 클래스 타입이 일치한다면 그대로 사용 (수치 유지)
                if (OldConfig && OldConfig->GetClass() == RequiredClass)
                {
                    Def.Config = OldConfig;
                }
                else
                {
                    // 클래스가 바뀌었거나 없으면 새로 생성
                    Def.Config = NewObject<UBaseGECConfig>(this, RequiredClass, NAME_None, RF_Transactional | RF_Public);
                }

                // 현재 구조가 Def 하나당 Config 하나라면 첫 번째 유효한 GEC에서 중단
                if (Def.Config) break;
            }
        }
    }

    MarkPackageDirty();
    UE_LOG(LogTemp, Log, TEXT("SkillEffectDataAsset: Configs Refreshed from GE Components."));
#endif
}