// Fill out your copyright notice in the Description page of Project Settings.


#include "ObstacleOcclusion/Manager/OcclusionBinderSubsystem.h"
#include "ObstacleOcclusion/Binder/OcclusionBinder.h"
#include "EditorSetting/OcclusionMIDSettings.h"

DEFINE_LOG_CATEGORY(OcclusionBinderSubsystem);

void UOcclusionBinderSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	//populate the pool
	PrePopulatePool();
}

void UOcclusionBinderSubsystem::Deinitialize()
{
	CommitGenocide();
	
	PrimitiveToBinderMap.Empty();
	Super::Deinitialize();
}

void UOcclusionBinderSubsystem::RegisterBinderPrimitive(UPrimitiveComponent* Prim, AOcclusionBinder* Binder)
{
	if (!IsValid(Prim) || !IsValid(Binder)) return;

	PrimitiveToBinderMap.Add(Prim, Binder);

	UE_LOG(OcclusionBinderSubsystem, Log,
		TEXT("UOcclusionBinderSubsystem::RegisterBinderPrimitive>> %s → %s"),
		*Prim->GetName(),
		*Binder->GetName());
}

void UOcclusionBinderSubsystem::UnregisterBinder(AOcclusionBinder* Binder)
{
	if (!IsValid(Binder)) return;

	// Remove all primitives that point to this binder
	for (auto It = PrimitiveToBinderMap.CreateIterator(); It; ++It)
	{
		if (It.Value() == Binder)
			It.RemoveCurrent();
	}

	UE_LOG(OcclusionBinderSubsystem, Log,
		TEXT("UOcclusionBinderSubsystem::UnregisterBinder>> %s unregistered"),
		*Binder->GetName());
}

AOcclusionBinder* UOcclusionBinderSubsystem::FindBinder(UPrimitiveComponent* Prim) const
{
	if (!IsValid(Prim)) return nullptr;

	const TWeakObjectPtr<AOcclusionBinder>* Found = PrimitiveToBinderMap.Find(Prim);
	if (!Found || !Found->IsValid()) return nullptr;

	return Found->Get();
}

UMaterialInstanceDynamic* UOcclusionBinderSubsystem::CheckoutMID(UMaterialInterface* Parent, UObject* Outer)
{
	if (!Parent) return nullptr;

	const UOcclusionMIDSettings* Settings = GetDefault<UOcclusionMIDSettings>();
	const int32 MaxPerMaterial = Settings ? Settings->MaxPooledMIDsPerMaterial : 50;

	// Recycle from free list first
	TArray<TObjectPtr<UMaterialInstanceDynamic>>* FreeList = MIDPool.Find(Parent);
	if (FreeList && FreeList->Num() > 0)
	{
		UMaterialInstanceDynamic* Recycled = FreeList->Pop(EAllowShrinking::No).Get();

		UE_LOG(OcclusionBinderSubsystem, Verbose,
			TEXT("CheckoutMID>> Recycled for %s | Pool remaining: %d"),
			*Parent->GetName(), FreeList->Num());

		return Recycled;
	}

	// Count total anchored for this parent
	const int32 TotalAnchored = AllPooledMIDs.FilterByPredicate(
		[&](const TObjectPtr<UMaterialInstanceDynamic>& M)
		{
			return IsValid(M) && M->Parent == Parent;
		}).Num();

	// Hard cap — return nullptr, caller skips this mesh
	if (TotalAnchored >= MaxPerMaterial)
	{
		UE_LOG(OcclusionBinderSubsystem, Verbose,
			TEXT("CheckoutMID>> Pool cap reached for %s | Cap: %d — mesh skipped"),
			*Parent->GetName(), MaxPerMaterial);
		return nullptr;
	}

	// Under cap — create and anchor
	UMaterialInstanceDynamic* Fresh = UMaterialInstanceDynamic::Create(Parent, Outer);
	AllPooledMIDs.Add(Fresh);

	UE_LOG(OcclusionBinderSubsystem, Verbose,
		TEXT("CheckoutMID>> Created+anchored for %s | Total: %d / %d"),
		*Parent->GetName(), TotalAnchored + 1, MaxPerMaterial);

	return Fresh;
}

void UOcclusionBinderSubsystem::PrePopulatePool()
{
	const UOcclusionMIDSettings* Settings = GetDefault<UOcclusionMIDSettings>();
	if (!Settings || Settings->PreWarmCountPerMaterial <= 0) return;

	for (const TSoftObjectPtr<UMaterialInterface>& SoftMat : Settings->PreWarmMaterials)
	{
		UMaterialInterface* Mat = SoftMat.LoadSynchronous();
		if (!Mat) continue;

		for (int32 i = 0; i < Settings->PreWarmCountPerMaterial; ++i)
		{
			UMaterialInstanceDynamic* MID = UMaterialInstanceDynamic::Create(Mat, this);
			if (!MID) continue;

			AllPooledMIDs.Add(MID);
			MIDPool.FindOrAdd(Mat).Add(TObjectPtr<UMaterialInstanceDynamic>(MID));
		}

		UE_LOG(OcclusionBinderSubsystem, Log,
			TEXT("UOcclusionBinderSubsystem::PrePopulatePool>> %s | Pre-warmed: %d"),
			*Mat->GetName(), Settings->PreWarmCountPerMaterial);
	}
}

void UOcclusionBinderSubsystem::CommitGenocide()
{
	// Clear pool map — MIDs still anchored in AllPooledMIDs until GC
	MIDPool.Empty();

	// Release GC anchor — engine can now collect all idle MIDs
	AllPooledMIDs.Empty();

	UE_LOG(OcclusionBinderSubsystem, Log,
		TEXT("UOcclusionBinderSubsystem::TeardownPool>> Pool cleared"));
}

void UOcclusionBinderSubsystem::ReturnMID(UMaterialInstanceDynamic* MID)
{
	if (!IsValid(MID)) return;

	UMaterialInterface* Parent = MID->Parent;
	if (!Parent) return;

	MID->ClearParameterValues();

	MIDPool.FindOrAdd(Parent).Add(TObjectPtr<UMaterialInstanceDynamic>(MID));

	UE_LOG(OcclusionBinderSubsystem, Verbose,
		TEXT("ReturnMID>> Returned for %s | Free: %d"),
		*Parent->GetName(), MIDPool.Find(Parent)->Num());
}
