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

extends Control

const GAS_EPSILON = 0.01
const GAS_METER_SPEED = 0.2

var current_gas_level = 1.0
var target_gas_level = 1.0
var gas_bar_width
var GameDataManager


func _ready():
	gas_bar_width = $TextureRect.rect_size.x
	GameDataManager = get_tree().get_root().get_node(GameConstants.GDM_NODEPATH)
	GameDataManager.connect("gas_filled", self, "_on_gas_filled")
	GameDataManager.connect("gas_used", self, "_on_gas_used")


func _set_target_gas_level(targetLevel):
	target_gas_level = targetLevel


func _process(delta):
	var update_gas_bar = true
	var gas_delta = abs(target_gas_level - current_gas_level)
	if gas_delta > 0.0 and gas_delta <= GAS_EPSILON:
		current_gas_level = target_gas_level
	elif target_gas_level < current_gas_level:
		current_gas_level -= (GAS_METER_SPEED * delta)
	elif target_gas_level > current_gas_level:
		current_gas_level += (GAS_METER_SPEED * delta)
	else:
		update_gas_bar = false
		
	if update_gas_bar == true:
		$ColorRect.rect_size.x = gas_bar_width * current_gas_level


func _on_gas_filled():
	_set_target_gas_level(1.0)


func _on_gas_used():
	var current = float(GameDataManager.get_current_gas_level())
	var full = float(GameConstants.FULL_GAS_LEVEL)
	var new_target = current / full 
	_set_target_gas_level(new_target)
