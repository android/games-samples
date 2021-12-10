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

signal coins_changed
signal current_bg_changed
signal current_car_changed
signal gas_filled
signal gas_used

const _GAME_DATA_FILENAME = "user://gamedata.save"

var _current_active_bg
var _current_active_car
var _current_active_subscription
var _current_coins
var _current_gas_level

var _car_unlocked_array
var _car_price_array
var _coin_price_array
var _subscription_price_array

# Dictionary of product id's that are set to pending purchase
var _pending_purchases = {}

# Send the appropriate signals to refresh game components on the next update
# used to set up active cars/backgrounds after loading the game data
var _pending_data_update
# Signals that the game data has changed and should be saved
var _save_needed

func set_purchase_pending(product_id, is_pending):
	if is_pending:
		if _pending_purchases.has(product_id) == false:
			_pending_purchases[product_id] = true
	else:
		if _pending_purchases.has(product_id) == true:
			_pending_purchases.erase(product_id)
	
func is_product_pending_purchase(product_id):
	return _pending_purchases.has(product_id)

func _ready():
	_pending_data_update = false
	_save_needed = false
	_set_default_game_data()
	_load_game_data()


func _set_default_game_data():
	_current_coins = 20
	_current_gas_level = GameConstants.FULL_GAS_LEVEL
	_current_active_bg = GameConstants.BGTypes.BG_BLUE
	_current_active_car = GameConstants.CarTypes.CAR_SEDAN
	_current_active_subscription = GameConstants.SubscriptionType.SUBSCRIPTION_NONE
	_car_unlocked_array = [true, false, false, false]
	# defaults, will be updated with current price data from store
	_car_price_array = ["unknown", "20 coins", "$2.98", "$4.98"]
	_coin_price_array = ["$0.98", "$1.98", "$2.48", "$4.98"]
	_subscription_price_array = ["unknown", "$1.98", "$4.98"]


func calculate_coin_discount(coin_price):
	if _current_active_subscription == GameConstants.SubscriptionType.SUBSCRIPTION_GOLD:
		var discount_price = int((float(coin_price) - (float(coin_price) 
			* GameConstants.GOLDEN_SUB_COIN_DISCOUNT)))
		return discount_price
	return coin_price


func get_current_subscription():
	return _current_active_subscription


func set_current_subscription(newSubscription):
	_current_active_subscription = newSubscription
	_save_needed = true


func get_current_bg():
	return _current_active_bg


func set_current_bg(currentBG):
	_current_active_bg = currentBG
	_save_needed = true
	emit_signal("current_bg_changed")


func get_current_car():
	return _current_active_car


func set_current_car(currentCar):
	_current_active_car = currentCar
	_save_needed = true
	emit_signal("current_car_changed")


func get_current_coins():
	return _current_coins


func set_current_coins(newCoins):
	_current_coins = newCoins
	_save_needed = true
	emit_signal("coins_changed")


func get_current_gas_level():
	return _current_gas_level


func set_current_gas_level(new_gas_level):
	_current_gas_level = new_gas_level
	if (new_gas_level == GameConstants.FULL_GAS_LEVEL):
		emit_signal("gas_filled")
	else:
		emit_signal("gas_used")
	_save_needed = true


func is_car_unlocked(car_type):
	var car_index = GameConstants.get_car_index(car_type)
	if (car_index >= 0):
		return _car_unlocked_array[car_index]
	return false


func unlock_car(car_type):
	var car_index = GameConstants.get_car_index(car_type)
	if (car_index >= 0):
		_car_unlocked_array[car_index] = true


func is_bg_unlocked(bg_type):
	if (bg_type == GameConstants.BGTypes.BG_BLUE):
		return true
	if (_current_active_subscription != GameConstants.SubscriptionType.SUBSCRIPTION_NONE):
		return true
	return false


func get_car_store_purchase(car_type):
	var car_index = GameConstants.get_car_index(car_type)
	if (car_index >= 0):
		return GameConstants.CAR_IS_STORE_PURCHASE[car_index]
	return false


func get_car_price(car_type):
	var car_index = GameConstants.get_car_index(car_type)
	if (car_index >= 0):
		return _car_price_array[car_index]
	return "$0.00"


func get_car_price_key(car_type):
	var car_index = GameConstants.get_car_index(car_type)
	if (car_index >= 0):
		return GameConstants.CAR_PRICE_LABEL_KEYS[car_index]
	return "$notfound"


func get_coin_price(coin_type):
	var coin_index = GameConstants.get_coin_index(coin_type)
	if (coin_index >= 0):
		return _coin_price_array[coin_index]
	return "$0.00"


func get_coin_price_key(coin_type):
	var coin_index = GameConstants.get_coin_index(coin_type)
	if (coin_index >= 0):
		return GameConstants.COIN_PRICE_LABEL_KEYS[coin_index]
	return "$notfound"


func get_subscription_price(subscription_type):
	var subscription_index = GameConstants.get_subscription_index(subscription_type)
	if (subscription_index >= 0):
		return _subscription_price_array[subscription_index]
	return "$0.00"


func get_subscription_price_key(subscription_type):
	var subscription_index = GameConstants.get_coin_index(subscription_type)
	if (subscription_index >= 0):
		return GameConstants.SUBSCRIPTION_PRICE_LABEL_KEYS[subscription_index]
	return "$notfound"


func set_product_price(product_id, product_price):
	for coin_index in range(0, GameConstants.COIN_PRODUCT_IDS.size()):
		if (product_id == GameConstants.COIN_PRODUCT_IDS[coin_index]):
			_coin_price_array[coin_index] = product_price
			return

	for car_index in range(0, GameConstants.CAR_PRODUCT_IDS.size()):
		if (product_id == GameConstants.CAR_PRODUCT_IDS[car_index]):
			_car_price_array[car_index] = product_price
			return
			
	for subscription_index in range(0, GameConstants.SUBSCRIPTION_PRODUCT_IDS.size()):
		if (product_id == GameConstants.SUBSCRIPTION_PRODUCT_IDS[subscription_index]):
			_subscription_price_array[subscription_index] = product_price
			return


func get_item_product_ids():
	var id_array = [ ]
	
	for coin_id in GameConstants.COIN_PRODUCT_IDS:
		id_array.append(coin_id)
		
	for car_id in GameConstants.CAR_PRODUCT_IDS:
		if car_id != "none":
			id_array.append(car_id)
			
	return id_array


func get_subscription_product_ids():
	var id_array = [ ]
	
	for sub_id in GameConstants.SUBSCRIPTION_PRODUCT_IDS:
		if sub_id != "none":
			id_array.append(sub_id)
			
	return id_array

func is_product_subscription(product_id):
	for subscription_index in range(0, GameConstants.SUBSCRIPTION_PRODUCT_IDS.size()):
		if (product_id == GameConstants.SUBSCRIPTION_PRODUCT_IDS[subscription_index]):
			return true
	return false

func is_product_consumable(product_id):
	for coin_index in range(0, GameConstants.COIN_PRODUCT_IDS.size()):
		if (product_id == GameConstants.COIN_PRODUCT_IDS[coin_index]):
			return true
	return false


func award_product(product_id):
	for coin_index in range(0, GameConstants.COIN_PRODUCT_IDS.size()):
		if (product_id == GameConstants.COIN_PRODUCT_IDS[coin_index]):
			var coins_to_award = GameConstants.COIN_AWARDS[coin_index]
			set_current_coins(get_current_coins() + coins_to_award)
			return

	for car_index in range(0, GameConstants.CAR_PRODUCT_IDS.size()):
		if (product_id == GameConstants.CAR_PRODUCT_IDS[car_index]):
			_car_unlocked_array[car_index] = true
			_save_needed = true
			return
			
	for subscription_index in range(0, GameConstants.SUBSCRIPTION_PRODUCT_IDS.size()):
		if (product_id == GameConstants.SUBSCRIPTION_PRODUCT_IDS[subscription_index]):
			var sub_type = GameConstants.get_subscription_type(subscription_index)
			set_current_subscription(sub_type)
			return


func _load_game_data():
	print("Loading game data")
	var game_data_file = File.new()
	if game_data_file.file_exists(_GAME_DATA_FILENAME):
		game_data_file.open(_GAME_DATA_FILENAME, File.READ)
		var game_data_dictionary = parse_json(game_data_file.get_line())
		if game_data_dictionary != null:
			for current_key in game_data_dictionary.keys():
				match current_key:
					"car_unlocked_array":
						_car_unlocked_array = game_data_dictionary[current_key]
					"current_active_bg":
						_current_active_bg = game_data_dictionary[current_key]
					"current_active_car":
						_current_active_car = game_data_dictionary[current_key]
					"current_active_subscription":
						_current_active_subscription = game_data_dictionary[current_key]
					"current_coins":
						_current_coins = game_data_dictionary[current_key]
					"current_gas_level":
						_current_gas_level = game_data_dictionary[current_key]
		_pending_data_update = true
		game_data_file.close()


func _save_game_data():
	print("Saving game data")
	var game_data_dictionary = {
		"car_unlocked_array" : _car_unlocked_array,
		"current_active_bg" : _current_active_bg,
		"current_active_car" : _current_active_car,
		"current_active_subscription" : _current_active_subscription,
		"current_coins" : _current_coins,
		"current_gas_level" : _current_gas_level
	}

	var game_data_file = File.new()
	game_data_file.open(_GAME_DATA_FILENAME, File.WRITE)
	game_data_file.store_line(to_json(game_data_dictionary))
	game_data_file.close()
	_save_needed = false


func _process(_delta):
	if _pending_data_update:
		# Call the set functions to fire the assorted signals so
		# the relevant game items get updated
		set_current_bg(_current_active_bg)
		set_current_car(_current_active_car)
		set_current_coins(_current_coins)
		set_current_gas_level(_current_gas_level)
		# Don't save in this case because we tripped this flag from setting
		# the data values we just loaded from the game data
		_save_needed = false
	
	if _save_needed:
		_save_game_data()
