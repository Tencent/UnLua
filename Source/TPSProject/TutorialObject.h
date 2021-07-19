#pragma once

#include "CoreMinimal.h"

struct FTutorialObject
{
protected:
	FString Name;

public:
	FTutorialObject();

	explicit FTutorialObject(const FString& Name)
		:Name(Name)
	{
	}

	FString GetTitle() const
	{
		return FString::Printf(TEXT("《%s》"), *Name);
	}

	FString ToString() const
	{
		return GetTitle();
	}
};