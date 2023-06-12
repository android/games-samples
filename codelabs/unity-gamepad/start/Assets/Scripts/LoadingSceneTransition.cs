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
using UnityEngine.Networking;
using UnityEngine.SceneManagement;
using UnityEngine.UI;

public class LoadingSceneTransition : MonoBehaviour
{
    public bool usingAssetPacks;
    public string sceneToLoad;
    public Canvas loadingUICanvas;
    public Text headerText;
    public ProgressBar progressBar;
    public GameAssetManager gameAssetManager;

    public void LoadNextScene()
    {
        SceneManager.LoadScene(sceneToLoad);
    }

    private enum LoadingSceneState
    {
        LOADING_STARTUP,
        LOADING_ASSETBUNDLES,
        LOADING_DISCRETE,
        LOADING_ASSETS,
        LOADING_COMPLETE,
        LOADING_ERROR
    };

    private LoadingSceneState loadingSceneState = 
        LoadingSceneState.LOADING_STARTUP;
    private int totalItemsToLoad;
    private int currentItemsLoaded;
    private Queue<GameAssetReference> assetQueue;

    private void Start()
    {
#if UNITY_EDITOR
        LoadNextScene();
#endif
        assetQueue = new Queue<GameAssetReference>();
        SetLoadingSceneState(LoadingSceneState.LOADING_ASSETBUNDLES);
    }

    void Update()
    {
    }

    private void SetLoadingSceneState(LoadingSceneState newState)
    {
        if (loadingSceneState != newState)
        {
            loadingSceneState = newState;
            switch (newState)
            {
                case LoadingSceneState.LOADING_STARTUP:
                    break;
                case LoadingSceneState.LOADING_ASSETBUNDLES:
                    headerText.text = "Loading AssetBundles";
                    if (usingAssetPacks)
                    {
                        StartLoadingAssetBundlesFromAssetPacks();
                    }
                    else
                    {
                        StartLoadingAssetBundlesFromStreaming();
                    }
                    break;
                case LoadingSceneState.LOADING_DISCRETE:
                    if (usingAssetPacks)
                    {
                        headerText.text = "Loading discrete pack";
                        StartLoadingDiscreteAssetPack();
                    }
                    else
                    {
                        SetupDiscreteAssetsStreaming();
                    }
                    break;
                case LoadingSceneState.LOADING_ASSETS:
                    headerText.text = "Loading assets";
                    StartLoadingAssets();
                    break;
                case LoadingSceneState.LOADING_COMPLETE:
                    headerText.text = "Loading complete";
                    LoadNextScene();
                    break;
                case LoadingSceneState.LOADING_ERROR:
                    headerText.text = "Error loading assets";
                    progressBar.PercentComplete = 0.0f;
                    break;
            }
        }
    }

    private void StartLoadingAssetBundlesFromStreaming()
    {
        var assetBundleNameList = 
            gameAssetManager.GetAssetBundleNameList();
        currentItemsLoaded = 0;
        totalItemsToLoad = assetBundleNameList.Count;

        var assetBundleQueue = new Queue<string>(assetBundleNameList);
        StartCoroutine(LoadAssetBundlesFromStreaming(assetBundleQueue));
    }

    private void StartLoadingAssets()
    {
        totalItemsToLoad = 0;
        currentItemsLoaded = 0;

        // Collect the asset names from the asset bundles and queue for load
        var assetBundlesDictionary = 
            gameAssetManager.AssetBundles;
        var assetBundles = assetBundlesDictionary.Values;

        foreach(AssetBundle assetBundle in assetBundles)
        {
           var bundleAssets = assetBundle.GetAllAssetNames();
           foreach(string bundleAsset in bundleAssets)
           {
                string logString = string.Format("Loading {0} from bundle {1}",
                    bundleAsset, assetBundle.name);
                Debug.Log(logString);

                assetQueue.Enqueue(new GameAssetReference(bundleAsset, null,
                   assetBundle, 0, 0));
           } 
        }

        totalItemsToLoad = assetQueue.Count;

        StartCoroutine(LoadAssetsFromQueue());
    }

    private void SetupDiscreteAssetsStreaming()
    {
        // Get the list of discrete streaming assets and queue for load
        var discreteAssets = 
            gameAssetManager.GetDiscreteAssetNameList();
        foreach(string discreteAsset in discreteAssets)
        {
            assetQueue.Enqueue(new GameAssetReference(discreteAsset,
                null, null, 0, 0));
        }
        SetLoadingSceneState(LoadingSceneState.LOADING_ASSETS);
    }

    IEnumerator LoadAssetBundlesFromStreaming(Queue<string> assetBundleQueue)
    {
        while (assetBundleQueue.Count > 0)
        {
            string assetBundleName = assetBundleQueue.Dequeue();
            string assetBundlePath = Path.Combine(
                Application.streamingAssetsPath, assetBundleName);

            using (var request = UnityWebRequestAssetBundle.GetAssetBundle(
                assetBundlePath))
            {
                yield return request.SendWebRequest();

                if (!request.isNetworkError)
                {
                    // As long as we aren't in an error state create the new
                    // asset bundle and then update the display
                    if (loadingSceneState != LoadingSceneState.LOADING_ERROR)
                    {
                        AssetBundle newBundle = 
                            DownloadHandlerAssetBundle.GetContent(request);
                        Debug.Log("Loaded asset bundle: " + newBundle.name);
                        string assetBundleFileName = Path.GetFileName(
                            assetBundlePath);
                        gameAssetManager.AddAssetBundle(assetBundleFileName, 
                            newBundle);

                        // Update the progress bar
                        ++currentItemsLoaded;
                        float percentComplete = (float)currentItemsLoaded /
                            (float)totalItemsToLoad;
                        progressBar.PercentComplete = percentComplete;

                        string debugLog = String.Format("{0} {1} {2}",
                        currentItemsLoaded, totalItemsToLoad, percentComplete);
                        Debug.Log("AssetBundleLoad Progress: " + debugLog);

                        if (currentItemsLoaded == totalItemsToLoad)
                        {
                            Debug.Log("All asset bundles loaded");
                            SetLoadingSceneState(
                                LoadingSceneState.LOADING_DISCRETE);
                        }
                    }
                }
                else
                {
                    SetLoadingSceneState(LoadingSceneState.LOADING_ERROR);
                }
            }
        }
    }

    IEnumerator LoadAssetsFromQueue()
    {
        while (assetQueue.Count > 0)
        {
            GameAssetReference queueItem = assetQueue.Dequeue();
            AssetBundle assetBundle = queueItem.AssetBundle;
            string assetName = queueItem.AssetName;
            if (assetBundle == null)
            {
                string assetLocation = queueItem.AssetLocation;
                // Not in an asset bundle, if location is null, assume is in
                // streaming assets, set the location there
                if (assetLocation == null)
                {
                    string assetFullPath = Path.Combine(
                    Application.streamingAssetsPath, assetName);

                    using (var request = UnityWebRequest.Get(assetFullPath))
                    {
                        yield return request.SendWebRequest();

                        if (!request.isNetworkError)
                        {
                            Debug.Log("Finished discrete load of " + assetName);
                            // The asset is a raw .jpg or .png, convert to a sprite
                            gameAssetManager.AddSpriteAsset(assetName,
                                GameAssetManager.GenerateSpriteFromRawImage(
                                    request.downloadHandler.data
                                ));
                            ++currentItemsLoaded;
                        }
                    }
                }
                else
                {
                    // Load discrete from the specified location
                    long assetSize = queueItem.AssetSize;
                    long assetOffset = queueItem.AssetOffset;
                    var assetFileStream = File.OpenRead(assetLocation);
                    var assetBuffer = new byte[assetSize];
                    assetFileStream.Seek(assetOffset, SeekOrigin.Begin);
                    assetFileStream.Read(assetBuffer, 0, assetBuffer.Length);
                    Debug.Log("Finished discrete load of " + assetName);
                    // The asset is a raw .jpg or .png, convert to a sprite
                    gameAssetManager.AddSpriteAsset(assetName,
                        GameAssetManager.GenerateSpriteFromRawImage(
                            assetBuffer));
                    ++currentItemsLoaded;
                }
            }
            else
            {
                // asset is located in an asset bundle, load from it
                AssetBundleRequest request = 
                    assetBundle.LoadAssetAsync<Sprite>(assetName);
                yield return request;
                Sprite newSprite = (Sprite)request.asset;
                Debug.Log("Finished load of " + assetName);
                gameAssetManager.AddSpriteAsset(assetName, newSprite);
                ++currentItemsLoaded;
            }
            float percentComplete = (float)currentItemsLoaded /
                (float)totalItemsToLoad;
            progressBar.PercentComplete = percentComplete;

            string debugLog = String.Format("{0} {1} {2}",
            currentItemsLoaded, totalItemsToLoad, percentComplete);
            Debug.Log("Asset Load Progress: " + debugLog);

            if (currentItemsLoaded == totalItemsToLoad)
            {
                Debug.Log("All assets loaded");
                // Assets are done, exit the loading scene
                SetLoadingSceneState(
                    LoadingSceneState.LOADING_COMPLETE);
            }
        }
    }

    private void StartLoadingAssetBundlesFromAssetPacks()
    {
    }

    private void StartLoadingDiscreteAssetPack()
    {
    }
}
