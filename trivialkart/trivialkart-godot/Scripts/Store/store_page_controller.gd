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

signal store_back_pressed

export(StreamTexture) var unselected_tab_bg
export(StreamTexture) var selected_tab_bg

var _active_tab_index
var _panel_array
var _tab_array
var _prices_updated

onready var PurchaseManager = get_tree().get_root().get_node(GameConstants.PM_NODEPATH)


func _ready():
	_active_tab_index = 0
	_prices_updated = false
	_tab_array = [$StoreTabs/GasTab, $StoreTabs/CoinTab, $StoreTabs/CarTab, \
		$StoreTabs/SubscriptionTab]
	_panel_array = [$StorePanel/GasPanel, $StorePanel/CoinPanel, $StorePanel/CarPanel, \
		$StorePanel/SubscriptionPanel]
	refresh_tabs()


func set_store_visible(is_visible):
	if is_visible == true:
		PurchaseManager.set_purchase_notifications_enabled(false)
		if _prices_updated == false:
			# One-time update of the prices to use the ones
			# retrieved from the Google Play Store at startup
			$StorePanel/CarPanel.update_car_prices()
			$StorePanel/CoinPanel.updateCoinPrices()
			$StorePanel/SubscriptionPanel.update_subscription_prices()
			_prices_updated = true
		_update_panels()
	else:
		PurchaseManager.set_purchase_notifications_enabled(true)
	self.visible = is_visible	
	
	
func refresh_tabs():
	for tab_index in range(0, _tab_array.size()):
		var tab_selected = true if tab_index == _active_tab_index else false
		update_selected_tab(_tab_array[tab_index], tab_selected)
		update_selected_panel(_panel_array[tab_index], tab_selected)


func update_selected_tab(tab_object, tab_selected):
	var tab_bg = selected_tab_bg if tab_selected else unselected_tab_bg
	
	tab_object.texture_disabled = tab_bg
	tab_object.texture_focused = tab_bg
	tab_object.texture_hover = tab_bg
	tab_object.texture_normal = tab_bg
	tab_object.texture_pressed = tab_bg


func update_selected_panel(panel_object, panel_selected):
	panel_object.visible = panel_selected


func _on_Tab_pressed(tab_index):
	_update_panels()
	_active_tab_index = tab_index
	refresh_tabs()


func _update_panels():
	$StorePanel/CarPanel.update_car_panel()
	$StorePanel/GasPanel.updateGasPanel()
	$StorePanel/SubscriptionPanel.update_subscription_panel()


func _on_BackButton_pressed():
	emit_signal("store_back_pressed")
