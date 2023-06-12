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

using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using UnityEngine;
using UnityEngine.UI;

public class AssetPackInfoController : MonoBehaviour
{
    public Text assetPackNameLabel;
    public Text assetPackStatusLabel;
    public Text assetPackDownloadLabel;
    public Button assetPackDownloadButton;
    public ProgressBar assetPackProgressBar;
    
    private enum AssetPackInfoStatus
    {
        PACKSTATUS_PENDING,
        PACKSTATUS_NEEDS_DOWNLOAD,
        PACKSTATUS_NEEDS_PERMISSION,
        PACKSTATUS_DOWNLOADING,
        PACKSTATUS_READY,
        PACKSTATUS_ERROR
    };

    private GameAssetManager gameAssetManager;
    private GameAssetPack gameAssetPack;
    private AssetPackInfoStatus infoStatus;
    private bool waitingOnPermissionResult;

    private void Awake()
    {
        infoStatus = AssetPackInfoStatus.PACKSTATUS_PENDING;
        gameAssetManager = 
            GameObject.Find("GameAssetManager").GetComponent<GameAssetManager>();
        if (gameAssetManager == null) 
        {
            Debug.LogError("Failed to find GameAssetManager");
        }
        waitingOnPermissionResult = false;
    }

    // Update is called once per frame
    void Update()
    {
        UpdateUI();
    }

    public void SetAssetPack(string assetPackName, bool isSingleAssetBundlePack)
    {
        gameAssetPack = gameObject.AddComponent<GameAssetPack>();
        gameAssetPack.InitializeAssetPack(assetPackName, 
            isSingleAssetBundlePack);
        assetPackNameLabel.text = assetPackName;
        assetPackStatusLabel.text = "Pending";
        UpdateUI();
    }

    void UpdateUI()
    {
        if (infoStatus == AssetPackInfoStatus.PACKSTATUS_NEEDS_PERMISSION)
        {
            if (waitingOnPermissionResult)
            {
                if (!gameAssetPack.IsCellularConfirmationActive)
                {
                    waitingOnPermissionResult = false;
                    PermissionResult(gameAssetPack.DidApproveCellularData);
                }
            }
        }
        else if (gameAssetPack.IsDownloadActive())
        {
            UpdateDownloading();
        }
        else if (gameAssetPack.IsDownloaded())
        {
            if (infoStatus != AssetPackInfoStatus.PACKSTATUS_READY)
            {
                SetReady();
            }
        }
        else if (infoStatus == AssetPackInfoStatus.PACKSTATUS_PENDING)
        {
            UpdatePending();
        }
    }

    public void StartDownload()
    {
        if (infoStatus == AssetPackInfoStatus.PACKSTATUS_NEEDS_PERMISSION)
        {
            if (!waitingOnPermissionResult)
            {
                Debug.Log("Cellular data request retry");
                waitingOnPermissionResult = true;
                gameAssetPack.RequestCellularDataDownload();
            }
        }
        else if (infoStatus == AssetPackInfoStatus.PACKSTATUS_NEEDS_DOWNLOAD)
        {
            infoStatus = AssetPackInfoStatus.PACKSTATUS_DOWNLOADING;
            gameAssetPack.StartDownload();
            SetupDownloadUI();
        }
    }

    public void SetReady()
    {
        infoStatus = AssetPackInfoStatus.PACKSTATUS_READY;
        assetPackStatusLabel.text = "Ready";
        assetPackDownloadButton.gameObject.SetActive(false);
        assetPackProgressBar.gameObject.SetActive(false);
    }

    private void UpdateDownloading()
    {
    }

    private void SetupDownloadUI()
    {
        assetPackStatusLabel.text = "Downloading";
        assetPackProgressBar.PercentComplete = 0.0f;
        assetPackDownloadButton.gameObject.SetActive(false);
        assetPackProgressBar.gameObject.SetActive(true);
    }

    private void SetupErrorUI()
    {
        assetPackStatusLabel.text = String.Format("Error : {0}",
            gameAssetPack.GetError().ToString());
        assetPackDownloadButton.gameObject.SetActive(false);
        assetPackProgressBar.gameObject.SetActive(false);
    }

    public void PermissionResult(bool allow)
    {
        if (allow)
        {
            infoStatus = AssetPackInfoStatus.PACKSTATUS_DOWNLOADING;
            SetupDownloadUI();            
        }
        else
        {
            // If no permission granted, reset UI to allow asking again
            // by tapping Download
            UpdatePending();
            // But keep us in a needs permission state since we are keeping
            // the underlying asset pack request live
            infoStatus = AssetPackInfoStatus.PACKSTATUS_NEEDS_PERMISSION;            
        }
    }

    private void UpdatePending()
    {
        // We need to wait for the async size request to return the
        // size of the asset pack
        if (gameAssetPack.IsAssetPackSizeValid)
        {
            assetPackStatusLabel.text = "Needs Download";
            infoStatus = AssetPackInfoStatus.PACKSTATUS_NEEDS_DOWNLOAD;
            long assetPackSize = gameAssetPack.AssetPackSize;
            float assetPackSizeMB = (float)(((double)assetPackSize)
                / (1024.0 * 1024.0) );
            assetPackProgressBar.TotalDownloadSizeInMB = 
                (int)assetPackSizeMB;
            string downloadText = string.Format("Download {0:0.#} MB",
                assetPackSizeMB);
            assetPackDownloadLabel.text = downloadText;
            assetPackDownloadButton.gameObject.SetActive(true);
            assetPackProgressBar.gameObject.SetActive(false);
        }
    }

    IEnumerator LoadAssetsFromBundle(AssetBundle assetBundle)
    {
        var bundleAssets = assetBundle.GetAllAssetNames();
        var assetQueue = new Queue<string>(bundleAssets);
        while (assetQueue.Count > 0)
        {
            string assetName = assetQueue.Dequeue();
            AssetBundleRequest request = 
                assetBundle.LoadAssetAsync<Sprite>(assetName);
            yield return request;
            Sprite newSprite = (Sprite)request.asset;
            Debug.Log("Finished load of " + assetName);
            gameAssetManager.AddSpriteAsset(assetName, newSprite);
        }
        SetReady();
    }
}
