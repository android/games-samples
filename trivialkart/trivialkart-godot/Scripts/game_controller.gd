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

extends Node

const _SCROLL_SPEED = 256.0
const _SCROLL_START_X = 256.0
const _SCROLL_WRAP_X = -3072.0

var _current_scroll_x


func _ready():
	_current_scroll_x = _SCROLL_START_X
	$Environment.position.x = _current_scroll_x
	$GaragePage.visible = false
	$StorePage.visible = false
	get_node("Car").connect("car_out_of_gas", self, "_on_car_out_of_gas")
	get_node("GameDataManager").connect("current_bg_changed", self, "_on_current_bg_changed")
	get_node("GameDataManager").connect("current_car_changed", self, "_on_current_car_changed")
	get_node("GameDataManager").connect("gas_filled", self, "_on_gas_filled")
	get_node("GameDataManager").connect("gas_used", self, "_on_gas_used")
	get_node("GaragePage").connect("garage_back_pressed", self, "_on_GaragePage_back_pressed")
	get_node("HUDPanel").connect("garage_button_pressed", self, "_on_GarageButton_pressed")
	get_node("HUDPanel").connect("store_button_pressed", self, "_on_StoreButton_pressed")
	get_node("StorePage").connect("store_back_pressed", self, "_on_StorePage_back_pressed")


func _process(delta):
	var car_velocity = $Car.get_current_velocity()
	if (car_velocity > 0.0):
		var delta_scroll_x = (car_velocity * delta)
		_current_scroll_x -= delta_scroll_x
		if _current_scroll_x < _SCROLL_WRAP_X:
			_current_scroll_x -= _SCROLL_WRAP_X
		$Environment.position.x = _current_scroll_x
		#$PositionLabel.text = str(_current_scroll_x)
	#$SpeedLabel.text = str(car_velocity)


func _on_GarageButton_pressed():
	if $GaragePage.visible == false && $StorePage.visible == false:
		$GaragePage.set_garage_visible(true)


func _on_StoreButton_pressed():
	if $GaragePage.visible == false && $StorePage.visible == false:
		$StorePage.set_store_visible(true)


func _on_GaragePage_back_pressed():
	$GaragePage.set_garage_visible(false)


func _on_StorePage_back_pressed():
	$StorePage.set_store_visible(false)


func _on_current_bg_changed():
	$Environment.update_bg_type()


func _on_current_car_changed():
	$Car/CarSprite.update_car_type()


func _on_gas_filled():
	$HUDPanel/OutOfGasLabel.visible = false


func _on_gas_used():
	$HUDPanel/TutorialLabel.visible = false


func _on_car_out_of_gas():
	$HUDPanel/OutOfGasLabel.visible = true
