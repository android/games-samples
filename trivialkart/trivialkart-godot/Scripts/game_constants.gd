# Copyright 2021 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

class_name GameConstants
extends Node

enum BGTypes {BG_BLUE, BG_MUSHROOM}
enum CarTypes {CAR_SEDAN, CAR_TRUCK, CAR_OFFROAD, CAR_KART}
enum CoinTypes {COINS_5, COINS_10, COINS_20, COINS_50}
enum SubscriptionType {SUBSCRIPTION_NONE, SUBSCRIPTION_SILVER, SUBSCRIPTION_GOLD}

const GDM_NODEPATH = "Main/GameDataManager"
const PM_NODEPATH = "Main/PurchaseManager"
const FULL_GAS_LEVEL = 5
const COINS_PER_GAS_LEVEL = 2
const TRUCK_COIN_COST = 20
const GOLDEN_SUB_COIN_DISCOUNT = 0.4
const COIN_AWARDS = [5, 10, 20, 50]
const CAR_IS_STORE_PURCHASE = [false, false, true, true]

# These product ID strings must match the product IDs defined in the
# Google Play Store Console entry for the app
const CAR_PRODUCT_IDS = [
	"none", 
	"none", 
	"car_offroad", 
	"car_kart",
]
	
const COIN_PRODUCT_IDS = [
	"five_coins",
	"ten_coins",
	"twenty_coins",
	"fifty_coins",
]

const SUBSCRIPTION_PRODUCT_IDS = [
	"none",
	"silver_subscription",
	"golden_subscription"
]

# Search and replace keys to replace the key text with the current localized
# price retrieved from the store
const CAR_PRICE_LABEL_KEYS = [
	"$CAR_SEDAN",
	"$CAR_TRUCK",
	"$CAR_OFFROAD",
	"$CAR_KART",
]

const COIN_PRICE_LABEL_KEYS = [
	"$FIVE_COINS",
	"$TEN_COINS",
	"$TWENTY_COINS",
	"$FIFTY_COINS",
]

const SUBSCRIPTION_PRICE_LABEL_KEYS = [
	"$NO_SUBSCRIPTION",
	"$SILVER_SUBSCRIPTION",
	"$GOLDEN_SUBSCRIPTION",
]


static func get_bg_index(bgType):
	if bgType == BGTypes.BG_BLUE:
		return 0
	elif bgType == BGTypes.BG_MUSHROOM:
		return 1
	return -1


static func get_car_index(carType):
	if carType == CarTypes.CAR_SEDAN:
		return 0
	elif carType == CarTypes.CAR_TRUCK:
		return 1
	elif carType == CarTypes.CAR_OFFROAD:
		return 2
	elif carType == CarTypes.CAR_KART:
		return 3
	return -1


static func get_coin_index(coinType):
	if (coinType == CoinTypes.COINS_5):
		return 0
	elif (coinType == CoinTypes.COINS_10):
		return 1
	elif (coinType == CoinTypes.COINS_20):
		return 2
	elif (coinType == CoinTypes.COINS_50):
		return 3
	return -1


static func get_subscription_index(subType):
	if (subType == SubscriptionType.SUBSCRIPTION_NONE):
		return 0
	elif (subType == SubscriptionType.SUBSCRIPTION_SILVER):
		return 1
	elif (subType == SubscriptionType.SUBSCRIPTION_GOLD):
		return 2
	return -1


static func get_subscription_type(subIndex):
	if (subIndex == 1):
		return SubscriptionType.SUBSCRIPTION_SILVER
	elif (subIndex == 2):
		return SubscriptionType.SUBSCRIPTION_GOLD
	return SubscriptionType.SUBSCRIPTION_NONE


static func get_car_product_id(carType):
	var car_index = get_car_index(carType)
	if car_index >= 0:
		return CAR_PRODUCT_IDS[car_index]
	return "none"	


static func get_coin_product_id(coinType):
	var coin_index = get_coin_index(coinType)
	if coin_index >= 0:
		return COIN_PRODUCT_IDS[coin_index]
	return "none"	


static func get_subscription_product_id(subType):
	var sub_index = get_subscription_index(subType)
	if sub_index >= 0:
		return SUBSCRIPTION_PRODUCT_IDS[sub_index]
	return "none"
