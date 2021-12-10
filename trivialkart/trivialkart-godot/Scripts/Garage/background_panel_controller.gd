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

const _BACKGROUND_ID_ARRAY = [
	GameConstants.BGTypes.BG_BLUE, 
	GameConstants.BGTypes.BG_MUSHROOM,
]

var _background_panel_array

onready var GameDataManager = get_tree().get_root().get_node(GameConstants.GDM_NODEPATH)


func _ready():
	_background_panel_array = [$BlueBGPanel, $MushroomBGPanel]


func update_background_panel():
	var current_active_bg = GameDataManager.get_current_bg()
	for bg_index in range(0, _background_panel_array.size()):
		if (GameDataManager.is_bg_unlocked(_BACKGROUND_ID_ARRAY[bg_index])):
			_background_panel_array[bg_index].visible = true
			var StatusLabel = _background_panel_array[bg_index].get_node("StatusLabel")
			StatusLabel.visible = (_BACKGROUND_ID_ARRAY[bg_index] == current_active_bg)
		else:
			_background_panel_array[bg_index].visible = false


func _on_bg_button_pressed(bg_index):
	GameDataManager.set_current_bg(_BACKGROUND_ID_ARRAY[bg_index])
	update_background_panel()
