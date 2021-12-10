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

extends AnimatedSprite

const _CAR_SPRITEFRAMES = [
	"res://GraphicAssets/Cars/carSedan/carSedan_spriteframes.tres",
	"res://GraphicAssets/Cars/carTruck/carTruck_spriteframes.tres",
	"res://GraphicAssets/Cars/carOffroad/carOffroad_spriteframes.tres",
	"res://GraphicAssets/Cars/carKart/carKart_spriteframes.tres",
]

onready var GameDataManager = get_tree().get_root().get_node(GameConstants.GDM_NODEPATH)


func update_car_type():
	var car_index = GameConstants.get_car_index(GameDataManager.get_current_car())
	self.frames = load(_CAR_SPRITEFRAMES[car_index])
