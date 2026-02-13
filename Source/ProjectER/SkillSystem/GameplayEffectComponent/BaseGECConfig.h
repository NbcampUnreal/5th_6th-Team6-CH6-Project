// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "BaseGECConfig.generated.h"

/**
 * 
 */

UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced)
class PROJECTER_API UBaseGECConfig : public UObject
{
	GENERATED_BODY()

public:

protected:
	
private:

public:

protected:

private:
	
};

UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced)
class PROJECTER_API UTESTConfig : public UBaseGECConfig
{
	GENERATED_BODY()

public:

protected:
	UPROPERTY(EditDefaultsOnly, meta = (AllowPrivateAccess = "true"))
	float Range;
private:

public:

protected:

private:

};
