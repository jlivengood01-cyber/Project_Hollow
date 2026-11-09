// Copyright Epic Games, Inc. All Rights Reserved.

#include "The_Hollow.h"

#define LOCTEXT_NAMESPACE "FThe_HollowModule"

void FThe_HollowModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
}

void FThe_HollowModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FThe_HollowModule, The_Hollow)