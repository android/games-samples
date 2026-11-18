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

#include "Controllers/TrivialKartPlayerController.h"
#include "Engine/Texture2D.h"
#include "GameInstances/TrivialKartGameInstance.h"
#include "TextureResource.h"
#include "Widgets/PlayBoardWidget.h"
#include "Widgets/TrivialKartTabbedModalWidget.h"

DEFINE_LOG_CATEGORY(LogTemplateHUD);

namespace
{
	float DistToSegmentSq2D(float Px, float Py, float Ax, float Ay, float Bx, float By)
	{
		const float Vx = Bx - Ax;
		const float Vy = By - Ay;
		const float Wx = Px - Ax;
		const float Wy = Py - Ay;
		const float C1 = Wx * Vx + Wy * Vy;
		if (C1 <= 0.0f)
		{
			return Wx * Wx + Wy * Wy;
		}
		const float C2 = Vx * Vx + Vy * Vy;
		if (C2 <= C1)
		{
			const float Dx = Px - Bx;
			const float Dy = Py - By;
			return Dx * Dx + Dy * Dy;
		}
		const float T = C1 / C2;
		const float Dx = Px - (Ax + T * Vx);
		const float Dy = Py - (Ay + T * Vy);
		return Dx * Dx + Dy * Dy;
	}

	float SignedCapsuleDist2D(float Px, float Py, float Ax, float Ay, float Bx, float By, float Radius)
	{
		return FMath::Sqrt(DistToSegmentSq2D(Px, Py, Ax, Ay, Bx, By)) - Radius;
	}

	UTexture2D* BuildProceduralTouchHandTexture()
	{
		constexpr int32 Size = 96;
		UTexture2D* Texture = UTexture2D::CreateTransient(Size, Size, PF_B8G8R8A8);
		if (!Texture || !Texture->GetPlatformData() || Texture->GetPlatformData()->Mips.Num() == 0)
		{
			return nullptr;
		}
		Texture->SRGB = true;
		Texture->Filter = TF_Bilinear;

		FTexture2DMipMap& Mip = Texture->GetPlatformData()->Mips[0];
		uint8* Pixels = static_cast<uint8*>(Mip.BulkData.Lock(LOCK_READ_WRITE));
		if (!Pixels)
		{
			return nullptr;
		}

		for (int32 Y = 0; Y < Size; ++Y)
		{
			for (int32 X = 0; X < Size; ++X)
			{
				const float Px = static_cast<float>(X) + 0.5f;
				const float Py = static_cast<float>(Y) + 0.5f;

				float HandDist = SignedCapsuleDist2D(Px, Py, 44.0f, 26.0f, 44.0f, 58.0f, 6.2f);
				HandDist = FMath::Min(HandDist, SignedCapsuleDist2D(Px, Py, 55.5f, 43.0f, 55.5f, 60.0f, 5.4f));
				HandDist = FMath::Min(HandDist, SignedCapsuleDist2D(Px, Py, 66.0f, 47.0f, 66.0f, 62.0f, 5.1f));
				HandDist = FMath::Min(HandDist, SignedCapsuleDist2D(Px, Py, 75.5f, 52.0f, 74.0f, 66.0f, 4.7f));
				HandDist = FMath::Min(HandDist, SignedCapsuleDist2D(Px, Py, 28.0f, 54.0f, 42.0f, 68.0f, 5.8f));
				HandDist = FMath::Min(HandDist, SignedCapsuleDist2D(Px, Py, 44.0f, 58.0f, 69.0f, 63.0f, 12.5f));
				HandDist = FMath::Min(HandDist, SignedCapsuleDist2D(Px, Py, 48.0f, 69.0f, 61.0f, 72.0f, 14.5f));

				float GrooveDist = 100.0f;
				if (HandDist < -1.0f)
				{
					const float DGrooveSq1 = DistToSegmentSq2D(Px, Py, 50.0f, 41.0f, 50.0f, 53.0f);
					const float DGrooveSq2 = DistToSegmentSq2D(Px, Py, 61.0f, 45.0f, 61.0f, 55.0f);
					const float DGrooveSq3 = DistToSegmentSq2D(Px, Py, 71.0f, 50.0f, 71.0f, 58.0f);
					GrooveDist = FMath::Sqrt(FMath::Min(DGrooveSq1, FMath::Min(DGrooveSq2, DGrooveSq3)));
				}

				const float TipDist = FVector2D::Distance(FVector2D(Px, Py), FVector2D(44.0f, 24.0f));
				float RippleDist = 100.0f;
				if (Py < 31.0f)
				{
					const float InnerRing = FMath::Abs(TipDist - 13.5f) - 1.6f;
					const float OuterRing = (Py < 25.0f) ? (FMath::Abs(TipDist - 20.5f) - 1.5f) : 100.0f;
					RippleDist = FMath::Min(InnerRing, OuterRing);
				}

				FLinearColor OutColor(0.0f, 0.0f, 0.0f, 0.0f);

				if (RippleDist < 1.0f && HandDist > 1.5f)
				{
					const float RippleAlpha = FMath::Clamp(1.0f - RippleDist, 0.0f, 1.0f);
					OutColor = FLinearColor(1.0f, 0.86f, 0.22f, RippleAlpha * 0.95f);
				}

				if (HandDist < 2.2f)
				{
					const float OuterAlpha = FMath::Clamp(2.2f - HandDist, 0.0f, 1.0f);
					const float FillFactor = FMath::Clamp(0.5f - HandDist, 0.0f, 1.0f);
					const float GrooveFactor = (HandDist < -1.0f) ? FMath::Clamp(1.4f - GrooveDist, 0.0f, 1.0f) : 0.0f;

					const FLinearColor OutlineColor(0.05f, 0.08f, 0.16f, 1.0f);
					const FLinearColor FillColor(1.0f, 0.97f, 0.88f, 1.0f);
					FLinearColor HandColor = FMath::Lerp(OutlineColor, FillColor, FillFactor * (1.0f - GrooveFactor * 0.85f));
					HandColor.A = OuterAlpha;
					OutColor = HandColor;
				}

				const int32 Idx = (Y * Size + X) * 4;
				Pixels[Idx + 0] = static_cast<uint8>(FMath::Clamp(OutColor.B * 255.0f, 0.0f, 255.0f));
				Pixels[Idx + 1] = static_cast<uint8>(FMath::Clamp(OutColor.G * 255.0f, 0.0f, 255.0f));
				Pixels[Idx + 2] = static_cast<uint8>(FMath::Clamp(OutColor.R * 255.0f, 0.0f, 255.0f));
				Pixels[Idx + 3] = static_cast<uint8>(FMath::Clamp(OutColor.A * 255.0f, 0.0f, 255.0f));
			}
		}

		Mip.BulkData.Unlock();
		Texture->UpdateResource();
		return Texture;
	}
}

bool ATrivialKartHUD::IsSubMenuWidgetType(EWidgetType WidgetType)
{
	return WidgetType == EWidgetType::Garage
		|| WidgetType == EWidgetType::PGS
		|| WidgetType == EWidgetType::Store;
}

void ATrivialKartHUD::BeginPlay()
{
	Super::BeginPlay();
	if (const UTrivialKartGameInstance* Instance = Cast<UTrivialKartGameInstance>(GetGameInstance()))
	{
		if (Instance->IsCloudSyncInFlight())
		{
			AddWidgetToScreen(EWidgetType::Loading, 10);
		}
	}
	AddWidgetToScreen(EWidgetType::PlayBoard);
}

void ATrivialKartHUD::AddWidgetToScreen(const EWidgetType WidgetType, int32 ZOrder)
{
#if WITH_EDITOR
	if (WidgetType == EWidgetType::Loading)
	{
		return;
	}
#endif

	if (GetActiveWidget(WidgetType) != nullptr)
	{
		return;
	}

	if (IsSubMenuWidgetType(WidgetType) && IsSubMenuOpen())
	{
		return;
	}

	if (!WidgetTemplates.Contains(WidgetType))
	{
		UE_LOG(LogTemplateHUD, Warning, TEXT("Widget template does not exist"));
		return;
	}

	const TWeakObjectPtr<UUserWidget> CurrentWidget = CreateWidget<UUserWidget>(GetOwningPlayerController(), WidgetTemplates[WidgetType]);
	if (CurrentWidget.IsValid())
	{
		if (WidgetType == EWidgetType::Loading)
		{
			CancelPendingPresses();
		}
		CurrentWidget->AddToViewport(ZOrder);
		ActiveWidgets.Emplace(WidgetType, CurrentWidget);
		SyncGameplayInputContext();

		if (ATrivialKartPlayerController* PlayerController = Cast<ATrivialKartPlayerController>(GetOwningPlayerController()))
		{
			PlayerController->LatchStickUntilNeutral();
			PlayerController->RestoreFocusToGameViewport();
		}
	}
}

void ATrivialKartHUD::RemoveWidgetFromScreen(const EWidgetType WidgetType)
{
	const TWeakObjectPtr<UUserWidget>* ExistingPtr = ActiveWidgets.Find(WidgetType);
	if (!ExistingPtr)
	{
		if (WidgetType == EWidgetType::Loading)
		{
			RefreshActiveSubMenu();
		}
		else
		{
			UE_LOG(LogTemplateHUD, Warning, TEXT("Widget is not on Screen"));
		}
		return;
	}
	if (UUserWidget* ExistingWidget = ExistingPtr->Get())
	{
		ExistingWidget->RemoveFromParent();
	}
	ActiveWidgets.Remove(WidgetType);
	SyncGameplayInputContext();

	if (WidgetType == EWidgetType::Loading)
	{
		RefreshActiveSubMenu();
	}

	if (ATrivialKartPlayerController* PlayerController = Cast<ATrivialKartPlayerController>(GetOwningPlayerController()))
	{
		PlayerController->LatchStickUntilNeutral();
		PlayerController->RestoreFocusToGameViewport();
	}
}

UUserWidget* ATrivialKartHUD::GetWidgetOfType(const EWidgetType WidgetType)
{
	if (UUserWidget* ActiveWidget = GetActiveWidget(WidgetType))
	{
		return ActiveWidget;
	}
	return nullptr;
}

UTrivialKartTabbedModalWidget* ATrivialKartHUD::GetActiveSubMenuWidget() const
{
	if (UTrivialKartTabbedModalWidget* Store = GetActiveWidget<UTrivialKartTabbedModalWidget>(EWidgetType::Store))
	{
		return Store;
	}
	if (UTrivialKartTabbedModalWidget* Garage = GetActiveWidget<UTrivialKartTabbedModalWidget>(EWidgetType::Garage))
	{
		return Garage;
	}
	return GetActiveWidget<UTrivialKartTabbedModalWidget>(EWidgetType::PGS);
}

bool ATrivialKartHUD::HandleGamepadKey(const FKey& Key, bool bIsPressed, bool bIsRepeat)
{
	if (IsLoadingOpen())
	{
		return bIsPressed;
	}
	if (UTrivialKartTabbedModalWidget* SubMenu = GetActiveSubMenuWidget())
	{
		return SubMenu->HandleGamepadKey(Key, bIsPressed);
	}
	if (UPlayBoardWidget* PlayBoard = GetActiveWidget<UPlayBoardWidget>(EWidgetType::PlayBoard))
	{
		return PlayBoard->HandleGamepadKey(Key, bIsPressed, bIsRepeat);
	}
	return false;
}

bool ATrivialKartHUD::IsLoadingOpen() const
{
	return GetActiveWidget(EWidgetType::Loading) != nullptr;
}

bool ATrivialKartHUD::IsSubMenuOpen() const
{
	return GetActiveSubMenuWidget() != nullptr;
}

bool ATrivialKartHUD::IsUIBlockingInput() const
{
	if (IsLoadingOpen() || IsSubMenuOpen())
	{
		return true;
	}
	const UPlayBoardWidget* PlayBoard = GetActiveWidget<UPlayBoardWidget>(EWidgetType::PlayBoard);
	return PlayBoard && PlayBoard->HasBlockingMenuFocus();
}

bool ATrivialKartHUD::IsStickNavigationEnabled() const
{
	if (IsLoadingOpen())
	{
		return false;
	}
	if (IsSubMenuOpen())
	{
		return true;
	}
	const ATrivialKartPlayerController* PC = Cast<ATrivialKartPlayerController>(GetOwningPlayerController());
	if (!PC || PC->GetActiveInputMode() != ETrivialKartInputMode::Gamepad)
	{
		return false;
	}
	const UPlayBoardWidget* PlayBoard = GetActiveWidget<UPlayBoardWidget>(EWidgetType::PlayBoard);
	return PlayBoard != nullptr;
}

void ATrivialKartHUD::SyncGameplayInputContext() const
{
	if (ATrivialKartPlayerController* PlayerController = Cast<ATrivialKartPlayerController>(GetOwningPlayerController()))
	{
		PlayerController->SetGameplayInputEnabled(!IsUIBlockingInput());
	}
}

void ATrivialKartHUD::ClearPlayBoardMenuFocus()
{
	if (UPlayBoardWidget* PlayBoard = GetActiveWidget<UPlayBoardWidget>(EWidgetType::PlayBoard))
	{
		PlayBoard->ClearMenuFocus(false);
	}
}

void ATrivialKartHUD::NotifyViewportPointerDown(const FVector2D& ScreenSpacePosition)
{
	if (UPlayBoardWidget* PlayBoard = GetActiveWidget<UPlayBoardWidget>(EWidgetType::PlayBoard))
	{
		PlayBoard->OnViewportPointerDown(ScreenSpacePosition);
	}
}

void ATrivialKartHUD::CancelPendingPresses()
{
	if (UTrivialKartTabbedModalWidget* SubMenu = GetActiveSubMenuWidget())
	{
		SubMenu->CancelPendingPress();
	}
	if (UPlayBoardWidget* PlayBoard = GetActiveWidget<UPlayBoardWidget>(EWidgetType::PlayBoard))
	{
		PlayBoard->CancelPendingPress();
	}
}

void ATrivialKartHUD::RefreshActiveSubMenu()
{
	if (UTrivialKartTabbedModalWidget* SubMenu = GetActiveSubMenuWidget())
	{
		SubMenu->RefreshOwnershipAndFocus(false);
	}
}

UTexture2D* ATrivialKartHUD::GetOrCreateTouchHandTexture()
{
	if (!IsValid(CachedTouchHandTexture))
	{
		CachedTouchHandTexture = BuildProceduralTouchHandTexture();
	}
	return CachedTouchHandTexture;
}
