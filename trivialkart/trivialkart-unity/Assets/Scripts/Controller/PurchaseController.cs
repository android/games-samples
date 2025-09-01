// Copyright 2022 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

using System;
using System.Collections.Generic;
using System.Linq;
using UnityEngine;

// Two versions of the PurchaseController class exist in this file, depending on whether USE_IAP is defined
// The non-USE_IAP version is stub that sets placeholder prices and simply approves purchases.

#if !USE_IAP
public class PurchaseController : MonoBehaviour
{
    private void Start()
    {
        // Placeholder prices
        const float currentPrice = 0.01f;
        const string priceString = "$0.01";
        var uiPriceChangeController = FindObjectOfType<UIPriceChangeController>();
        if (uiPriceChangeController != null)
        {
            foreach (var coin in CoinList.List)
            {
                coin.Price = currentPrice;
                uiPriceChangeController.UpdatePriceTextItem(coin.ProductId, priceString);
            }

            foreach (var car in CarList.List)
            {
                if (car.IsRealMoneyPurchase)
                {
                    car.Price = currentPrice;
                    uiPriceChangeController.UpdatePriceTextItem(car.ProductId, priceString);
                }
            }

            foreach (var subscription in SubscriptionList.List)
            {
                subscription.Price = currentPrice;
                uiPriceChangeController.UpdatePriceTextItem(subscription.ProductId, priceString);
            }
        }
    }

    public static void BuyProductId(string productId)
    {
        // Unlock the appropriate content.
        GameDataController.UnlockInGameContent(productId);
    }

    public static void PurchaseASubscription(SubscriptionList.Subscription oldSubscription,
        SubscriptionList.Subscription newSubscription)
    {
        BuyProductId(newSubscription.ProductId);
    }

    public static void RestorePurchase()
    {

    }
}
#else

using UnityEngine.Purchasing;
using UnityEngine.Purchasing.Security;

/// <summary>
/// Controller for the dollar item purchase flow in the game.
/// It uses Unity IAP and the Google Play Billing Library plugin
/// to do purchasing, order verification, and order restore.
/// </summary>
public class PurchaseController : MonoBehaviour
{
    private static StoreController _storeController;
    private static GameManager _gameManager;
    private static UIPriceChangeController _uiPriceChangeController;

    private static bool IsInitialized => _storeController != null;

    private void Start()
    {
        _gameManager = FindFirstObjectByType<GameManager>();
        _uiPriceChangeController = FindFirstObjectByType<UIPriceChangeController>();

        if (!IsInitialized)
        {
            InitializeIAP();
        }
    }

    private async void InitializeIAP()
    {
        _storeController = UnityIAPServices.StoreController();

        _storeController.OnPurchasePending += OnPurchasePending;
        _storeController.OnPurchaseConfirmed += OnPurchaseConfirmed;
        _storeController.OnPurchaseFailed += OnPurchaseFailed;

        _storeController.OnStoreDisconnected += OnStoreDisconnected;
        Debug.Log("Connecting to store.");
        await _storeController.Connect();
        
        // Purchasing has succeeded initializing. Collect our Purchasing references.
        Debug.Log("PurchaseController: OnInitialized success");

        _storeController.OnProductsFetchFailed += OnProductsFetchedFailed;
        _storeController.OnProductsFetched += OnProductsFetched;
        FetchProducts();
        UpdateItemPrices();
    }

    private void OnProductsFetched(List<Product> obj)
    {
        Debug.Log("Products fetched");
    }

    private static void OnProductsFetchedFailed(ProductFetchFailed obj)
    {
        foreach (var e in obj.FailedFetchProducts)
        {
            Debug.Log("Fetch failed for product " + e.id + " ");
        }

        Debug.Log("Fetch failed reason " + obj.FailureReason);
    }

    private static void OnStoreDisconnected(StoreConnectionFailureDescription description)
    {
        Debug.Log($"Store disconnected details: {description.message}");
    }

    private static void OnPurchaseFailed(FailedOrder obj)
    {
        Debug.Log("Purchase Failed details" + obj.Details + " " + obj.FailureReason);
    }

    private static void OnPurchaseConfirmed(Order obj)
    {
        Debug.Log("Purchase confirmed details" + obj.Info);
    }

    private static void OnPurchasePending(PendingOrder obj)
    {
        Debug.Log("Purchase pending " + obj.Info);
    }

    private static void FetchProducts()
    {
        var catalog = new CatalogProvider();

        foreach (var coin in CoinList.List)
        {
            catalog.AddProduct(coin.ProductId, ProductType.Consumable);
        }

        foreach (var car in CarList.List.Where(car => car.IsRealMoneyPurchase && car.Price > 0))
        {
            catalog.AddProduct(car.ProductId, ProductType.NonConsumable);
        }

        foreach (var subscription in SubscriptionList.List.Where(subscription =>
                     subscription.Type != SubscriptionType.NoSubscription))
        {
            catalog.AddProduct(subscription.ProductId, ProductType.Subscription);
        }

        catalog.FetchProducts(UnityIAPServices.DefaultProduct().FetchProductsWithNoRetries);
    }

    private static void UpdateItemPrices()
    {
        Debug.Log("PurchaseController: UpdateItemPrices");

        // This is a very basic loop through all the items, updating the
        // default price information with the current localized price
        // supplied by the store.
        foreach (var product in _storeController.GetProducts())
        {
            float currentPrice = Convert.ToSingle(product.metadata.localizedPrice);
            string productId = product.definition.storeSpecificId;
            string localizedPriceString = product.metadata.localizedPriceString;
            bool foundItem = false;

            foreach (var coin in CoinList.List.Where(coin =>
                         string.Equals(productId, coin.ProductId, StringComparison.Ordinal)))
            {
                Debug.Log($"Updated price for Product: '{productId}', New Price: {currentPrice}");
                coin.Price = currentPrice;
                foundItem = true;
                break;
            }

            if (!foundItem)
            {
                foreach (var car in CarList.List.Where(car => string.Equals(productId, car.ProductId,
                             StringComparison.Ordinal)))
                {
                    Debug.Log($"Updated price for Product: '{productId}', New Price: {currentPrice}");
                    car.Price = currentPrice;
                    foundItem = true;
                    break;
                }
            }

            if (!foundItem)
            {
                foreach (var subscription in SubscriptionList.List.Where(subscription => string.Equals(productId,
                             subscription.ProductId,
                             StringComparison.Ordinal)))
                {
                    Debug.Log($"Updated price for Product: '{productId}', New Price: {currentPrice}");
                    subscription.Price = currentPrice;
                    foundItem = true;
                    break;
                }
            }

            // If we found the item, update the UI text element that actually displays
            // the price information for the current item
            if (foundItem)
            {
                _uiPriceChangeController.UpdatePriceTextItem(productId, localizedPriceString);
            }
        }
    }

    // Buying products and upgrading/downgrading subscriptions are merged
    public static void BuyProductId(string productId)
    {
        // If Purchasing has been initialized ...
        if (IsInitialized)
        {
            // ... look up the Product reference with the general product identifier and the Purchasing
            // system's products collection.
            var product = _storeController.GetProductById(productId);

            // If the look up found a product for this device's store and that product is ready to be sold ...
            if (product is { availableToPurchase: true })
            {
                // Bring up the 'Purchasing' modal message
                _gameManager.SetWaitMessageActive(true);

                Debug.Log($"PurchaseController: Purchasing product asynchronously: '{product.definition.id}'");
                // ... buy the product. Expect a response either through ProcessPurchase or OnPurchaseFailed
                // asynchronously.
                _storeController.PurchaseProduct(product);
            }
            // Otherwise ...
            else
            {
                // ... report the product look-up failure situation
                Debug.Log(
                    "PurchaseController: BuyProductID: FAIL. Not purchasing product, either is not found or is not available for purchase");
            }
        }
        // Otherwise ...
        else
        {
            // ... report the fact Purchasing has not succeeded initializing yet. Consider waiting longer or
            // retrying initialization.
            Debug.Log("PurchaseController: BuyProductID FAIL. Not initialized.");
        }
    }

    public static void RestorePurchase()
    {
        _storeController.RestoreTransactions((restoreSuccess, transactionId) =>
        {
            var garageController = FindFirstObjectByType<GarageController>();
            if (restoreSuccess)
            {
                Debug.Log("PurchaseController: Successfully restored purchase! ");
                garageController.OnRestorePurchaseSuccess();
            }
            else
            {
                Debug.Log("PurchaseController: Failed to restore purchase" + transactionId);
                garageController.OnRestorePurchaseFail();
            }
        });
    }
}
#endif // !NO_IAP
