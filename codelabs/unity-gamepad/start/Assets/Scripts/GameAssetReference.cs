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

public class GameAssetReference
{
    private string assetName;
    private string assetLocation;
    private AssetBundle assetBundle;
    private long assetSize;
    private long assetOffset;

    public string AssetName 
    {
        get { return assetName; }
        private set {}
    }

    public string AssetLocation 
    {
        get { return assetLocation; }
        private set {}
    }
    
    public AssetBundle AssetBundle 
    {
        get { return assetBundle; }
        private set {}
    }

    public long AssetSize 
    {
        get { return assetSize; }
        private set {}
    }

    public long AssetOffset
    {
        get { return assetOffset; }
        private set {}
    }

    public GameAssetReference(string aName, string aLocation, 
        AssetBundle aBundle, long aSize, long aOffset)
    {
        assetName = aName;
        assetLocation = aLocation;
        assetBundle = aBundle;
        assetSize = aSize;
        assetOffset = aOffset;
    }
}
