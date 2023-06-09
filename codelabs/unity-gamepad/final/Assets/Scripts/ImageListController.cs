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
using UnityEngine.UI;
#if UNITY_EDITOR
using UnityEditor;
#endif

public class ImageListController : MonoBehaviour
{
    public Text demoText;
    public Image currentImage;
    public GameObject imageButtonPrefab;
    public GameObject imageListContent;

#if !UNITY_EDITOR
    private GameAssetManager gameAssetManager;    
#endif
    private Dictionary<string, Sprite> imageDictionary;
    private bool needsRefresh;

    private void Awake()
    {
#if !UNITY_EDITOR
        gameAssetManager = 
            GameObject.Find("GameAssetManager").GetComponent<GameAssetManager>();
        if (gameAssetManager == null) 
        {
            Debug.LogError("Failed to find GameAssetManager");
        }

        gameAssetManager.NewSpriteEvent.AddListener(SetNeedsRefresh);
#endif
        
        imageDictionary = new Dictionary<string, Sprite>();
    }

    private void Start()
    {
        SetNeedsRefresh();
    }

    void Update()
    {
        if (needsRefresh)
        {
            RefreshImageList();
            needsRefresh = false;
        }        
    }

    public void SetNeedsRefresh()
    {
        needsRefresh = true;
    }

    #if UNITY_EDITOR
    private void RefreshImageList()
    {
        Debug.Log("RefreshImageList");
        var assets = AssetDatabase.FindAssets("t:Sprite", 
            new[] {"Assets/Images"});
        foreach (var guid in assets) 
        {
            string assetPath = AssetDatabase.GUIDToAssetPath(guid);
            Sprite loadedSprite = null;
            if (imageDictionary.TryGetValue(assetPath, out loadedSprite) 
                == false)
            {
                loadedSprite = AssetDatabase.LoadAssetAtPath<Sprite>(
                    assetPath);
                if (loadedSprite != null)
                {
                    imageDictionary.Add(assetPath, loadedSprite);
                    AddImageToList(assetPath, loadedSprite);       
                }
            }
        }
    }
    #else
    private void RefreshImageList()
    {
        var allSprites = gameAssetManager.SpriteAssets;
        foreach(var dictionaryEntry in allSprites)
        {
            string assetPath = dictionaryEntry.Key;
            Sprite assetSprite = dictionaryEntry.Value;
            Sprite loadedSprite = null;
            if (imageDictionary.TryGetValue(assetPath, out loadedSprite) 
                == false)
            {
                imageDictionary.Add(assetPath, assetSprite);
                AddImageToList(assetPath, assetSprite);
            }
        }
    }
    #endif

    private void AddImageToList(string assetPath, Sprite newSprite)
    {
        GameObject newButton = Instantiate(imageButtonPrefab) as GameObject;
        ImageButtonController buttonController = 
            newButton.GetComponent<ImageButtonController>();
        buttonController.SetupButton(assetPath, newSprite, currentImage, demoText);
        newButton.transform.SetParent(imageListContent.transform, false);
        newButton.transform.localScale = Vector3.one;
    }
}
