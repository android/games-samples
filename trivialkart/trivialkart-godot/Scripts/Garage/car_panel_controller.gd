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

var car_panel_array

onready var GameDataManager = get_tree().get_root().get_node(GameConstants.GDM_NODEPATH)


func _ready():
	car_panel_array = [$SedanPanel, $TruckPanel, $OffroadPanel, $KartPanel]


func update_car_panel():
	var current_active_car = GameDataManager.get_current_car()
	for car_index in range(0, car_panel_array.size()):
		if (GameDataManager.is_car_unlocked(_CAR_ID_ARRAY[car_index])):
			car_panel_array[car_index].visible = true
			var status_label = car_panel_array[car_index].get_node("StatusLabel")
			status_label.visible = (_CAR_ID_ARRAY[car_index] == current_active_car)
		else:
			car_panel_array[car_index].visible = false


func _on_car_button_pressed(carIndex):
	GameDataManager.set_current_car(_CAR_ID_ARRAY[carIndex])
	update_car_panel()
