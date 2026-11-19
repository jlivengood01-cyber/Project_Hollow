#include "Access/HLWAccessTypes.h"

#include "Engine/Level.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Misc/PackageName.h"

FName HLWAccess::MakeStableID(const AActor* Actor)
{
	if (!Actor || !Actor->GetLevel())
	{
		return NAME_None;
	}

	// Level package, not the actor's, so one-file-per-actor maps stay stable
	const FString LevelName = FPackageName::GetShortName(UWorld::RemovePIEPrefix(Actor->GetLevel()->GetPackage()->GetName()));
	return FName(*FString::Printf(TEXT("%s.%s"), *LevelName, *Actor->GetName()));
}
