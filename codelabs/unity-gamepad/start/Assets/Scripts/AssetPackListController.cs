// Copyright 2020 Google LLC
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

using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class AssetPackListController : MonoBehaviour
{
    public GameObject assetPackInfoPrefab;
    public GameObject assetPackListContent;

    private GameAssetManager gameAssetManager;

    private void Awake()
    {
        gameAssetManager = 
            GameObject.Find("GameAssetManager").GetComponent<GameAssetManager>();
        if (gameAssetManager == null) 
        {
            Debug.LogError("Failed to find GameAssetManager");
        }
    }

    private void Start()
    {
    }

    void Update()
    {
        
    }

    private void AddAssetPack(string assetPackName)
    {
        GameObject assetPackInfo = Instantiate(assetPackInfoPrefab)
            as GameObject;
        AssetPackInfoController infoController = 
            assetPackInfo.GetComponent<AssetPackInfoController>();
        bool singleAssetBundlePack = 
            gameAssetManager.IsSingleAssetBundleAssetPack(assetPackName);
        infoController.SetAssetPack(assetPackName, singleAssetBundlePack);
        infoController.transform.SetParent(assetPackListContent.transform,
            false);
        infoController.transform.localScale = Vector3.one;
    }
}
