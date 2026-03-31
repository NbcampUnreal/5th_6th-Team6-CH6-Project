#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LineOfSight/VisionData.h"
#include "VisionPlayerStateComp.generated.h"

TOPDOWNVISION_API DECLARE_LOG_CATEGORY_EXTERN(VisionPlayerStateComp, Log, All);

UCLASS(ClassGroup=(Vision), meta=(BlueprintSpawnableComponent))
class TOPDOWNVISION_API UVisionPlayerStateComp : public UActorComponent
{
    GENERATED_BODY()

public:
    UVisionPlayerStateComp();

protected:
    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
    UFUNCTION(BlueprintCallable, Category="Vision")
    void SetTeamChannel(EVisionChannel InTeam);

    UFUNCTION(BlueprintCallable, Category="Vision")
    EVisionChannel GetTeamChannel() const { return TeamChannel; }

    UFUNCTION(BlueprintCallable, Category="Vision")
    void SetAllReveal(bool bEnabled);

    UFUNCTION(BlueprintCallable, Category="Vision")
    bool IsAllReveal() const { return bAllReveal; }

    bool CanSeeTeam(EVisionChannel InTeam) const;

    void ReevaluateTargetVisibility(
        AActor* Target,
        EVisionChannel ExcludeObserverTeam = EVisionChannel::None);

    UFUNCTION(BlueprintCallable, Category="Vision")
    void RefreshVisibility();

private:
    UPROPERTY(ReplicatedUsing=OnRep_TeamChannel)
    EVisionChannel TeamChannel = EVisionChannel::None;

    UPROPERTY(ReplicatedUsing=OnRep_AllReveal)
    bool bAllReveal = false;

    UFUNCTION() void OnRep_TeamChannel();
    UFUNCTION() void OnRep_AllReveal();

    void InitializeSameTeamEvaluators();

    /** Pushes TeamChannel onto the owning pawn's Vision_VisualComp and
     *  re-registers it with the subsystem. Players have VisionChannel=None
     *  at Initialize() time because the team replicates later — this call
     *  fixes that as soon as the team is known. */
    void SyncPawnVisionChannel();
};