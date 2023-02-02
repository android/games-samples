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

const _SUBSCRIPTION_IDS = [
	GameConstants.SubscriptionType.SUBSCRIPTION_NONE,
	GameConstants.SubscriptionType.SUBSCRIPTION_SILVER,
	GameConstants.SubscriptionType.SUBSCRIPTION_GOLD,
]

var _current_product_id
var _pending_product_id
var _purchased_subscription_id
var _subscription_panel_array

onready var GameDataManager = get_tree().get_root().get_node(GameConstants.GDM_NODEPATH)
onready var PurchaseManager = get_tree().get_root().get_node(GameConstants.PM_NODEPATH)


func _on_purchase_failed(product_id):
	if product_id == _pending_product_id:
		_pending_product_id = ""


func _on_purchase_successful(product_id):
	if product_id == _pending_product_id:
		_pending_product_id = ""
		update_subscription_panel()


func _ready():
	_pending_product_id = ""
	_subscription_panel_array = [null, $SilverPanel, $GoldPanel]
	PurchaseManager.connect("purchase_failed", self, "_on_purchase_failed")
	PurchaseManager.connect("purchase_successful", self, "_on_purchase_successful")


func update_subscription_panel():
	for subscription_index in range(0, _subscription_panel_array.size()):
		if _subscription_panel_array[subscription_index] != null:
			var priceLabel = _subscription_panel_array[subscription_index].get_node("PriceLabel")
			var subscriptionButton = \
					_subscription_panel_array[subscription_index].get_node("SubscriptionButton")
			var subscriptionButton2 = \
					_subscription_panel_array[subscription_index].get_node("SubscriptionButton2")
			if (GameDataManager.get_current_subscription() == \
					_SUBSCRIPTION_IDS[subscription_index]):
				_subscription_panel_array[subscription_index].modulate = Color(0.5, 0.5, 0.5, 1.0)
				priceLabel.visible = false
				subscriptionButton.disabled = true
				subscriptionButton2.disabled = true
				# Uncomment to test price change confirmation flow
				#_current_product_id = GameConstants.get_subscription_product_id( \
				#		GameDataManager.get_current_subscription())
				#_test_price_change()
			else:	
				_subscription_panel_array[subscription_index].modulate = Color(1.0, 1.0, 1.0, 1.0)
				priceLabel.visible = true
				subscriptionButton.disabled = false	
				subscriptionButton2.disabled = false	


func update_subscription_prices():
	for subscription_index in range(0, _SUBSCRIPTION_IDS.size()):
		if _subscription_panel_array[subscription_index] != null:
			var subscription_id = _SUBSCRIPTION_IDS[subscription_index]
			var price_key = GameDataManager.get_subscription_price_key(subscription_id)
			var price_value = GameDataManager.get_subscription_price(subscription_id)
			var priceLabel = _subscription_panel_array[subscription_index].get_node("PriceLabel")
			priceLabel.text = priceLabel.text.replace(price_key, price_value)


func _on_subscribe_button_pressed(subscription_index):
	_purchased_subscription_id = _SUBSCRIPTION_IDS[subscription_index]          
	var purchase_message = "Purchase for " + \
		GameDataManager.get_subscription_price(_purchased_subscription_id) + "/month?"
	$ConfirmSubscriptionPanel/MessageLabel.text = purchase_message
	$ConfirmSubscriptionPanel.visible = true


func _on_YesButton_pressed():
	_pending_product_id = GameConstants.get_subscription_product_id(_purchased_subscription_id)
	if _pending_product_id == "none":
		GameDataManager.set_current_subscription(_purchased_subscription_id)
		update_subscription_panel()
		_pending_product_id = ""
	else:
		PurchaseManager.begin_purchase(_pending_product_id)
	$ConfirmSubscriptionPanel.visible = false


func _on_NoButton_pressed():
	$ConfirmSubscriptionPanel.visible = false


func _test_price_change():
	PurchaseManager.connect("price_change_accepted", self, "_on_price_change_accepted")
	PurchaseManager.connect("price_change_canceled", self, "_on_price_change_canceled")
	PurchaseManager.confirm_price_change(_current_product_id)


func _on_price_change_accepted():
	print("Price change accepted")


func _on_price_change_canceled():
	print("Price change canceled")
