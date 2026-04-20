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


#include "Actors/TrivialKartHUD.h"

#include "Blueprint/UserWidget.h"

DEFINE_LOG_CATEGORY(LogTemplateHUD);

void ATrivialKartHUD::BeginPlay()
{
	Super::BeginPlay();
	AddWidgetToScreen(EWidgetType::Loading, 10);
	AddWidgetToScreen(EWidgetType::PlayBoard);
}

void ATrivialKartHUD::AddWidgetToScreen(const EWidgetType WidgetType, int32 ZOrder)
{
#if WITH_EDITOR
	if (WidgetType == EWidgetType::Loading)
		return;
#endif
	if (!WidgetTemplates.Contains(WidgetType))
	{
UE_LOG(LogTemplateHUD, Warning, TEXT("Widget template does not exist"));
		return;
	}
	const TWeakObjectPtr CurrentWidget = CreateWidget(GetOwningPlayerController(), 
		WidgetTemplates[WidgetType]);
	if (CurrentWidget.IsValid())
	{
		CurrentWidget->AddToViewport(ZOrder);
		ActiveWidgets.Emplace(WidgetType, CurrentWidget);
	}
}

void ATrivialKartHUD::RemoveWidgetFromScreen(const EWidgetType WidgetType)
{
	if (!ActiveWidgets.Contains(WidgetType))
	{
		UE_LOG(LogTemplateHUD, Warning, TEXT("Widget is not on Screen"));
		return;
	}
	ActiveWidgets[WidgetType]->RemoveFromParent();
	ActiveWidgets.Remove(WidgetType);
}

UUserWidget* ATrivialKartHUD::GetWidgetOfType(const EWidgetType WidgetType)
{
	if (!ActiveWidgets.Contains(WidgetType))
	{
		UE_LOG(LogTemplateHUD, Warning, TEXT("Active widget does not exist"));
		return nullptr;
	}
	return ActiveWidgets[WidgetType].Get();
}
