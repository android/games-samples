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
#include "TextureResource.h"
#include "Widgets/GarageWidget.h"
#include "Widgets/PGSWidget.h"
#include "Widgets/PlayBoardWidget.h"
#include "Widgets/StoreWidget.h"

DEFINE_LOG_CATEGORY(LogTemplateHUD);

namespace
{
	float DistToSegment2D(float Px, float Py, float Ax, float Ay, float Bx, float By)
	{
		const float Vx = Bx - Ax;
		const float Vy = By - Ay;
		const float Wx = Px - Ax;
		const float Wy = Py - Ay;
		const float C1 = Wx * Vx + Wy * Vy;
		if (C1 <= 0.0f)
		{
			return FMath::Sqrt(Wx * Wx + Wy * Wy);
		}
		const float C2 = Vx * Vx + Vy * Vy;
		if (C2 <= C1)
		{
			const float Dx = Px - Bx;
			const float Dy = Py - By;
			return FMath::Sqrt(Dx * Dx + Dy * Dy);
		}
		const float T = C1 / C2;
		const float ProjX = Ax + T * Vx;
		const float ProjY = Ay + T * Vy;
		const float Dx = Px - ProjX;
		const float Dy = Py - ProjY;
		return FMath::Sqrt(Dx * Dx + Dy * Dy);
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

				const float DIndex = DistToSegment2D(Px, Py, 44.0f, 26.0f, 44.0f, 58.0f) - 6.2f;
				const float DMiddle = DistToSegment2D(Px, Py, 55.5f, 43.0f, 55.5f, 60.0f) - 5.4f;
				const float DRing = DistToSegment2D(Px, Py, 66.0f, 47.0f, 66.0f, 62.0f) - 5.1f;
				const float DPinky = DistToSegment2D(Px, Py, 75.5f, 52.0f, 74.0f, 66.0f) - 4.7f;
				const float DThumb = DistToSegment2D(Px, Py, 28.0f, 54.0f, 42.0f, 68.0f) - 5.8f;
				const float DPalmUpper = DistToSegment2D(Px, Py, 44.0f, 58.0f, 69.0f, 63.0f) - 12.5f;
				const float DPalmLower = DistToSegment2D(Px, Py, 48.0f, 69.0f, 61.0f, 72.0f) - 14.5f;

				float HandDist = FMath::Min(DIndex, DMiddle);
				HandDist = FMath::Min(HandDist, DRing);
				HandDist = FMath::Min(HandDist, DPinky);
				HandDist = FMath::Min(HandDist, DThumb);
				HandDist = FMath::Min(HandDist, DPalmUpper);
				HandDist = FMath::Min(HandDist, DPalmLower);

				const float DGroove1 = DistToSegment2D(Px, Py, 50.0f, 41.0f, 50.0f, 53.0f);
				const float DGroove2 = DistToSegment2D(Px, Py, 61.0f, 45.0f, 61.0f, 55.0f);
				const float DGroove3 = DistToSegment2D(Px, Py, 71.0f, 50.0f, 71.0f, 58.0f);
				const float GrooveDist = FMath::Min(DGroove1, FMath::Min(DGroove2, DGroove3));

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
		UE_LOG(LogTemplateHUD, Warning, TEXT("Widget is not on Screen"));
		return;
	}
	if (UUserWidget* ExistingWidget = ExistingPtr->Get())
	{
		ExistingWidget->RemoveFromParent();
	}
	ActiveWidgets.Remove(WidgetType);
	SyncGameplayInputContext();

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

bool ATrivialKartHUD::HandleGamepadKey(const FKey& Key, bool bIsPressed, bool bIsRepeat)
{
	if (UStoreWidget* Store = GetActiveWidget<UStoreWidget>(EWidgetType::Store))
	{
		return Store->HandleGamepadKey(Key, bIsPressed);
	}
	if (UGarageWidget* Garage = GetActiveWidget<UGarageWidget>(EWidgetType::Garage))
	{
		return Garage->HandleGamepadKey(Key, bIsPressed);
	}
	if (UPGSWidget* PGS = GetActiveWidget<UPGSWidget>(EWidgetType::PGS))
	{
		return PGS->HandleGamepadKey(Key, bIsPressed);
	}
	if (UPlayBoardWidget* PlayBoard = GetActiveWidget<UPlayBoardWidget>(EWidgetType::PlayBoard))
	{
		return PlayBoard->HandleGamepadKey(Key, bIsPressed, bIsRepeat);
	}
	return false;
}

bool ATrivialKartHUD::IsSubMenuOpen() const
{
	return GetActiveWidget(EWidgetType::Store) != nullptr
		|| GetActiveWidget(EWidgetType::Garage) != nullptr
		|| GetActiveWidget(EWidgetType::PGS) != nullptr;
}

bool ATrivialKartHUD::IsUIBlockingInput() const
{
	if (IsSubMenuOpen())
	{
		return true;
	}
	const UPlayBoardWidget* PlayBoard = GetActiveWidget<UPlayBoardWidget>(EWidgetType::PlayBoard);
	return PlayBoard && PlayBoard->HasBlockingMenuFocus();
}

bool ATrivialKartHUD::IsStickNavigationEnabled() const
{
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
	if (UStoreWidget* Store = GetActiveWidget<UStoreWidget>(EWidgetType::Store))
	{
		Store->CancelPendingPress();
	}
	if (UGarageWidget* Garage = GetActiveWidget<UGarageWidget>(EWidgetType::Garage))
	{
		Garage->CancelPendingPress();
	}
	if (UPGSWidget* PGS = GetActiveWidget<UPGSWidget>(EWidgetType::PGS))
	{
		PGS->CancelPendingPress();
	}
	if (UPlayBoardWidget* PlayBoard = GetActiveWidget<UPlayBoardWidget>(EWidgetType::PlayBoard))
	{
		PlayBoard->CancelPendingPress();
	}
}

void ATrivialKartHUD::RefreshActiveSubMenu()
{
	if (UStoreWidget* Store = GetActiveWidget<UStoreWidget>(EWidgetType::Store))
	{
		Store->RefreshOwnershipAndFocus(false);
	}
	if (UGarageWidget* Garage = GetActiveWidget<UGarageWidget>(EWidgetType::Garage))
	{
		Garage->RefreshOwnershipAndFocus(false);
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
