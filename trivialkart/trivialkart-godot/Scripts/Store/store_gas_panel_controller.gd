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

var _gas_refill_cost

onready var GameDataManager = get_tree().get_root().get_node(GameConstants.GDM_NODEPATH)


func _ready():
	_gas_refill_cost = 0


func updateGasPanel():
	var gas_level = GameDataManager.get_current_gas_level()
	_gas_refill_cost = \
		GameDataManager.calculate_coin_discount((GameConstants.FULL_GAS_LEVEL - gas_level) \
		* GameConstants.COINS_PER_GAS_LEVEL)
	var gas_cost_string = str(_gas_refill_cost)
	$GasPriceLabel.text = ("*" + gas_cost_string)
	$ConfirmGasPanel/MessageLabel.text = \
		("Fill the gas tank\nusing " + gas_cost_string + " coins?")
	$NeedMoreCoinsLabel.visible = false
	if _gas_refill_cost > 0:
		self.modulate = Color(1.0, 1.0, 1.0, 1.0)
	else:
		self.modulate = Color(0.5, 0.5, 0.5, 1.0)


func _on_GasButton_pressed():
	if _gas_refill_cost > 0:
		if GameDataManager.get_current_coins() < _gas_refill_cost:
			$NeedMoreCoinsLabel.visible = true
		else:
			$ConfirmGasPanel.visible = true


func _on_YesButton_pressed():
	GameDataManager.set_current_coins(GameDataManager.get_current_coins() - _gas_refill_cost)
	GameDataManager.set_current_gas_level(GameConstants.FULL_GAS_LEVEL)
	$ConfirmGasPanel.visible = false
	updateGasPanel()


func _on_NoButton_pressed():
	$ConfirmGasPanel.visible = false
