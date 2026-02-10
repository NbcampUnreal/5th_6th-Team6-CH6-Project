#include "ItemSystem/ItemTestPlayerController.h"
#include "ItemSystem/Interface/I_ItemInteractable.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"

AItemTestPlayerController::AItemTestPlayerController()
{
	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Hand;
}

void AItemTestPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	InputComponent->BindAction("RightClick", IE_Pressed, this, &AItemTestPlayerController::OnRightClick);
}

void AItemTestPlayerController::OnRightClick()
{
	FHitResult Hit;
	if (GetHitResultUnderCursor(ECC_Visibility, false, Hit))
	{
		AActor* HitActor = Hit.GetActor();

		if (HitActor && HitActor->Implements<UI_ItemInteractable>())
		{
			Target = HitActor;
		}
		else
		{
			Target = nullptr;
		}

		MoveTo(Hit.ImpactPoint);
	}
}

void AItemTestPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
	TickInteract();
}

void AItemTestPlayerController::MoveTo(const FVector& Dest)
{
	if (GetPawn())
	{
		UAIBlueprintHelperLibrary::SimpleMoveToLocation(this, Dest);
	}
}

void AItemTestPlayerController::TickInteract()
{
	if (Target && GetPawn())
	{
		float Dist = FVector::Dist(GetPawn()->GetActorLocation(), Target->GetActorLocation());

		if (Dist <= 150.f)
		{
			// 인터페이스 캐스팅 시에도 II_ItemInteractable (I+대문자I) 확인
			if (II_ItemInteractable* Interactable = Cast<II_ItemInteractable>(Target))
			{
				Interactable->PickupItem(GetPawn());
			}
			Target = nullptr;
		}
	}
}