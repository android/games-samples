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

signal garage_back_pressed

export(StreamTexture) var unselected_tab_bg
export(StreamTexture) var selected_tab_bg

var _active_tab_index
var _panel_array
var _tab_array

onready var GameDataManager = get_tree().get_root().get_node(GameConstants.GDM_NODEPATH)


func _ready():
	_active_tab_index = 0
	_tab_array = [$GarageTabs/CarTab, $GarageTabs/BackgroundTab]
	_panel_array = [$GaragePanel/CarPanel, $GaragePanel/BackgroundPanel]
	refresh_tabs()


func set_garage_visible(is_visible):
	if is_visible == true:
		$GaragePanel/CarPanel.update_car_panel()
		$GaragePanel/BackgroundPanel.update_background_panel()
	self.visible = is_visible


func refresh_tabs():
	for tabIndex in range(0, _tab_array.size()):
		var tabSelected = true if tabIndex == _active_tab_index else false
		update_selected_tab(_tab_array[tabIndex], tabSelected)
		update_selected_panel(_panel_array[tabIndex], tabSelected)


func update_selected_tab(tab_object, tab_selected):
	var tabBG = selected_tab_bg if tab_selected else unselected_tab_bg
	
	tab_object.texture_disabled = tabBG
	tab_object.texture_focused = tabBG
	tab_object.texture_hover = tabBG
	tab_object.texture_normal = tabBG
	tab_object.texture_pressed = tabBG


func update_selected_panel(panel_object, panel_selected):
	panel_object.visible = panel_selected


func _on_Tab_pressed(tab_index):
	_active_tab_index = tab_index
	refresh_tabs()


func _on_BackButton_pressed():
	emit_signal("garage_back_pressed")
