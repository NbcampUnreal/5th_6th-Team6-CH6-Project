#include "ObstacleOcclusion/Helper/OcclusionMeshUtil.h"
#include "Components/MeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/SplineMeshComponent.h"
#include "Materials/MaterialInterface.h"

#include "ObstacleOcclusion/Manager/OcclusionBinderSubsystem.h"
#include "ObstacleOcclusion/MIDPool/OcclusionMIDSlot.h"
#include "EditorSetting/OcclusionMIDPoolSettings.h"

DEFINE_LOG_CATEGORY(OcclusionMeshHelper);

// ── Discover ──────────────────────────────────────────────────────────────────

FName UOcclusionMeshUtil::GetNormalMeshTag()
{
    const UOcclusionTagSettings* S = GetDefault<UOcclusionTagSettings>();
    return S ? S->NormalMeshTag : TEXT("OcclusionMesh");
}

FName UOcclusionMeshUtil::GetOccludedMeshTag()
{
    const UOcclusionTagSettings* S = GetDefault<UOcclusionTagSettings>();
    return S ? S->OccludedMeshTag : TEXT("OccludedVisual");
}

FName UOcclusionMeshUtil::GetRTMaterialTag()
{
    const UOcclusionTagSettings* S = GetDefault<UOcclusionTagSettings>();
    return S ? S->RTMaterialTag : TEXT("RTOcclusionMesh");
}

FName UOcclusionMeshUtil::GetNoShadowProxyTag()
{
    const UOcclusionTagSettings* S = GetDefault<UOcclusionTagSettings>();
    return S ? S->NoShadowProxyTag : NAME_None;
}

FName UOcclusionMeshUtil::GetAlphaParameterName()
{
    const UOcclusionTagSettings* S = GetDefault<UOcclusionTagSettings>();
    return S ? S->AlphaParameterName : TEXT("OcclusionAlpha");
}

FName UOcclusionMeshUtil::GetForceOccludeParameterName()
{
    const UOcclusionTagSettings* S = GetDefault<UOcclusionTagSettings>();
    return S ? S->ForceOccludeParameterName : TEXT("FullOcclusionAlpha");
}

FName UOcclusionMeshUtil::GetRTSwitchParameterName()
{
    const UOcclusionTagSettings* S = GetDefault<UOcclusionTagSettings>();
    return S ? S->OcclusionTypeSwitchTag : TEXT("IsRTorPhysical");
}

FName UOcclusionMeshUtil::GetOcclusionLockParameterName()
{
    const UOcclusionTagSettings* S = GetDefault<UOcclusionTagSettings>();
    return S ? S->OcclusionLockTag : TEXT("ShouldOcclude");
}

void UOcclusionMeshUtil::DiscoverChildMeshes(
    USceneComponent* Root,
    TArray<TSoftObjectPtr<UMeshComponent>>& OutNormalMeshes,
    TArray<TSoftObjectPtr<UMeshComponent>>& OutOccludedMeshes)
{
    OutNormalMeshes.Empty();
    OutOccludedMeshes.Empty();

    if (!Root)
    {
        UE_LOG(OcclusionMeshHelper, Warning,
            TEXT("UOcclusionMeshUtil::DiscoverChildMeshes>> Root is null"));
        return;
    }

    UE_LOG(OcclusionMeshHelper, Log,
        TEXT("UOcclusionMeshUtil::DiscoverChildMeshes>> Scanning children of %s"),
        *Root->GetOwner()->GetName());

    const FName NormalTag   = GetNormalMeshTag();
    const FName OccludedTag = GetOccludedMeshTag();

    TArray<USceneComponent*> Children;
    Root->GetChildrenComponents(true, Children);

    for (USceneComponent* Child : Children)
    {
        UMeshComponent* Mesh = Cast<UMeshComponent>(Child);
        if (!Mesh) continue;

        if (NormalTag != NAME_None && Mesh->ComponentHasTag(NormalTag))
        {
            OutNormalMeshes.Add(Mesh);
            UE_LOG(OcclusionMeshHelper, Log,
                TEXT("UOcclusionMeshUtil::DiscoverChildMeshes>> NormalMesh: %s (%s)"),
                *Mesh->GetName(), *Mesh->GetClass()->GetName());
        }
        else if (OccludedTag != NAME_None && Mesh->ComponentHasTag(OccludedTag))
        {
            OutOccludedMeshes.Add(Mesh);
            UE_LOG(OcclusionMeshHelper, Log,
                TEXT("UOcclusionMeshUtil::DiscoverChildMeshes>> OccludedMesh: %s (%s)"),
                *Mesh->GetName(), *Mesh->GetClass()->GetName());
        }
    }

    UE_LOG(OcclusionMeshHelper, Log,
        TEXT("UOcclusionMeshUtil::DiscoverChildMeshes>> Normal: %d | Occluded: %d"),
        OutNormalMeshes.Num(), OutOccludedMeshes.Num());

    if (OutNormalMeshes.Num() == 0 && OutOccludedMeshes.Num() == 0)
    {
        UE_LOG(OcclusionMeshHelper, Warning,
            TEXT("UOcclusionMeshUtil::DiscoverChildMeshes>> No tagged meshes found on %s"),
            *Root->GetOwner()->GetName());
    }
}

// ── MID Creation — internal helper ───────────────────────────────────────────

static void CreateMIDsForMeshType(
    const TArray<TSoftObjectPtr<UMeshComponent>>& Meshes,
    TArray<UMaterialInstanceDynamic*>& OutMIDs,
    bool bStaticOnly)
{
    OutMIDs.Empty();

    for (const TSoftObjectPtr<UMeshComponent>& MeshPtr : Meshes)
    {
        UMeshComponent* Mesh = MeshPtr.Get();
        if (!Mesh) continue;

        const bool bIsStatic   = Mesh->IsA<UStaticMeshComponent>();
        const bool bIsSkeletal = Mesh->IsA<USkeletalMeshComponent>();

        if (bStaticOnly  && !bIsStatic)  continue;
        if (!bStaticOnly && !bIsSkeletal) continue;

        const int32 MaterialCount = Mesh->GetNumMaterials();

        UE_LOG(OcclusionMeshHelper, Log,
            TEXT("UOcclusionMeshUtil::CreateMIDsForMeshType>> %s has %d material slots [%s]"),
            *Mesh->GetName(), MaterialCount, bStaticOnly ? TEXT("Static") : TEXT("Skeletal"));

        for (int32 i = 0; i < MaterialCount; i++)
        {
            UMaterialInterface* BaseMaterial = Mesh->GetMaterial(i);
            if (!BaseMaterial)
            {
                UE_LOG(OcclusionMeshHelper, Warning,
                    TEXT("UOcclusionMeshUtil::CreateMIDsForMeshType>> %s slot %d has no material — skipping"),
                    *Mesh->GetName(), i);
                continue;
            }

            UMaterialInstanceDynamic* MID = Mesh->CreateDynamicMaterialInstance(i, BaseMaterial);
            if (MID)
            {
                OutMIDs.Add(MID);
                UE_LOG(OcclusionMeshHelper, Log,
                    TEXT("UOcclusionMeshUtil::CreateMIDsForMeshType>> MID created — %s slot %d"),
                    *Mesh->GetName(), i);
            }
            else
            {
                UE_LOG(OcclusionMeshHelper, Warning,
                    TEXT("UOcclusionMeshUtil::CreateMIDsForMeshType>> Failed MID — %s slot %d"),
                    *Mesh->GetName(), i);
            }
        }
    }

    UE_LOG(OcclusionMeshHelper, Log,
        TEXT("UOcclusionMeshUtil::CreateMIDsForMeshType>> Total MIDs [%s]: %d"),
        bStaticOnly ? TEXT("Static") : TEXT("Skeletal"), OutMIDs.Num());
}

void UOcclusionMeshUtil::CreateDynamicMaterials_Static(
    const TArray<TSoftObjectPtr<UMeshComponent>>& Meshes,
    TArray<UMaterialInstanceDynamic*>& OutMIDs)
{
    CreateMIDsForMeshType(Meshes, OutMIDs, true);
}

void UOcclusionMeshUtil::CreateDynamicMaterials_Skeletal(
    const TArray<TSoftObjectPtr<UMeshComponent>>& Meshes,
    TArray<UMaterialInstanceDynamic*>& OutMIDs)
{
    CreateMIDsForMeshType(Meshes, OutMIDs, false);
}

void UOcclusionMeshUtil::CheckoutMaterials(const TArray<TSoftObjectPtr<UMeshComponent>>& Meshes,
    UOcclusionBinderSubsystem* Pool, TArray<FOcclusionMIDSlot>& OutSlots)
{
    OutSlots.Reset();
    if (!Pool) return;

    for (const TSoftObjectPtr<UMeshComponent>& MeshPtr : Meshes)
    {
        UMeshComponent* Mesh = MeshPtr.Get();
        if (!IsValid(Mesh)) continue;

        const int32 NumMats = Mesh->GetNumMaterials();
        for (int32 SlotIdx = 0; SlotIdx < NumMats; ++SlotIdx)
        {
            UMaterialInterface* Current = Mesh->GetMaterial(SlotIdx);
            if (!Current) continue;

            // Unwrap any already-applied MID to get the real asset for the pool key
            UMaterialInterface* ParentAsset = Current;
            if (const UMaterialInstanceDynamic* ExistingMID = Cast<UMaterialInstanceDynamic>(Current))
            {
                if (!ExistingMID->Parent) continue;
                ParentAsset = ExistingMID->Parent;
            }

            UMaterialInstanceDynamic* MID = Pool->CheckoutMID(ParentAsset, Mesh);
            if (!MID) continue;

            Mesh->SetMaterial(SlotIdx, MID);

            FOcclusionMIDSlot& Slot = OutSlots.AddDefaulted_GetRef();
            Slot.Mesh             = Mesh;
            Slot.SlotIndex        = SlotIdx;
            Slot.OriginalMaterial = ParentAsset;
            Slot.MID              = MID;

            UE_LOG(OcclusionMeshHelper, Verbose,
                TEXT("UOcclusionMeshUtil::CheckoutMaterials>> %s slot %d | Parent: %s"),
                *Mesh->GetName(), SlotIdx, *ParentAsset->GetName());
        }
    }

    UE_LOG(OcclusionMeshHelper, Verbose,
        TEXT("UOcclusionMeshUtil::CheckoutMaterials>> Checked out %d slots"),
        OutSlots.Num());
}

void UOcclusionMeshUtil::ReturnMaterials(TArray<FOcclusionMIDSlot>& Slots, UOcclusionBinderSubsystem* Pool)
{
    for (int32 i = Slots.Num() - 1; i >= 0; --i)
    {
        FOcclusionMIDSlot& Slot = Slots[i];
        if (!Slot.bPooled) continue;

        if (Pool && IsValid(Slot.MID))
            Pool->ReturnMID(Slot.MID);

        if (Slot.Mesh.IsValid())
            Slot.Mesh->SetMaterial(Slot.SlotIndex, Slot.OriginalMaterial);

        Slots.RemoveAtSwap(i, EAllowShrinking::No);
    }

    UE_LOG(OcclusionMeshHelper, Verbose,
        TEXT("UOcclusionMeshUtil::ReturnMaterials>> Pooled slots returned, non-pooled kept"));
}

void UOcclusionMeshUtil::AcquireMaterials(
    const TArray<TSoftObjectPtr<UMeshComponent>>& Meshes,
    const TArray<FName>& RequiredParameters,
    TArray<FOcclusionMIDSlot>& OutSlots,
    UOcclusionBinderSubsystem* Pool)
{
    OutSlots.Reset();

    for (const TSoftObjectPtr<UMeshComponent>& MeshPtr : Meshes)
    {
        UMeshComponent* Mesh = MeshPtr.Get();
        if (!IsValid(Mesh)) continue;

        const int32 NumMats = Mesh->GetNumMaterials();
        for (int32 SlotIdx = 0; SlotIdx < NumMats; ++SlotIdx)
        {
            UMaterialInterface* Current = Mesh->GetMaterial(SlotIdx);
            if (!Current) continue;

            // Unwrap existing MID to get real parent asset
            UMaterialInterface* ParentAsset = Current;
            if (const UMaterialInstanceDynamic* ExistingMID = Cast<UMaterialInstanceDynamic>(Current))
            {
                if (!IsValid(ExistingMID) || !IsValid(ExistingMID->Parent)) continue;
                ParentAsset = ExistingMID->Parent;
            }

            // Skip slots whose material doesn't have all required parameters
            if (!DoesMaterialHaveScalarParameters(ParentAsset, RequiredParameters))
            {
                UE_LOG(OcclusionMeshHelper, Verbose,
                    TEXT("UOcclusionMeshUtil::AcquireMaterials>> %s slot %d skipped — missing required params"),
                    *Mesh->GetName(), SlotIdx);
                continue;
            }

            const bool bUsePool = Pool != nullptr;

            // Non-pooled path — skip materials that belong to the pool
            // They will be handled by AcquireMIDs on occlusion enter
            if (!bUsePool)
            {
                if (UWorld* World = Mesh->GetWorld())
                {
                    if (UOcclusionBinderSubsystem* Sub = World->GetSubsystem<UOcclusionBinderSubsystem>())
                    {
                        if (Sub->IsMaterialPooled(ParentAsset))
                        {
                            UE_LOG(OcclusionMeshHelper, Verbose,
                                TEXT("UOcclusionMeshUtil::AcquireMaterials>> %s slot %d skipped — material is pooled"),
                                *Mesh->GetName(), SlotIdx);
                            continue;
                        }
                    }
                }
            }

            UMaterialInstanceDynamic* MID = bUsePool
                ? Pool->CheckoutMID(ParentAsset, Mesh)
                : Mesh->CreateDynamicMaterialInstance(SlotIdx, ParentAsset);

            if (!IsValid(MID))
            {
                if (bUsePool)
                {
                    // Pool full — mesh keeps its existing direct MID, skip pooled slot
                    UE_LOG(OcclusionMeshHelper, Verbose,
                        TEXT("UOcclusionMeshUtil::AcquireMaterials>> %s slot %d — pool full, mesh skipped"),
                        *Mesh->GetName(), SlotIdx);
                }
                else
                {
                    UE_LOG(OcclusionMeshHelper, Warning,
                        TEXT("UOcclusionMeshUtil::AcquireMaterials>> %s slot %d — MID acquisition failed"),
                        *Mesh->GetName(), SlotIdx);
                }
                continue;
            }

            if (bUsePool)
            {
                if (!IsValid(Mesh))
                {
                    Pool->ReturnMID(MID);
                    continue;
                }

                if (Mesh->IsA<USplineMeshComponent>())
                {
                    Pool->ReturnMID(MID);

                    UMaterialInstanceDynamic* DirectMID = Mesh->CreateDynamicMaterialInstance(SlotIdx, ParentAsset);
                    if (!IsValid(DirectMID)) continue;

                    FOcclusionMIDSlot& Slot = OutSlots.AddDefaulted_GetRef();
                    Slot.Mesh             = Mesh;
                    Slot.SlotIndex        = SlotIdx;
                    Slot.OriginalMaterial = ParentAsset;
                    Slot.MID              = DirectMID;
                    Slot.bPooled          = false;

                    UE_LOG(OcclusionMeshHelper, Verbose,
                        TEXT("UOcclusionMeshUtil::AcquireMaterials>> %s slot %d | spline — direct create"),
                        *Mesh->GetName(), SlotIdx);
                    continue;
                }

                Mesh->SetMaterial(SlotIdx, MID);
            }

            

            FOcclusionMIDSlot& Slot = OutSlots.AddDefaulted_GetRef();
            Slot.Mesh             = Mesh;
            Slot.SlotIndex        = SlotIdx;
            Slot.OriginalMaterial = ParentAsset;
            Slot.MID              = MID;
            Slot.bPooled          = bUsePool;

            UE_LOG(OcclusionMeshHelper, Verbose,
                TEXT("UOcclusionMeshUtil::AcquireMaterials>> %s slot %d | %s | Parent: %s"),
                *Mesh->GetName(), SlotIdx,
                bUsePool ? TEXT("pooled") : TEXT("created"),
                *ParentAsset->GetName());
        }
    }

    UE_LOG(OcclusionMeshHelper, Log,
        TEXT("UOcclusionMeshUtil::AcquireMaterials>> %d slots acquired (%s)"),
        OutSlots.Num(), Pool ? TEXT("pooled") : TEXT("created"));
}

// ── Shadow Proxy Generation ───────────────────────────────────────────────────

void UOcclusionMeshUtil::GenerateShadowProxyMeshes(
    AActor* Owner,
    const TArray<TSoftObjectPtr<UMeshComponent>>& SourceMeshes,
    UMaterialInterface* ShadowProxyMaterial,
    TArray<TObjectPtr<UStaticMeshComponent>>& OutStaticProxies,
    TArray<TObjectPtr<USkeletalMeshComponent>>& OutSkeletalProxies)
{
    if (!Owner)
    {
        UE_LOG(OcclusionMeshHelper, Warning,
            TEXT("UOcclusionMeshUtil::GenerateShadowProxyMeshes>> Owner is null"));
        return;
    }

    for (TObjectPtr<UStaticMeshComponent> Proxy : OutStaticProxies)
        if (Proxy) Proxy->DestroyComponent();
    OutStaticProxies.Empty();

    for (TObjectPtr<USkeletalMeshComponent> Proxy : OutSkeletalProxies)
        if (Proxy) Proxy->DestroyComponent();
    OutSkeletalProxies.Empty();

    if (!IsValid(ShadowProxyMaterial))
    {
        UE_LOG(OcclusionMeshHelper, Warning,
            TEXT("UOcclusionMeshUtil::GenerateShadowProxyMeshes>> No ShadowProxyMaterial set on %s — proxies will cast no shadow"),
            *Owner->GetName());
    }

    const FName NoShadowTag = GetNoShadowProxyTag();  // loaded from UOcclusionTagSettings

    for (const TSoftObjectPtr<UMeshComponent>& MeshPtr : SourceMeshes)
    {
        UMeshComponent* SourceMesh = MeshPtr.Get();
        if (!SourceMesh) continue;

        if (NoShadowTag != NAME_None && SourceMesh->ComponentHasTag(NoShadowTag))
        {
            UE_LOG(OcclusionMeshHelper, Log,
                TEXT("UOcclusionMeshUtil::GenerateShadowProxyMeshes>> %s skipped — has NoShadowTag '%s'"),
                *SourceMesh->GetName(), *NoShadowTag.ToString());
            continue;
        }

        // ── Static proxy ──────────────────────────────────────────────────
        if (UStaticMeshComponent* SourceStatic = Cast<UStaticMeshComponent>(SourceMesh))
        {
            UStaticMeshComponent* Proxy = NewObject<UStaticMeshComponent>(
                Owner, UStaticMeshComponent::StaticClass(), NAME_None, RF_NoFlags);
            if (!Proxy) continue;

            Proxy->SetStaticMesh(SourceStatic->GetStaticMesh());
            Proxy->AttachToComponent(
                SourceStatic,
                FAttachmentTransformRules::SnapToTargetIncludingScale);

            Proxy->SetVisibility(false);
            Proxy->SetHiddenInGame(true);
            Proxy->SetCastShadow(true);
            Proxy->bCastHiddenShadow  = true;
            Proxy->bCastDynamicShadow = true;
            Proxy->SetCollisionEnabled(ECollisionEnabled::NoCollision);

            if (IsValid(ShadowProxyMaterial))
                for (int32 i = 0; i < Proxy->GetNumMaterials(); i++)
                    Proxy->SetMaterial(i, ShadowProxyMaterial);

            Proxy->RegisterComponent();
            Owner->AddInstanceComponent(Proxy);
            OutStaticProxies.Add(Proxy);

            UE_LOG(OcclusionMeshHelper, Log,
                TEXT("UOcclusionMeshUtil::GenerateShadowProxyMeshes>> Static proxy created for %s"),
                *SourceStatic->GetName());
        }
        // ── Skeletal proxy ────────────────────────────────────────────────
        else if (USkeletalMeshComponent* SourceSkeletal = Cast<USkeletalMeshComponent>(SourceMesh))
        {
            USkeletalMeshComponent* Proxy = NewObject<USkeletalMeshComponent>(
                Owner, USkeletalMeshComponent::StaticClass(), NAME_None, RF_NoFlags);
            if (!Proxy) continue;

            Proxy->SetSkeletalMeshAsset(SourceSkeletal->GetSkeletalMeshAsset());
            Proxy->SetLeaderPoseComponent(SourceSkeletal);
            Proxy->AttachToComponent(
                SourceSkeletal,
                FAttachmentTransformRules::SnapToTargetIncludingScale);

            Proxy->SetVisibility(false);
            Proxy->SetHiddenInGame(true);
            Proxy->SetCastShadow(true);
            Proxy->bCastHiddenShadow  = true;
            Proxy->bCastDynamicShadow = true;
            Proxy->SetCollisionEnabled(ECollisionEnabled::NoCollision);

            if (IsValid(ShadowProxyMaterial))
                for (int32 i = 0; i < Proxy->GetNumMaterials(); i++)
                    Proxy->SetMaterial(i, ShadowProxyMaterial);

            Proxy->RegisterComponent();
            Owner->AddInstanceComponent(Proxy);
            OutSkeletalProxies.Add(Proxy);

            UE_LOG(OcclusionMeshHelper, Log,
                TEXT("UOcclusionMeshUtil::GenerateShadowProxyMeshes>> Skeletal proxy created for %s"),
                *SourceSkeletal->GetName());
        }
    }

    UE_LOG(OcclusionMeshHelper, Log,
        TEXT("UOcclusionMeshUtil::GenerateShadowProxyMeshes>> Done for %s — Static: %d | Skeletal: %d"),
        *Owner->GetName(), OutStaticProxies.Num(), OutSkeletalProxies.Num());
}

TEnumAsByte<ECollisionChannel> UOcclusionMeshUtil::GetOcclusionTraceChannel()
{
    const UOcclusionTagSettings* S = GetDefault<UOcclusionTagSettings>();
    return S ? S->OcclusionTraceChannel : TEnumAsByte<ECollisionChannel>(ECC_GameTraceChannel1);
}

TEnumAsByte<ECollisionChannel> UOcclusionMeshUtil::GetMouseTraceChannel()
{
    const UOcclusionTagSettings* S = GetDefault<UOcclusionTagSettings>();
    return S ? S->MouseTraceChannel : TEnumAsByte<ECollisionChannel>(ECC_Visibility);
}

TEnumAsByte<ECollisionChannel> UOcclusionMeshUtil::GetInteriorTraceChannel()
{
    const UOcclusionTagSettings* S = GetDefault<UOcclusionTagSettings>();
    return S ? S->InteriorTraceChannel : TEnumAsByte<ECollisionChannel>(ECC_GameTraceChannel2);
}

bool UOcclusionMeshUtil::DoesMaterialHaveScalarParameters(const UMaterialInterface* Material,
                                                          const TArray<FName>& ParameterNames)
{
    if (!Material || ParameterNames.IsEmpty()) return false;

    float Dummy = 0.f;
    for (const FName& Param : ParameterNames)
    {
        if (Param == NAME_None) continue;  // skip invalid

        if (!Material->GetScalarParameterValue(Param, Dummy))
            return false;  // this param doesn't exist — fail immediately
    }

    return true;
}
