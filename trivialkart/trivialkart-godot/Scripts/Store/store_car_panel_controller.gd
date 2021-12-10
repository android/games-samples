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

const _CAR_ID_ARRAY = [
	GameConstants.CarTypes.CAR_SEDAN, 
	GameConstants.CarTypes.CAR_TRUCK, 
	GameConstants.CarTypes.CAR_OFFROAD, 
	GameConstants.CarTypes.CAR_KART,
]

var _car_panel_array
var _pending_product_id
var _purchased_car_id
var _truck_purchase_cost

onready var GameDataManager = get_tree().get_root().get_node(GameConstants.GDM_NODEPATH)
onready var PurchaseManager = get_tree().get_root().get_node(GameConstants.PM_NODEPATH)


func _on_purchase_failed(product_id):
	if product_id == _pending_product_id:
		_pending_product_id = ""


func _on_purchase_successful(product_id):
	if product_id == _pending_product_id:
		_pending_product_id = ""
		update_car_panel()


func _ready():
	_car_panel_array = [$SedanPanel, $TruckPanel, $OffroadPanel, $KartPanel]
	_pending_product_id = ""
	PurchaseManager.connect("purchase_failed", self, "_on_purchase_failed")
	PurchaseManager.connect("purchase_successful", self, "_on_purchase_successful")


func update_car_panel():
	$TruckPanel/NeedMoreCoinsLabel.visible = false
	$ConfirmCarPanel.visible = false
	_truck_purchase_cost = GameDataManager.calculate_coin_discount(GameConstants.TRUCK_COIN_COST)
	for car_index in range(0, _CAR_ID_ARRAY.size()):
		var car_id = _CAR_ID_ARRAY[car_index]
		var PriceLabel = _car_panel_array[car_index].get_node("PriceLabel")
		var PurchaseButton = _car_panel_array[car_index].get_node("PurchaseButton")
		if (GameDataManager.is_car_unlocked(car_id)):
			_car_panel_array[car_index].modulate = Color(0.5, 0.5, 0.5, 1.0)
			PriceLabel.visible = false
			PurchaseButton.disabled = true
			if car_id == GameConstants.CarTypes.CAR_TRUCK:
				_car_panel_array[car_index].get_node("CoinIcon").visible = false
		else:	
			_car_panel_array[car_index].modulate = Color(1.0, 1.0, 1.0, 1.0)
			PriceLabel.visible = true
			PurchaseButton.disabled = false
			# Always update the truck price in case we got a subscription discount
			if car_id == GameConstants.CarTypes.CAR_TRUCK:
				PriceLabel.text = "*" + str(_truck_purchase_cost)


func update_car_prices():
	for car_index in range(0, _CAR_ID_ARRAY.size()):
		var car_id = _CAR_ID_ARRAY[car_index]
		# only update price text for the cars that actually use the store
		if GameDataManager.get_car_store_purchase(car_id) == true:
			var price_key = GameDataManager.get_car_price_key(car_id)
			var price_value = GameDataManager.get_car_price(car_id)
			var PriceLabel = _car_panel_array[car_index].get_node("PriceLabel")
			PriceLabel.text = PriceLabel.text.replace(price_key, price_value)


func _on_purchase_button_pressed(car_index):
	_purchased_car_id = _CAR_ID_ARRAY[car_index]
	# The truck is purchased with coins
	if _purchased_car_id == GameConstants.CarTypes.CAR_TRUCK:
		if GameDataManager.get_current_coins() < _truck_purchase_cost:
			$TruckPanel/NeedMoreCoinsLabel.visible = true
			return
	
	var purchase_message
	if _purchased_car_id == GameConstants.CarTypes.CAR_TRUCK:
		purchase_message = "Purchase for " + str(_truck_purchase_cost) + " coins?"
	else:
		purchase_message = "Purchase for " \
			+ GameDataManager.get_car_price(_purchased_car_id) + "?"
	$ConfirmCarPanel/MessageLabel.text = purchase_message
	$ConfirmCarPanel.visible = true


func _on_YesButton_pressed():
	if _purchased_car_id == GameConstants.CarTypes.CAR_TRUCK:
		GameDataManager.set_current_coins(GameDataManager.get_current_coins() \
			- _truck_purchase_cost)
		GameDataManager.unlock_car(_purchased_car_id)
		update_car_panel()
	else:
		_pending_product_id = GameConstants.get_car_product_id(_purchased_car_id)
		PurchaseManager.begin_purchase(_pending_product_id)
	$ConfirmCarPanel.visible = false


func _on_NoButton_pressed():
	$ConfirmCarPanel.visible = false
