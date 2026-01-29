// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "TargetableInterface.generated.h"

// 팀 구분을 위한 열거형 (Enum) 정의
UENUM(BlueprintType)
enum class ETeamType : uint8
{
	None        UMETA(DisplayName = "None"),
	Team_A      UMETA(DisplayName = "Team_A"),
	Team_B      UMETA(DisplayName = "Team_B"),
	Team_C      UMETA(DisplayName = "Team_C"),
	Neutral     UMETA(DisplayName = "Neutral")
};

UINTERFACE(MinimalAPI)
class UTargetableInterface : public UInterface
{
	GENERATED_BODY()
};

class PROJECTER_API ITargetableInterface
{
	GENERATED_BODY()

public:
	// 해당 액터의 팀 정보를 반환
	virtual ETeamType GetTeamType() const = 0;

	// 현재 타겟팅이 가능한 상태인지 확인 (죽었거나, 은신이거나, 무적 등)
	virtual bool IsTargetable() const = 0;

	// (추가 예정) 타겟팅 되었을 때 시각적 효과(아웃라인)를 켜고 끄기 위한 함수
	// virtual void HighlightActor(bool bIsHighlight) {}
};
