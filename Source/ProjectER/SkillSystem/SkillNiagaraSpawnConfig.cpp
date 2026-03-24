// Fill out your copyright notice in the Description page of Project Settings.


#include "SkillSystem/SkillNiagaraSpawnConfig.h"
#include "NiagaraSystem.h"
#include "NiagaraTypes.h"

#if WITH_EDITOR
void USkillNiagaraSpawnConfig::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName PropertyName = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;

	if (PropertyName == GET_MEMBER_NAME_CHECKED(USkillNiagaraSpawnConfig, NiagaraSystem))
	{
		RefreshParameters();
	}
}

void USkillNiagaraSpawnConfig::RefreshParameters()
{
	UNiagaraSystem* NS = NiagaraSystem.LoadSynchronous();
	if (!NS)
	{
		FloatParameters.Empty();
		VectorParameters.Empty();
		ColorParameters.Empty();
		return;
	}

	TArray<FNiagaraVariable> Variables;
	NS->GetExposedParameters().GetUserParameters(Variables);

	TSet<FName> NewFloatNames;
	TSet<FName> NewVectorNames;
	TSet<FName> NewColorNames;

	for (const FNiagaraVariable& Var : Variables)
	{
		FName VarName = Var.GetName();
		const FNiagaraTypeDefinition& Type = Var.GetType();

		if (Type == FNiagaraTypeDefinition::GetFloatDef())
		{
			NewFloatNames.Add(VarName);
			if (!FloatParameters.Contains(VarName))
			{
				FloatParameters.Add(VarName, 0.0f);
			}
		}
		else if (Type == FNiagaraTypeDefinition::GetVec3Def() || Type == FNiagaraTypeDefinition::GetPositionDef())
		{
			NewVectorNames.Add(VarName);
			if (!VectorParameters.Contains(VarName))
			{
				VectorParameters.Add(VarName, FVector::ZeroVector);
			}
		}
		else if (Type == FNiagaraTypeDefinition::GetColorDef())
		{
			NewColorNames.Add(VarName);
			if (!ColorParameters.Contains(VarName))
			{
				ColorParameters.Add(VarName, FLinearColor::White);
			}
		}
	}

	// Remove parameters that no longer exist in the system
	auto CleanMap = [](TMap<FName, float>& Map, const TSet<FName>& NewNames)
	{
		TArray<FName> KeysToRemove;
		for (const auto& Pair : Map)
		{
			if (!NewNames.Contains(Pair.Key)) { KeysToRemove.Add(Pair.Key); }
		}
		for (FName Key : KeysToRemove) { Map.Remove(Key); }
	};

	auto CleanVectorMap = [](TMap<FName, FVector>& Map, const TSet<FName>& NewNames)
	{
		TArray<FName> KeysToRemove;
		for (const auto& Pair : Map)
		{
			if (!NewNames.Contains(Pair.Key)) { KeysToRemove.Add(Pair.Key); }
		}
		for (FName Key : KeysToRemove) { Map.Remove(Key); }
	};

	auto CleanColorMap = [](TMap<FName, FLinearColor>& Map, const TSet<FName>& NewNames)
	{
		TArray<FName> KeysToRemove;
		for (const auto& Pair : Map)
		{
			if (!NewNames.Contains(Pair.Key)) { KeysToRemove.Add(Pair.Key); }
		}
		for (FName Key : KeysToRemove) { Map.Remove(Key); }
	};

	CleanMap(FloatParameters, NewFloatNames);
	CleanVectorMap(VectorParameters, NewVectorNames);
	CleanColorMap(ColorParameters, NewColorNames);
}
#endif
