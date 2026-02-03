// Fill out your copyright notice in the Description page of Project Settings.


#include "LineOfSight/ObstacleMask/ObstacleMaskBaker.h"

#include "EngineUtils.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Components/BoxComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "TopDownVisionLogCategories.h"//log
#include "LineOfSight/VisionSubsystem.h"

#if WITH_EDITOR
#include "EditorAssetLibrary.h"// this handles the asset management during the editor time
#endif

// Sets default values
AObstacleMaskBaker::AObstacleMaskBaker()
{
	PrimaryActorTick.bCanEverTick = false;

	// root
	USceneComponent* RootComp = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = RootComp;
	
	// Create BoxVolume
	BoxVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxVolume"));
	BoxVolume->SetBoxExtent(FVector(500.f, 500.f, 100.f)); // default size
	BoxVolume->SetupAttachment(RootComponent);

	// Create SceneCaptureComponent2D
	SceneCaptureComp = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("SceneCaptureComp"));
	SceneCaptureComp->SetupAttachment(RootComponent);

	// SceneCapture settings
	SceneCaptureComp->ProjectionType = ECameraProjectionMode::Orthographic;
	SceneCaptureComp->CaptureSource = ESceneCaptureSource::SCS_SceneDepth;// use scene depth for maks texture
	SceneCaptureComp->bCaptureEveryFrame = false;
	SceneCaptureComp->bCaptureOnMovement = false;
	SceneCaptureComp->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;

	// make it look down
	SceneCaptureComp->SetWorldRotation(SceneCaptureWorldRotation);// add yaw rotation to match forward direction

	//default ortho width based on box size
	SceneCaptureComp->OrthoWidth = BoxVolume->GetScaledBoxExtent().X * 2.f;
}

// Called when the game starts or when spawned
void AObstacleMaskBaker::BeginPlay()
{
	Super::BeginPlay();
	
}


//========= Editor functions here ==========//
#if WITH_EDITOR

void AObstacleMaskBaker::CommandToAllBakers(EObstacleBakeRequest Request)
{
	if (!GEditor) return;

	UWorld* EditorWorld = GEditor->GetEditorWorldContext().World();
	if (!EditorWorld) return;

	for (TActorIterator<AObstacleMaskBaker> It(EditorWorld); It; ++It)
	{
		AObstacleMaskBaker* Baker = *It;
		if (Baker)
		{
			Baker->OnBakeRequested(Request);
		}
	}
}

// cpp internal helper for merging texture
static void MergeRTs_To_RG(
	UTextureRenderTarget2D* RT_R,
	UTextureRenderTarget2D* RT_G,
	UTextureRenderTarget2D* OutRT)
{
	if (!RT_R || !RT_G || !OutRT)
	{
		UE_LOG(LOSWorldBaker, Error,
			TEXT("MergeRTs_To_RG >> Invalid render target input"));
		return;
	}

	FTextureRenderTargetResource* ResR = RT_R->GameThread_GetRenderTargetResource();
	FTextureRenderTargetResource* ResG = RT_G->GameThread_GetRenderTargetResource();
	FTextureRenderTargetResource* ResOut = OutRT->GameThread_GetRenderTargetResource();

	TArray<FColor> PixelsR;
	TArray<FColor> PixelsG;

	ResR->ReadPixels(PixelsR);
	ResG->ReadPixels(PixelsG);

	if (PixelsR.Num() != PixelsG.Num())
	{
		UE_LOG(LOSWorldBaker, Error,
			TEXT("MergeRTs_To_RG >> Pixel count mismatch"));
		return;
	}

	TArray<FColor> OutPixels;
	OutPixels.Init(FColor::Black, PixelsR.Num());

	for (int32 i = 0; i < PixelsR.Num(); i++)
	{
		OutPixels[i].R = PixelsR[i].R; // Shadow
		OutPixels[i].G = PixelsG[i].R; // Low obstacle
		OutPixels[i].A = 255;
	}

	ENQUEUE_RENDER_COMMAND(WriteMergedRT)(
		[ResOut, OutPixels](FRHICommandListImmediate& RHICmdList)
		{
			FUpdateTextureRegion2D Region(
				0, 0, 0, 0,
				ResOut->GetSizeX(),
				ResOut->GetSizeY()
			);

			RHIUpdateTexture2D(
				ResOut->GetRenderTargetTexture(),
				0,
				Region,
				ResOut->GetSizeX() * 4,
				reinterpret_cast<const uint8*>(OutPixels.GetData())
			);
		}
	);

	FlushRenderingCommands();

	UE_LOG(LOSWorldBaker, Log,
		TEXT("MergeRTs_To_RG >> Merge complete"));
}



void AObstacleMaskBaker::BakeObstacleMask()
{
	if (!BoxVolume || !SceneCaptureComp)
	{
		UE_LOG(LOSWorldBaker, Error,
			TEXT("AObstacleMaskBaker::BakeObstacleMask >> Missing components"));
		return;
	}
	
	//Locks the center position
	const float CameraHeightOffset =
		SceneCaptureComp->GetComponentLocation().Z -
		BoxVolume->GetComponentLocation().Z;

	SceneCaptureComp->SetWorldLocation(
		BoxVolume->GetComponentLocation() +
		FVector(0, 0, CameraHeightOffset));
	
	// Locks the look_down rotation
	SceneCaptureComp->SetRelativeRotation(SceneCaptureWorldRotation);//always look down


	//RT prep
	const FVector BoxExtent = BoxVolume->GetScaledBoxExtent();
	const float WorldSize = FMath::Max(BoxExtent.X, BoxExtent.Y) * 2.f;

	PixelResolution = FMath::Clamp(
		FMath::RoundToInt(WorldSize * WorldUnitToPixelRatio),
		MinResolution,
		MaxResolution
	);

	SceneCaptureComp->OrthoWidth = WorldSize;

	UE_LOG(LOSWorldBaker, Log,
		TEXT("AObstacleMaskBaker::BakeObstacleMask >> Resolution %dx%d"),
		PixelResolution, PixelResolution);

	// PASS R
	TArray<AActor*> ShadowActors;
	if (!GetObstacleActors(EObstacleType::ShadowCastable, ShadowActors))
		return;

	UTextureRenderTarget2D* RT_R = CaptureObstaclePass(ShadowActors);

	// PASS G
	TArray<AActor*> LowActors;
	if (!GetObstacleActors(EObstacleType::None_ShadowCastable, LowActors))
		return;

	UTextureRenderTarget2D* RT_G = CaptureObstaclePass(LowActors);

	// MERGE
	UTextureRenderTarget2D* FinalRT = NewObject<UTextureRenderTarget2D>(this);
	FinalRT->RenderTargetFormat = RTF_RGBA8;
	FinalRT->ClearColor = FLinearColor::Black;
	FinalRT->InitAutoFormat(PixelResolution, PixelResolution);
	FinalRT->UpdateResourceImmediate(true);

	MergeRTs_To_RG(RT_R, RT_G, FinalRT);

	// SAVE
	const FString AssetName =
		FString::Printf(TEXT("ObstacleMask_%s"), *GetName());

	const FString PackagePath = AssetSavePath / AssetName;

	UTexture2D* FinalTex =
		UKismetRenderingLibrary::RenderTargetCreateStaticTexture2DEditorOnly(
			FinalRT,
			PackagePath,
			TC_Default,
			TMGS_NoMipmaps
		);

	if (!FinalTex)
	{
		UE_LOG(LOSWorldBaker, Error,
			TEXT("AObstacleMaskBaker::BakeObstacleMask >> Texture creation failed"));
		return;
	}

	FinalTex->CompressionSettings = TC_VectorDisplacementmap;
	FinalTex->SRGB = false;
	FinalTex->MipGenSettings = TMGS_NoMipmaps;
	FinalTex->UpdateResource();

	FAssetRegistryModule::AssetCreated(FinalTex);
	if (!FinalTex->MarkPackageDirty())
	{
		UE_LOG(
			LOSWorldBaker, Warning,
			TEXT("AObstacleMaskBaker::BakeObstacleMask >> Failed to mark package dirty for %s"),
			*FinalTex->GetName()
		);
	}

	LastBakedTexture=FinalTex;//cache the final bake

	UE_LOG(LOSWorldBaker, Log,
		TEXT("AObstacleMaskBaker::BakeObstacleMask >> Baked %s"),
		*FinalTex->GetName());

	RegisterTile();// register on subsystem
}

bool AObstacleMaskBaker::GetObstacleActors(
	EObstacleType ObstacleType,
	TArray<AActor*>& OutActors) const
{
	OutActors.Reset();

	UWorld* World = GetWorld();
	if (!World || !BoxVolume)
	{
		UE_LOG(LOSWorldBaker, Error,
			TEXT("AObstacleMaskBaker::GetObstacleActors >> Invalid world or volume"));
		return false;
	}

	FName RequiredTag;//catcher for tag

	switch (ObstacleType)
	{
	case EObstacleType::ShadowCastable:
		RequiredTag = ObjectType_ShadowCastable;
		break;
	case EObstacleType::None_ShadowCastable:
		RequiredTag = ObjectType_LowObject;
		break;
		
	case EObstacleType::None:
	default:
		UE_LOG(LOSWorldBaker, Error,
			TEXT("AObstacleMaskBaker::GetObstacleActors >> Invalid obstacle type"));
		return false;
	}

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (!Actor)
			continue;

		if (!Actor->ActorHasTag(RequiredTag))
			continue;

		OutActors.Add(Actor);
	}

	UE_LOG(LOSWorldBaker, Log,
		TEXT("AObstacleMaskBaker::GetObstacleActors >> Found %d actors for %s"),
		OutActors.Num(),
		*RequiredTag.ToString());

	return true;
}


UTextureRenderTarget2D* AObstacleMaskBaker::CaptureObstaclePass(
	const TArray<AActor*>& ShowOnlyActors)
{
	UTextureRenderTarget2D* RT = NewObject<UTextureRenderTarget2D>(this);
	RT->RenderTargetFormat = RTF_R32f;
	RT->ClearColor = FLinearColor::Black;
	RT->InitAutoFormat(PixelResolution, PixelResolution);
	RT->UpdateResourceImmediate(true);

	SceneCaptureComp->TextureTarget = RT;
	SceneCaptureComp->ClearShowOnlyComponents();
	SceneCaptureComp->ShowOnlyActors.Empty();

	for (AActor* Actor : ShowOnlyActors)
	{
		SceneCaptureComp->ShowOnlyActors.Add(Actor);
	}
	
	SceneCaptureComp->CaptureScene();

	UE_LOG(LOSWorldBaker, Log,
		TEXT("AObstacleMaskBaker::CaptureObstaclePass >> Captured %d actors"),
		ShowOnlyActors.Num());

	return RT;
}

FMatrix AObstacleMaskBaker::BuildWorldToUV(const FBox2D& WorldBounds)
{
	const FVector2D Size = WorldBounds.GetSize();
	const FVector2D Min = WorldBounds.Min;

	// Scale world -> 0..1
	FMatrix Scale = FScaleMatrix(FVector(1.f / Size.X, 1.f / Size.Y, 1.f));

	// Translate Min -> 0..1
	FVector3f TranslationVec(-Min.X / Size.X, -Min.Y / Size.Y, 0.f);
	FMatrix Translate = FTranslationMatrix(FVector(TranslationVec));

	// First scale, then translate
	return Translate * Scale;
}

void AObstacleMaskBaker::OnBakeRequested(EObstacleBakeRequest Request)
{
	switch (Request)
	{
	case EObstacleBakeRequest::Clear:
		ClearLocalData();
		break;

	case EObstacleBakeRequest::Bake:
		BakeObstacleMask();
		break;

	case EObstacleBakeRequest::Rebuild:
		ClearLocalData();
		BakeObstacleMask();
		break;

	case EObstacleBakeRequest::None:
	default:
		
		UE_LOG(LOSWorldBaker, Log,
				TEXT("AObstacleMaskBaker::OnBakeRequested >> Invalid Request"));
		break;
	}
}

void AObstacleMaskBaker::ClearLocalData()
{
#if WITH_EDITOR
	if (!LastBakedTexture)
		return;

	FString AssetPath = LastBakedTexture->GetPathName();

	// Remove from VisionSubsystem
	if (UWorld* World = GetWorld())
	{
		if (UVisionSubsystem* Vision = World->GetSubsystem<UVisionSubsystem>())
		{
			Vision->RemoveTileByTexture(LastBakedTexture);
		}
	}
#endif
	
	// Null  reference
	LastBakedTexture = nullptr;
	
#if WITH_EDITOR
	// Delete the asset using Editor Asset Library
	if (UEditorAssetLibrary::DoesAssetExist(AssetPath))
	{
		const bool bDeleted = UEditorAssetLibrary::DeleteAsset(AssetPath);
		if (bDeleted)
		{
			UE_LOG(LOSWorldBaker, Log, TEXT("Deleted baked texture asset: %s"), *AssetPath);
		}
		else
		{
			UE_LOG(LOSWorldBaker, Warning, TEXT("Failed to delete baked texture asset: %s"), *AssetPath);
		}
	}

	//trigger GC later
	CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
#endif
}

void AObstacleMaskBaker::RegisterTile()
{
	if (!LastBakedTexture)
	{
		UE_LOG(LOSWorldBaker, Log,
		TEXT("AObstacleMaskBaker::RegisterTile >> Invalid Texture"));
		return;
	}
	if (!TileDataAsset)
	{
		UE_LOG(LOSWorldBaker, Log,
		TEXT("AObstacleMaskBaker::RegisterTile >> Invalid TileDataAsset"));
		return;
	}

	UVisionSubsystem* Vision = GetWorld()->GetSubsystem<UVisionSubsystem>();
	if (!Vision)
	{
		UE_LOG(LOSWorldBaker, Log,
		TEXT("AObstacleMaskBaker::RegisterTile >> Failed to get VisionSubsystem"));
		return;
	}
	
	const FVector Center = BoxVolume->GetComponentLocation();
	const FVector Extent = BoxVolume->GetScaledBoxExtent();

	FBox2D Bounds(
		FVector2D(Center.X - Extent.X, Center.Y - Extent.Y),
		FVector2D(Center.X + Extent.X, Center.Y + Extent.Y));

	FObstacleMaskTile Tile;
	Tile.Mask = LastBakedTexture;
	Tile.WorldBounds = Bounds;
	Tile.WorldSize = Bounds.GetSize();
	Tile.WorldToUV = BuildWorldToUV(Bounds);

	Vision->RegisterObstacleTile(Tile);
	
#if WITH_EDITOR
	if (TileDataAsset) // this is for editor-runtime data link
	{
		TileDataAsset->Tiles.Add(Tile);
		FAssetRegistryModule::AssetCreated(TileDataAsset);
		TileDataAsset->MarkPackageDirty();
		UE_LOG(LOSWorldBaker, Log,
			TEXT("AObstacleMaskBaker::RegisterTile >> Added tile to DataAsset %s"),
			*TileDataAsset->GetName());
	}
#endif
}

#endif


UTexture2D* AObstacleMaskBaker::GetBakedObstacleMask() const
{
	if (!LastBakedTexture)
	{
		UE_LOG(LOSWorldBaker, Warning,
			TEXT("AObstacleMaskBaker::GetBakedObstacleMask >> No baked texture"));
	}

	return LastBakedTexture;
}

