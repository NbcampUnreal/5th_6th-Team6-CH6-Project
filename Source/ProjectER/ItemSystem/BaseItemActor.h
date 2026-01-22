#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BaseItemActor.generated.h"

class UStaticMeshComponent;
class UBaseItemData;

UCLASS()
class PROJECTER_API ABaseItemActor : public AActor
{
	GENERATED_BODY()

public:
	ABaseItemActor();

protected:
	virtual void BeginPlay() override;

public:
	// 외형을 담당하는 스태틱 메시 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item|Component")
	UStaticMeshComponent* ItemMesh;

	// 이 액터가 어떤 아이템인지 정의하는 데이터 (에디터에서 할당)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Data")
	UBaseItemData* ItemData;
};