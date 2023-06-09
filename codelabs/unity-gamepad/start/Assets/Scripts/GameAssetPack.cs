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

public class GameAssetPack : MonoBehaviour
{
    private string assetPackName;
    private long assetPackSize;

    // Does the asset pack only contain a single, identically
    // named asset bundle? (created with the plugin interface)
    private bool isSingleAssetBundlePack;

    // Have we retrieved the asset pack size
    private bool isAssetPackSizeValid;

    // Are we requesting cellular data download
    private bool isCellularConfirmationActive;

    // Was the cellular confirmation affirmative
    private bool didApproveCellularData;

    public string AssetPackName
    {
        get { return assetPackName; }
        set
        {
            assetPackName = value;
        }
    }

    public long AssetPackSize
    {
        get { return assetPackSize; }
        private set {}
    }

    public bool IsSingleAssetBundlePack
    {
        get { return isSingleAssetBundlePack; }
        set { isSingleAssetBundlePack = value; }
    }

    public bool IsAssetPackSizeValid
    {
        get { return isAssetPackSizeValid; }
        private set {}
    }

    public bool IsCellularConfirmationActive
    {
        get { return isCellularConfirmationActive; }
        private set {}
    }

    public bool DidApproveCellularData
    {
        get { return didApproveCellularData; }
        private set {}
    }

    public bool IsDownloaded()
    {
        return true;
    }

    public void StartDownload()
    {
    }

    public bool IsDownloadActive()
    {
        return false;
    }

    private void Awake()
    {
        assetPackSize = 0;
        isAssetPackSizeValid = false;
        isCellularConfirmationActive = false;
        didApproveCellularData = false;
    }

    public void InitializeAssetPack(string newAssetPackName,
        bool isSingleBundlePack)
    {
        AssetPackName = newAssetPackName;
        IsSingleAssetBundlePack = isSingleBundlePack;
    }

    public int GetError()
    {
        return 0;
    }
    
    public void RequestCellularDataDownload()
    {
    }
}
