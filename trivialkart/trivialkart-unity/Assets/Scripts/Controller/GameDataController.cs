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
using System.IO;
using System.Linq;
using UnityEngine;

/// <summary>
/// Controller for game data.
/// It controls game data loading, saving and fetching.
/// </summary>
public class GameDataController
{
    private const string FileName = "data.json";
    private static readonly string DataPath = Application.persistentDataPath + "/" + FileName;

    private static GameData _gameData;

    public static void SaveGameData()
    {
        SaveGameDataOffline();
    }

    public static void LoadGameData()
    {
        LoadGameOffline();
    }

    private static void SaveGameDataOffline()
    {
        File.WriteAllText(DataPath, JsonUtility.ToJson(_gameData, true));
    }

    private static void LoadGameOffline()
    {
        Debug.Log("loading data");
        try
        {
            // Load data from data.json file if it exists and it's not empty.
            if (File.Exists(DataPath) && new FileInfo( DataPath ).Length != 0)
            {
                var contents = File.ReadAllText(DataPath);
                _gameData = JsonUtility.FromJson<GameData>(contents);
                Debug.Log(contents);
            }
            else // If data file doesn't exist, create a default one.
            {
                Debug.Log("Unable to read the save data, file does not exist");
                _gameData = new GameData();
            }
        }
        catch (Exception ex)
        {
            Debug.Log(ex.Message);
        }
    }

    public static GameData GetGameData()
    {
        if (_gameData == null)
        {
            Debug.LogError("Game data has not been loaded yet.");
        }
        return _gameData;
    }

    public static void SetGameData(GameData gameData)
    {
        _gameData = gameData;
    }
    
    public static void UnlockInGameContent(string productId)
    {
        // Check if a consumable (coins) has been purchased by this user.
        foreach (var coin in CoinList.List.Where(coin =>
                     string.Equals(productId, coin.ProductId, StringComparison.Ordinal)))
        {
            _gameData.UpdateCoins(coin);
            return;
        }

        // Check if a non-consumable (car) has been purchased by this user.
        foreach (var car in CarList.List.Where(car => string.Equals(productId, car.ProductId,
                     StringComparison.Ordinal)))
        {
            _gameData.PurchaseCar(car);
            return;
        }

        // Check if a subscription has been purchased by this user.
        foreach (var subscription in SubscriptionList.List.Where(subscription => string.Equals(productId,
                     subscription.ProductId,
                     StringComparison.Ordinal)))
        {
            _gameData.UpdateSubscription(subscription);
            return;
        }

        
        // Pop up window
        Debug.LogError("PurchaseController: Product ID doesn't match any existing products.");
    }
}
