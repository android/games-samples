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

signal price_change_accepted()
signal price_change_canceled()
signal purchase_failed(product_id)
signal purchase_successful(product_id)

enum BillingResponse {SUCCESS = 0, CANCELLED = 1}

enum PurchaseManagerStatus {DISABLED, CONNECTING, QUERYING_SKUS,
	QUERYING_PURCHASES, INITIALIZED, PURCHASING}
	
# Matches Purchase.PurchaseState in the Play Billing Library
enum PurchaseState {UNSPECIFIED, PURCHASED, PENDING}
	
enum SubscriptionProrationMode {
	# Replacement takes effect immediately, and the remaining time 
	# will be prorated and credited to the user.
	IMMEDIATE_WITH_TIME_PRORATION = 1,
	# Replacement takes effect immediately, and the billing cycle remains the same. 
	# The price for the remaining period will be charged. 
	# This option is only available for subscription upgrade.
	IMMEDIATE_AND_CHARGE_PRORATED_PRICE,
	# Replacement takes effect immediately, and the new price will be charged on
	# next recurrence time. The billing cycle stays the same.
	IMMEDIATE_WITHOUT_PRORATION,
	# Replacement takes effect when the old plan expires, and the new price
	# will be charged at the same time.
	DEFERRED,
	# Replacement takes effect immediately, and the user is charged full price
	# of new plan and is given a full billing cycle of subscription,
	# plus remaining prorated time from the old plan.
	IMMEDIATE_AND_CHARGE_FULL_PRICE,
}

const _PURCHASE_STATE_STRINGS = ["UNSPECIFIED", "PURCHASED", "PENDING"]

# Test values for obfuscated account id and profile id
# These are user defined data in string format with a max of 64 characters
# PII data is prohibited, it is recommended to use encryption or a one-way
# hash to generate the obfuscated values.
const _TEST_ACCOUNT_ID = "7bn3a932npgg;sld93[f.g]39mskdfj"
const _TEST_PROFILE_ID = "93mfj30sm0gn3ksmvl3,g];fj3mxpwf"

const _PRODUCT_ID_KEY = "product_id"
const _PURCHASE_TOKEN_KEY = "purchase_token"
const _ERROR_WAIT_TEXT = "Purchase Init Error"
const _PURCHASE_WAIT_TEXT = "Purchasing"
const _STARTUP_WAIT_TEXT = "Please Wait"
const _WAIT_MAX_PERIOD = 3.0

# Test var, if true automatically consume purchased inapp items, resets
# purchase status so they can be purchased again, even if they normally
# aren't 'consumable'
const _AUTO_CONSUME = false

var _active_subscription_purchase
var _billing_api
var _billing_status
var _display_purchase_notifications
var _last_product_id
var _processing_purchases = []
var _purchase_messages = []
var _purchase_request_count
var _sku_request_count
var _subscription_proration_mode
var _use_google_play_billing
var _wait_panel_elapsed
var _wait_panel_text

onready var GameDataManager = get_tree().get_root().get_node(GameConstants.GDM_NODEPATH)
onready var PurchaseLabel = $PurchaseNotificationPanel/MessageLabel
onready var WaitLabel = $WaitPanel/WaitFramePanel/WaitLabel


func _ready():
	_active_subscription_purchase = null
	_billing_status = PurchaseManagerStatus.DISABLED
	_display_purchase_notifications = true
	_purchase_request_count = 0
	_sku_request_count = 0
	_subscription_proration_mode = SubscriptionProrationMode.IMMEDIATE_WITH_TIME_PRORATION
	_wait_panel_elapsed = 0.0
	_wait_panel_text = _STARTUP_WAIT_TEXT
	WaitLabel.text = _STARTUP_WAIT_TEXT
	_use_google_play_billing = Engine.has_singleton("GodotGooglePlayBilling")
	if _use_google_play_billing == false:
		print("Google Play Bililng plugin not found")
		$WaitPanel.visible = false
	else:
		print("Initializing Google Play Billing plugin")
		_billing_api = Engine.get_singleton("GodotGooglePlayBilling")
		_setup_signals()
		_billing_status = PurchaseManagerStatus.CONNECTING
		_billing_api.startConnection()


func _process(delta):
	_update_purchase_notifications()
	_update_wait_panel(delta)


func _on_billing_resume():
	# When the billing plugin signals a resume event, we need to run a purchase
	# query to detect any purchases that might have occured while we
	# were paused
	if _billing_status == PurchaseManagerStatus.INITIALIZED:
		_query_purchases()


func _on_connected():
	print("Entering _on_connected")
	# Set our test data for obfuscated account and profile ids
	_billing_api.setObfuscatedAccountId(_TEST_ACCOUNT_ID)
	_billing_api.setObfuscatedProfileId(_TEST_PROFILE_ID)

	# After connecting to the billing API, start queries on our
	# product ids, we use the results to obtain the current prices
	# of each product
	var item_product_ids = GameDataManager.get_item_product_ids()
	var sub_product_ids = GameDataManager.get_subscription_product_ids()
	_sku_request_count += item_product_ids.size()
	_sku_request_count += sub_product_ids.size()
	_billing_status = PurchaseManagerStatus.QUERYING_SKUS
	_billing_api.querySkuDetails(item_product_ids, "inapp")
	_billing_api.querySkuDetails(sub_product_ids, "subs")


func _on_connect_error(response_id, error_message):
	print("Entering _on_connect_error id: ", response_id, " message: ", error_message)
	_billing_status = PurchaseManagerStatus.DISABLED


func _on_disconnected():
	print("Entering _on_disconnected")
	_billing_status = PurchaseManagerStatus.DISABLED


func _on_price_acknowledged(response_id):
	if response_id == BillingResponse.SUCCESS:
		emit_signal("price_change_accepted")
	elif response_id == BillingResponse.CANCELED:
		emit_signal("price_change_canceled")


func _on_purchase_acknowledged(purchase_token):
	print("Entering _on_purchase_acknowledged")
	_handle_purchase_token(purchase_token, true)


func _on_purchase_acknowledgement_error(response_id, error_message, purchase_token):
	print("Entering _on_purchase_acknowledgement_error id: ", response_id, \
		" message: ", error_message)
	_handle_purchase_token(purchase_token, false)


func _on_purchase_consumed(purchase_token):
	print("Entering _on_purchase_consumed")
	_handle_purchase_token(purchase_token, true)


func _on_purchase_consumption_error(response_id, error_message, purchase_token):
	print("Entering _on_purchase_consumption_error id:", response_id, \
		" message: ", error_message)
	_handle_purchase_token(purchase_token, false)


func _on_purchase_error(response_id, error_message):
	print("Entering _on_purchase_error id:", response_id, " message: ", error_message)
	_reset_purchase_state()
	emit_signal("purchase_failed", _last_product_id)


func _on_purchases_updated(purchases):
	print("Entering _on_purchases_updated")
	for purchase in purchases:
		_process_purchase(purchase)


func _on_query_purchases_response(query_result):
	print("Entering _on_query_purchases_response")	
	if query_result.status == OK:
		for purchase in query_result.purchases:
			print(purchase)
			_process_purchase(purchase)
	else:
		print("queryPurchases failed, response code: ", \
				query_result.response_code, \
				" debug message: ", query_result.debug_message)
	_purchase_request_count -= 1
	if _purchase_request_count == 0:
		_billing_status = PurchaseManagerStatus.INITIALIZED


func _on_sku_details_query_completed(sku_dictionary_array):
	print("Entering sku_details_query_completed")
	for sku_dictionary in sku_dictionary_array:
		var product_id = sku_dictionary.sku
		var product_price = sku_dictionary.price
		# Update our default price to the current store price, so we
		# can display the current price in the UI
		print("Price for ", product_id, " is: ", product_price)
		GameDataManager.set_product_price(product_id, product_price)
		_sku_request_count -= 1
		
	# Once we have details on all the SKUs,
	# start a query of purchase status for our products
	if _sku_request_count == 0:
		print("Completed price updates")
		_query_purchases()


func _on_sku_details_query_error(response_id, error_message, skus_queried):
	print("Entering _on_sku_details_query_error id:", response_id, " message: ", \
		error_message, " skus: ", skus_queried)
	_billing_status = PurchaseManagerStatus.DISABLED


func _reset_purchase_state():
	if _billing_status == PurchaseManagerStatus.PURCHASING:
		_billing_status = PurchaseManagerStatus.INITIALIZED


func _is_product_purchaseable(product_id):
	# Check if we are already purchasing this product
	for purchase_index in range(0, _processing_purchases.size()):
		var purchase = _processing_purchases[purchase_index]
		if purchase.product_id == product_id:
			return false
	# Check if this product is in a pending purchase state
	if GameDataManager.is_product_pending_purchase(product_id):
		return false
	return true


func _use_subscription_update(product_id):
	# If we are trying to purchase a different subscription
	# than the one we are currently subscribed to, use
	# the update subscription flow
	if GameDataManager.is_product_subscription(product_id):
		if _active_subscription_purchase != null:
			if _active_subscription_purchase.sku != product_id:
				return true
	return false


func _handle_purchase_token(purchase_token, purchase_successful):
	# Find the product id associated with this purchase token
	# and award the product if it was a successful purchase
	for purchase_index in range(0, _processing_purchases.size()):
		var purchase = _processing_purchases[purchase_index]
		var product_id = purchase.product_id
		if purchase_token == purchase.purchase_token:
			if purchase_successful == true:
				print("awarding product: ", product_id)
				GameDataManager.award_product(product_id)
				emit_signal("purchase_successful", product_id)
			else:
				emit_signal("purchase_failed", product_id)
			_processing_purchases.remove(purchase_index)
			_reset_purchase_state()
			# If display purchase notifications are enabled, queue up
			# the product id to display
			if _display_purchase_notifications == true:
				_purchase_messages.push_back(product_id)
			return
	print("No match for purchase token")


func _query_purchases():
	_billing_status = PurchaseManagerStatus.QUERYING_PURCHASES
	_purchase_request_count = 2
	_billing_api.queryPurchases("inapp")
	_billing_api.queryPurchases("subs")


func _process_purchase(purchase):
	var product_id = purchase.sku
	var purchase_state = purchase.purchase_state
	var purchase_token = purchase.purchase_token
	var is_acknowledged = purchase.is_acknowledged
	var is_consumable = GameDataManager.is_product_consumable(product_id)
	var state_string = "unknown"
	
	# Auto consume test option
	if _AUTO_CONSUME == true and \
			GameDataManager.is_product_subscription(product_id) == false:
		is_consumable = true
		
	if purchase_state >= PurchaseState.UNSPECIFIED and \
			purchase_state <= PurchaseState.PENDING:
		state_string = _PURCHASE_STATE_STRINGS[purchase_state]

	print("process_purchase: ", product_id, \
			" consumable: ", is_consumable, \
			" acknowledged: ", is_acknowledged, \
			" state: ", state_string)

	if purchase_state == PurchaseState.PENDING:
		# If an item's purchase state is pending, flag it so we can display
		# the appropriate message in the store UI.
		GameDataManager.set_purchase_pending(product_id, true)
		return
	elif purchase_state == PurchaseState.PURCHASED:
		# If the purchase is complete, make sure we clear any pending status
		GameDataManager.set_purchase_pending(product_id, false)
		# If we have an active subscription purchase, stash the purchase
		# record because we will need it to update the subscription if the
		# user changes to a different subscription
		if GameDataManager.is_product_subscription(product_id):
			_active_subscription_purchase = purchase
	
	# Create a record of this purchase before consuming or acknowledging it,
	# since those callbacks only provide the purchase token and we will need
	# to know which product id they were associated with to make the
	# appropriate award.
	var purchase_summary = {}
	purchase_summary[_PRODUCT_ID_KEY] = product_id
	purchase_summary[_PURCHASE_TOKEN_KEY] = purchase_token
	
	if is_consumable:
		print("Consuming purchase")
		_processing_purchases.append(purchase_summary)
		_billing_api.consumePurchase(purchase_token)
	else:
		# If the purchase hasn't been acknowledged, we need to do so.
		if !is_acknowledged:
			print("Acknowledging purchase")
			_processing_purchases.append(purchase_summary)
			_billing_api.acknowledgePurchase(purchase_token)
		else:	
			# If it already has, we can just award the product
			# Make sure one-time purchases that unlock something are registered
			print("Purchase already acknowledged, awarding")
			GameDataManager.award_product(product_id)


func _setup_signals():
		_billing_api.connect("billing_resume", self, "_on_billing_resume")
		_billing_api.connect("connected", self, "_on_connected")
		_billing_api.connect("connect_error", self, "_on_connect_error")
		_billing_api.connect("disconnected", self, "_on_disconnected")
		_billing_api.connect("price_change_acknowledged", self, "_on_price_acknowledged")
		_billing_api.connect("purchase_acknowledged", self, "_on_purchase_acknowledged")
		_billing_api.connect("purchase_acknowledgement_error", 
			self, "_on_purchase_acknowledgement_error")
		_billing_api.connect("purchase_consumed", self, "_on_purchase_consumed")
		_billing_api.connect("purchase_consumption_error",
			self, "_on_purchase_consumption_error")
		_billing_api.connect("purchase_error", self, "_on_purchase_error")
		_billing_api.connect("purchases_updated", self, "_on_purchases_updated")
		_billing_api.connect("query_purchases_response", self, "_on_query_purchases_response")
		_billing_api.connect("sku_details_query_completed",
			self, "_on_sku_details_query_completed")
		_billing_api.connect("sku_details_query_error",
			self, "_on_sku_details_query_error")


func _on_OK_button_pressed():
	if $PurchaseNotificationPanel.visible == true:
		$PurchaseNotificationPanel.visible = false


func _update_purchase_notifications():
	if _display_purchase_notifications == true and \
			$PurchaseNotificationPanel.visible == false and \
			_purchase_messages.size() > 0:
		var purchase_message = _purchase_messages.pop_back()
		PurchaseLabel.text = purchase_message
		$PurchaseNotificationPanel.visible = true


func _update_wait_panel(delta):
	if $WaitPanel.visible == true:
		if _billing_status == PurchaseManagerStatus.INITIALIZED:
			$WaitPanel.visible = false
		elif _billing_status == PurchaseManagerStatus.DISABLED:
			WaitLabel.text = _ERROR_WAIT_TEXT
		else:
			# Append 0-_WAIT_MAX_PERIOD looping numbers of periods to the
			# waiting text to show we are waiting for something to complete
			_wait_panel_elapsed += delta
			var elapsed = int(_wait_panel_elapsed)
			var waitmax = int(_WAIT_MAX_PERIOD)
			var wait_period_count = elapsed % waitmax
			if (_wait_panel_elapsed > _WAIT_MAX_PERIOD):
				_wait_panel_elapsed = float(wait_period_count)
			var new_wait_text = _wait_panel_text
			for _period_num in range(0, wait_period_count):
				new_wait_text += "."
			WaitLabel.text = new_wait_text


func begin_purchase(product_id):
	if _use_google_play_billing == false:
		# If the billing plugin isn't active, just award the product
		# and say we succeeded
		print("awarding product: ", product_id)
		GameDataManager.award_product(product_id)
		emit_signal("purchase_successful", product_id)
	else:
		if _billing_status == PurchaseManagerStatus.INITIALIZED \
				and _is_product_purchaseable(product_id):
			_last_product_id = product_id
			if _use_subscription_update(product_id):
				print("updateSubscription: ", _active_subscription_purchase.sku, \
						", ", product_id, ", ", _subscription_proration_mode)
				_billing_api.updateSubscription(_active_subscription_purchase.purchase_token, \
						product_id, _subscription_proration_mode)
			else:
				_billing_api.purchase(product_id)
			_billing_status = PurchaseManagerStatus.PURCHASING
			$WaitPanel.visible = true
		else:
			emit_signal("purchase_failed", product_id)


func confirm_price_change(product_id):
	if _use_google_play_billing == false:
		emit_signal("price_change_accepted")
	else:	
		if _billing_status == PurchaseManagerStatus.INITIALIZED:
			_billing_api.confirmPriceChange(product_id)


func set_purchase_notifications_enabled(display_notifications):
	_display_purchase_notifications = display_notifications
