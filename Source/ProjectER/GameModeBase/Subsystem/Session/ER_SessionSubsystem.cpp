#include "ER_SessionSubsystem.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "Online/OnlineSessionNames.h"

// If you want FOnlineSessionSearch, it must be included here but usually OnlineSessionSettings.h covers it.
// If missing, consider #include "Interfaces/OnlineSessionInterface.h" which is already in the header.

UER_SessionSubsystem::UER_SessionSubsystem()
{
}

void UER_SessionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	const IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	if (OnlineSubsystem != nullptr)
	{
		SessionInterface = OnlineSubsystem->GetSessionInterface();
	}
}

void UER_SessionSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

void UER_SessionSubsystem::CreateSession(int32 NumPublicConnections, bool bIsLANMatch)
{
	if (!SessionInterface.IsValid())
	{
		return;
	}

	auto ExistingSession = SessionInterface->GetNamedSession(NAME_GameSession);
	if (ExistingSession != nullptr)
	{
		SessionInterface->DestroySession(NAME_GameSession);
	}

	// Register Delegate
	SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(FOnCreateSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnCreateSessionComplete));

	// Setup settings
	TSharedPtr<FOnlineSessionSettings> SessionSettings = MakeShareable(new FOnlineSessionSettings());
	
	// Force LAN match off for Steam Testing (Bypasses SO_BROADCAST crash)
	UE_LOG(LogTemp, Warning, TEXT("[ER_SessionSubsystem] CreateSession called with bIsLANMatch = %s. Forcing to FALSE for Steam."), bIsLANMatch ? TEXT("TRUE") : TEXT("FALSE"));
	bool bActualLANMatch = false;

	SessionSettings->bIsLANMatch = bActualLANMatch;
	SessionSettings->NumPublicConnections = NumPublicConnections;
	SessionSettings->bAllowJoinInProgress = true;
	SessionSettings->bAllowJoinViaPresence = true;
	SessionSettings->bShouldAdvertise = true;
	SessionSettings->bUsesPresence = true;
	SessionSettings->bUseLobbiesIfAvailable = true;
	SessionSettings->Set(FName("CUSTOM_GAME_ID"), FString("ProjectER_Sparta"), EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

	// Get Local Player

	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	if (LocalPlayer != nullptr)
	{
		SessionInterface->CreateSession(*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSession, *SessionSettings);
	}
}

void UER_SessionSubsystem::FindSessions(int32 MaxSearchResults, bool bIsLANMatch)
{
	if (!SessionInterface.IsValid())
	{
		return;
	}

	SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(FOnFindSessionsCompleteDelegate::CreateUObject(this, &ThisClass::OnFindSessionsComplete));

	// Force LAN match off for Steam Testing (Bypasses SO_BROADCAST crash if Blueprint passes true)
	UE_LOG(LogTemp, Warning, TEXT("[ER_SessionSubsystem] FindSessions called with bIsLANMatch = %s. Forcing to FALSE for Steam."), bIsLANMatch ? TEXT("TRUE") : TEXT("FALSE"));
	bool bActualLANMatch = false;

	LastSessionSearch = MakeShareable(new FOnlineSessionSearch());
	LastSessionSearch->MaxSearchResults = MaxSearchResults;
	LastSessionSearch->bIsLanQuery = bActualLANMatch;
	LastSessionSearch->QuerySettings.Set(FName("PRESENCESEARCH"), true, EOnlineComparisonOp::Equals);
	LastSessionSearch->QuerySettings.Set(FName("LOBBYSEARCH"), true, EOnlineComparisonOp::Equals);
	LastSessionSearch->QuerySettings.Set(FName("CUSTOM_GAME_ID"), FString("ProjectER_Sparta"), EOnlineComparisonOp::Equals);

	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	if (LocalPlayer != nullptr)
	{
		SessionInterface->FindSessions(*LocalPlayer->GetPreferredUniqueNetId(), LastSessionSearch.ToSharedRef());
	}
}

void UER_SessionSubsystem::JoinSession(int32 SessionIndex)
{
	if (!SessionInterface.IsValid())
	{
		return;
	}

	if (!LastSessionSearch.IsValid() || LastSessionSearch->SearchResults.Num() <= SessionIndex || SessionIndex < 0)
	{
		return;
	}

	SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(FOnJoinSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnJoinSessionComplete));

	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	if (LocalPlayer != nullptr)
	{
		SessionInterface->JoinSession(*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSession, LastSessionSearch->SearchResults[SessionIndex]);
	}
}

void UER_SessionSubsystem::DestroyGameSession()
{
	if (!SessionInterface.IsValid())
	{
		return;
	}

	SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(FOnDestroySessionCompleteDelegate::CreateUObject(this, &ThisClass::OnDestroySessionComplete));
	SessionInterface->DestroySession(NAME_GameSession);
}

void UER_SessionSubsystem::StartGameSession()
{
	if (!SessionInterface.IsValid())
	{
		return;
	}

	SessionInterface->AddOnStartSessionCompleteDelegate_Handle(FOnStartSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnStartSessionComplete));
	SessionInterface->StartSession(NAME_GameSession);
}

void UER_SessionSubsystem::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnCreateSessionCompleteDelegates(this);
	}

	OnCreateSessionCompleteEvent.Broadcast(bWasSuccessful);
}

void UER_SessionSubsystem::OnFindSessionsComplete(bool bWasSuccessful)
{
	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnFindSessionsCompleteDelegates(this);
	}

	OnFindSessionsCompleteEvent.Broadcast(bWasSuccessful);
}

void UER_SessionSubsystem::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnJoinSessionCompleteDelegates(this);
	}

	bool bWasSuccessful = (Result == EOnJoinSessionCompleteResult::Success);
	OnJoinSessionCompleteEvent.Broadcast(bWasSuccessful);

	// ClientTravel if Join is Successful
	if (bWasSuccessful)
	{
		FString ConnectInfo;
		if (SessionInterface->GetResolvedConnectString(SessionName, ConnectInfo))
		{
			if (GetWorld() != nullptr)
			{
				APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
				if (PlayerController != nullptr)
				{
					PlayerController->ClientTravel(ConnectInfo, ETravelType::TRAVEL_Absolute);
				}
			}
		}
	}
}

void UER_SessionSubsystem::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnDestroySessionCompleteDelegates(this);
	}

	OnDestroySessionCompleteEvent.Broadcast(bWasSuccessful);
}

void UER_SessionSubsystem::OnStartSessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnStartSessionCompleteDelegates(this);
	}

	OnStartSessionCompleteEvent.Broadcast(bWasSuccessful);
}

TArray<FSessionResultWrapper> UER_SessionSubsystem::GetCustomSearchResults() const
{
	TArray<FSessionResultWrapper> CustomResults;

	if (LastSessionSearch.IsValid())
	{
		const TArray<FOnlineSessionSearchResult>& SearchResults = LastSessionSearch->SearchResults;
		for (int32 i = 0; i < SearchResults.Num(); ++i)
		{
			const FOnlineSessionSearchResult& Result = SearchResults[i];
			if (Result.IsValid())
			{
				FSessionResultWrapper Wrapper;
				Wrapper.SessionIndex = i;
				Wrapper.Ping = Result.PingInMs;
				Wrapper.MaxPlayers = Result.Session.SessionSettings.NumPublicConnections;
				Wrapper.CurrentPlayers = Wrapper.MaxPlayers - Result.Session.NumOpenPublicConnections;
				Wrapper.OwningUserName = Result.Session.OwningUserName;
				
				CustomResults.Add(Wrapper);
			}
		}
	}

	return CustomResults;
}
