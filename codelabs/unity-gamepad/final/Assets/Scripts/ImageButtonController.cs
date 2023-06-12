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
using System.IO;
using UnityEngine;
using UnityEngine.UI;

public class ImageButtonController : MonoBehaviour
{
    public Image buttonImage;
    public Text buttonText;

    // Reference to the image to update when a button is selected
    private Image currentImage;
    // Reference to the demo title text
    private Text demoText;

    private void Start()
    {
        
    }

    void Update()
    {
        
    }

    public void SetupButton(string assetPath, Sprite newSprite,
        Image currentImageRef, Text demoTextRef)
    {
        buttonImage.sprite = newSprite;
        currentImage = currentImageRef;
        demoText = demoTextRef;

        // Trim the path and just use the asset name for the button text
        buttonText.text = Path.GetFileName(assetPath);
    }

    public void ImageButtonOnClick()
    {
        currentImage.sprite = buttonImage.sprite;

        string fmtText = "";
        TextureFormat fmt = buttonImage.sprite.texture.format;
        if (fmt == TextureFormat.ETC2_RGB ||
            fmt == TextureFormat.ETC2_RGBA1 ||
            fmt == TextureFormat.ETC2_RGBA8)
        {
            fmtText = "ETC2";
        }
        else if (fmt == TextureFormat.ASTC_RGB_4x4 ||
            fmt == TextureFormat.ASTC_RGB_4x4 ||
            fmt == TextureFormat.ASTC_RGB_5x5 ||
            fmt == TextureFormat.ASTC_RGB_6x6 ||
            fmt == TextureFormat.ASTC_RGB_8x8 ||
            fmt == TextureFormat.ASTC_RGB_10x10 ||
            fmt == TextureFormat.ASTC_RGB_12x12 ||
            fmt == TextureFormat.ASTC_RGBA_4x4 ||
            fmt == TextureFormat.ASTC_RGBA_4x4 ||
            fmt == TextureFormat.ASTC_RGBA_5x5 ||
            fmt == TextureFormat.ASTC_RGBA_6x6 ||
            fmt == TextureFormat.ASTC_RGBA_8x8 ||
            fmt == TextureFormat.ASTC_RGBA_10x10 ||
            fmt == TextureFormat.ASTC_RGBA_12x12)
        {
            fmtText = "ASTC";
        }
        demoText.text = string.Format("Unity Play Asset Delivery Demo {0}",
            fmtText);
    }
}
