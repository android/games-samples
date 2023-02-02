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

extends Node2D

const _ENVIRONMENT_TILESETS = [
	"res://Tilesets/BlueGrassTileset.tres",
	"res://Tilesets/ColoredShroomTileset.tres",
]

onready var GameDataManager = get_tree().get_root().get_node(GameConstants.GDM_NODEPATH)


func update_bg_type():
	var bg_index = GameConstants.get_bg_index(GameDataManager.get_current_bg())
	$BackgroundTileMap.tile_set = load(_ENVIRONMENT_TILESETS[bg_index])
