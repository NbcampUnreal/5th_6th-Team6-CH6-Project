#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LineOfSight/VisionData.h"
#include "VisionPlayerStateComp.generated.h"

TOPDOWNVISION_API DECLARE_LOG_CATEGORY_EXTERN(VisionPlayerStateComp, Log, All);

TOPDOWNVISION_API DECLARE_MULTICAST_DELEGATE(FOnVisionReady);


UCLASS(ClassGroup=(Vision), meta=(BlueprintSpawnableComponent))
class TOPDOWNVISION_API UVisionPlayerStateComp : public UActorComponent
{
    GENERATED_BODY()

public:
    UVisionPlayerStateComp();

protected:
    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
    // --- Team --- //
    UFUNCTION(BlueprintCallable, Category="Vision")
    void SetTeamChannel(EVisionChannel InTeam);

    UFUNCTION(BlueprintCallable, Category="Vision")
    EVisionChannel GetTeamChannel() const { return TeamChannel; }

    // --- All Reveal --- //
    UFUNCTION(BlueprintCallable, Category="Vision")
    void SetAllReveal(bool bEnabled);

    UFUNCTION(BlueprintCallable, Category="Vision")
    bool IsAllReveal() const { return bAllReveal; }

    // --- Visibility logic (single location for filter + apply) --- //

    /** Returns true if this player can see actors belonging to the given team. */
    bool CanSeeTeam(EVisionChannel InTeam) const;

    /** Applies team filter then calls VisualComp->SetVisible.
     *  Called by GameStateComp push callbacks and RefreshVisibility. */
    void ApplyActorVisibility(AActor* Target, EVisionChannel Team, bool bVisible);

    /** Full re-evaluation against all currently tracked actors.
     *  Also drains any pending queue in GameStateComp.
     *  Called on rep, after team assignment, and on BeginPlay next tick. */
    UFUNCTION(BlueprintCallable, Category="Vision")
    void RefreshVisibility();

    bool IsVisionReady() const { return bVisionReady; }

    //Signal for reporting 
    FOnVisionReady OnVisionReady;
    
private:
    UPROPERTY(ReplicatedUsing=OnRep_TeamChannel)
    EVisionChannel TeamChannel = EVisionChannel::None;

    UPROPERTY(ReplicatedUsing=OnRep_AllReveal)
    bool bAllReveal = false;

    UFUNCTION() void OnRep_TeamChannel();
    UFUNCTION() void OnRep_AllReveal();



private:
    bool bVisionReady = false;
};