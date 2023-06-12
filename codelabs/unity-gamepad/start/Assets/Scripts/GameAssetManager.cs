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
using System.Linq;
using UnityEngine;
using UnityEngine.Events;

public class GameAssetManager : MonoBehaviour
{
    public List<string> discreteAssetNameList;

    public List<string> installtimeAssetPackNameList;
    public List<string> fastfollowAssetPackNameList;
    public List<string> ondemandAssetPackNameList;
    public string discreteAssetPackName;

    private Dictionary<string, AssetBundle> assetBundles;
    private Dictionary<string, Sprite> spriteAssets;

    private UnityEvent newSpriteEvent;

    public Dictionary<string, AssetBundle> AssetBundles
    {
        get { return assetBundles; }
        private set {}
    }

    public Dictionary<string, Sprite> SpriteAssets
    {
        get { return spriteAssets; }
        private set {}
    }

    public UnityEvent NewSpriteEvent
    {
        get { return newSpriteEvent; }
        private set {}
    }

    private void Awake()
    {
        DontDestroyOnLoad(this.gameObject);
        assetBundles = new Dictionary<string, AssetBundle>();
        spriteAssets = new Dictionary<string, Sprite>();
        newSpriteEvent = new UnityEvent();
    }

    public List<string> GetAssetPackNameList()
    {
        var assetPacks = installtimeAssetPackNameList.Concat(
            fastfollowAssetPackNameList).Concat(
                ondemandAssetPackNameList).ToList();
        return assetPacks;
    }

    public List<string> GetAssetBundleNameList()
    {
        return GetAssetPackNameList();
    }

    public List<string> GetDiscreteAssetNameList()
    {
        return discreteAssetNameList;
    }

    public bool IsSingleAssetBundleAssetPack(string assetPackName)
    {
        if (assetPackName.Equals(discreteAssetPackName))
        {
            return false;
        }
        return true;
    }

    public AssetBundle GetAssetBundleForName(string assetBundleName)
    {
        AssetBundle returnBundle;
        if (assetBundles.TryGetValue(assetBundleName, out returnBundle)
            == false)
        {
            returnBundle = null;
        }
        return returnBundle;
    }

    public void AddAssetBundle(string assetBundleName, AssetBundle assetBundle)
    {
        assetBundles.Add(assetBundleName, assetBundle);
    }

    public void AddSpriteAsset(string spriteAssetPath, Sprite newSprite)
    {
        spriteAssets.Add(spriteAssetPath, newSprite);
        newSpriteEvent.Invoke();
    }

    public static Sprite GenerateSpriteFromRawImage(byte[] data)
    {
        Texture2D tex = new Texture2D(2, 2);
        tex.LoadImage(data);
        Rect spriteRect = new Rect(0.0f, 0.0f, tex.width, tex.height);
        Vector2 spritePivot = new Vector2(0.5f, 0.5f);
        Sprite newSprite = Sprite.Create(tex, spriteRect, spritePivot,
            100.0f, 1, SpriteMeshType.FullRect);
        return newSprite;
    }
}
