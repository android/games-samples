/*
* Copyright 2026 The Android Open Source Project
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       https://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 */


#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "TrivialKartHUD.generated.h"


DECLARE_LOG_CATEGORY_EXTERN(LogTemplateHUD, Log, All);

UENUM(BlueprintType)
enum class EWidgetType : uint8
{
	None,
	Loading,
	PlayBoard,
	Garage,
	PGS,
	Store
};
/**
 * 
 */
UCLASS()
class TRIVIALKART_UNREAL_API ATrivialKartHUD : public AHUD
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere)
	TMap<EWidgetType, TSubclassOf<UUserWidget>> WidgetTemplates;
	
	UPROPERTY()
	TMap<EWidgetType, TWeakObjectPtr<UUserWidget>> ActiveWidgets;
	
public:
	virtual void BeginPlay() override;
	
	void AddWidgetToScreen(const EWidgetType WidgetType, int32 ZOrder = 0);
	
	void RemoveWidgetFromScreen(const EWidgetType WidgetType);
	
	UUserWidget* GetWidgetOfType(const EWidgetType WidgetType);
	
	
};
