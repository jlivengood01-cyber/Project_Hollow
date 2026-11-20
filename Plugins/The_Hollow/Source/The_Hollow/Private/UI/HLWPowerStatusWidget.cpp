#include "UI/HLWPowerStatusWidget.h"

#include "Components/TextBlock.h"
#include "Electrical/HLWFacilityPowerSubsystem.h"
#include "Electrical/HLWPowerGenerator.h"
#include "Engine/World.h"
#include "TimerManager.h"

#define LOCTEXT_NAMESPACE "HLWPowerStatus"

void UHLWPowerStatusWidget::BuildDefaultLayout()
{
	UVerticalBox* Box = MakeDefaultRoot(FVector2D(0.82f, 0.3f));
	TitleText = MakeText(Box, TEXT("TitleText"), 16, FLinearColor::White);
	StatusText = MakeText(Box, TEXT("StatusText"), 12, FLinearColor(0.85f, 0.85f, 0.85f));
	StatusText->SetJustification(ETextJustify::Left);
	TitleText->SetText(LOCTEXT("Title", "Power Status"));
}

void UHLWPowerStatusWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (UHLWFacilityPowerSubsystem* Power = GetWorld() ? GetWorld()->GetSubsystem<UHLWFacilityPowerSubsystem>() : nullptr)
	{
		Power->OnCircuitStateChanged.AddDynamic(this, &ThisClass::HandleCircuitStateChanged);
		Power->OnGeneratorStateChanged.AddDynamic(this, &ThisClass::HandleGeneratorStateChanged);
	}
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(RefreshTimer, this, &ThisClass::Refresh, RefreshInterval, true);
	}
	Refresh();
}

void UHLWPowerStatusWidget::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RefreshTimer);
		if (UHLWFacilityPowerSubsystem* Power = World->GetSubsystem<UHLWFacilityPowerSubsystem>())
		{
			Power->OnCircuitStateChanged.RemoveDynamic(this, &ThisClass::HandleCircuitStateChanged);
			Power->OnGeneratorStateChanged.RemoveDynamic(this, &ThisClass::HandleGeneratorStateChanged);
		}
	}
	Super::NativeDestruct();
}

void UHLWPowerStatusWidget::SetFilter(const TArray<FName>& InGeneratorIDs, const TArray<FName>& InCircuitIDs)
{
	GeneratorIDs = InGeneratorIDs;
	CircuitIDs = InCircuitIDs;
	Refresh();
}

void UHLWPowerStatusWidget::HandleCircuitStateChanged(FName CircuitID, EHLWPowerState State, bool bInitial)
{
	Refresh();
}

void UHLWPowerStatusWidget::HandleGeneratorStateChanged(FName GeneratorID, EHLWGeneratorState State)
{
	Refresh();
}

void UHLWPowerStatusWidget::Refresh()
{
	const UHLWFacilityPowerSubsystem* Power = GetWorld() ? GetWorld()->GetSubsystem<UHLWFacilityPowerSubsystem>() : nullptr;
	if (!StatusText || !Power || !IsVisible())
	{
		return;
	}

	TArray<FString> Lines;

	TArray<FName> Gens = GeneratorIDs.IsEmpty() ? Power->GetGeneratorIDs() : GeneratorIDs;
	Gens.Sort(FNameLexicalLess());
	for (const FName& ID : Gens)
	{
		if (const AHLWPowerGenerator* Generator = Power->FindGenerator(ID))
		{
			FString Line = FString::Printf(TEXT("%s: %s"), *ID.ToString(), *UEnum::GetDisplayValueAsText(Generator->GetGeneratorState()).ToString());
			if (Generator->GetGeneratorData().bRequiresFuel)
			{
				Line += FString::Printf(TEXT(" (fuel %.0f)"), Generator->GetFuel());
			}
			Lines.Add(Line);
		}
	}

	TArray<FName> Circuits = CircuitIDs.IsEmpty() ? Power->GetCircuitIDs() : CircuitIDs;
	Circuits.Sort(FNameLexicalLess());
	for (const FName& ID : Circuits)
	{
		Lines.Add(FString::Printf(TEXT("%s: %s"),
			*Power->GetCircuitDisplayName(ID).ToString(),
			*UEnum::GetDisplayValueAsText(Power->GetCircuitState(ID)).ToString()));
	}

	StatusText->SetText(FText::FromString(FString::Join(Lines, TEXT("\n"))));
}

#undef LOCTEXT_NAMESPACE
