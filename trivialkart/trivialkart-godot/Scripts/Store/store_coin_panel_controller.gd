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

extends Panel

const _COIN_ID_ARRAY = [
	GameConstants.CoinTypes.COINS_5, 
	GameConstants.CoinTypes.COINS_10, 
	GameConstants.CoinTypes.COINS_20, 
	GameConstants.CoinTypes.COINS_50,
]

var _coin_panel_array
var _coins_to_award
var _pending_product_id

onready var GameDataManager = get_tree().get_root().get_node(GameConstants.GDM_NODEPATH)
onready var PurchaseManager = get_tree().get_root().get_node(GameConstants.PM_NODEPATH)

func _on_purchase_failed(product_id):
	if product_id == _pending_product_id:
		_pending_product_id = ""

func _on_purchase_successful(product_id):
	if product_id == _pending_product_id:
		_pending_product_id = ""

func _ready():
	_coin_panel_array = [$Coin5Panel, $Coin10Panel, $Coin20Panel, $Coin50Panel]
	_pending_product_id = ""
	PurchaseManager.connect("purchase_failed", self, "_on_purchase_failed")
	PurchaseManager.connect("purchase_successful", self, "_on_purchase_successful")

func updateCoinPrices():
	$ConfirmCoinPanel.visible = false
	for coin_index in range(0, _COIN_ID_ARRAY.size()):
		var coin_id = _COIN_ID_ARRAY[coin_index]
		var price_key = GameDataManager.get_coin_price_key(coin_id)
		var price_value = GameDataManager.get_coin_price(coin_id)
		var priceLabel = _coin_panel_array[coin_index].get_node("CoinPriceLabel")
		priceLabel.text = priceLabel.text.replace(price_key, price_value)


func _on_purchase_button_pressed(coin_index):
	_coins_to_award = GameConstants.COIN_AWARDS[coin_index]
	var coin_id = _COIN_ID_ARRAY[coin_index]
	_pending_product_id = GameConstants.get_coin_product_id(coin_id)
	var purchase_message = "Purchase for " + GameDataManager.get_coin_price(coin_id) + "?"
	$ConfirmCoinPanel/MessageLabel.text = purchase_message
	$ConfirmCoinPanel.visible = true


func _on_YesButton_pressed():
	PurchaseManager.begin_purchase(_pending_product_id)
	$ConfirmCoinPanel.visible = false


func _on_NoButton_pressed():
	$ConfirmCoinPanel.visible = false
