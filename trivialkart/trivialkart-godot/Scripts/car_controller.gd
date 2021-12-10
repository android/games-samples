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

extends Area2D

signal car_driven
signal car_out_of_gas

# Amount of time to apply acceleration
const _CAR_ACCELERATION_TIME = 2.5
# Amount of time before adding friction
const _CAR_DRIVE_TIME = 5.5
# Minimum velocity after which we stop
const _CAR_BRAKE_VELOCITY = 16.0
# Acceleration to apply during acceleration phase
const _CAR_ACCELERATION = 120.0
# Max clamp speed
const _CAR_MAX_VELOCITY = 300.0
# Friction to apply after we stop 'driving'
const _CAR_FRICTION = 0.975

var current_car_velocity = 0.0
var current_car_drive_time = 0.0

onready var GameDataManager = get_tree().get_root().get_node(GameConstants.GDM_NODEPATH)


func _physics_process(delta):
	if current_car_drive_time != 0.0:
		_update_driving(delta)


func _start_driving(delta):
	current_car_velocity = _CAR_ACCELERATION * delta
	current_car_drive_time = delta
	$CarSprite.playing = true


func _update_driving(delta):
	current_car_drive_time += delta
	if (current_car_drive_time <= _CAR_ACCELERATION_TIME):
		current_car_velocity += _CAR_ACCELERATION * delta
		if current_car_velocity > _CAR_MAX_VELOCITY:
			current_car_velocity = _CAR_MAX_VELOCITY
	if (current_car_drive_time > _CAR_DRIVE_TIME):
		current_car_velocity *= _CAR_FRICTION
		if (current_car_velocity < _CAR_BRAKE_VELOCITY):
			current_car_drive_time = 0.0
			current_car_velocity = 0.0
			$CarSprite.playing = false


func get_current_velocity():
	return current_car_velocity


func _car_input_event(_viewport, event, _shape_idx):
	if event is InputEventMouseButton and event.button_index == BUTTON_LEFT and event.pressed:
		var gas_level = GameDataManager.get_current_gas_level()
		if gas_level == 0:
			emit_signal("car_out_of_gas")
		else:
			GameDataManager.set_current_gas_level(gas_level - 1)
			emit_signal("car_driven")
			if current_car_drive_time == 0.0:
				_start_driving(1.0/60.0)
			else:
				# If we are already driving, just extend the drive time
				current_car_drive_time -= _CAR_DRIVE_TIME
