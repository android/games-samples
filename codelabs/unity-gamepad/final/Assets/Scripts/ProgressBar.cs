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

public class ProgressBar : MonoBehaviour
{
    private int totalDownloadSizeInMB = 0;
    private float percentComplete = 0.0f;

    public Slider slider;
    public Text percentLabel;

    public int TotalDownloadSizeInMB
    {
        get { return totalDownloadSizeInMB; }
        set
        {
            totalDownloadSizeInMB = value;
            UpdatePercentDisplay();
        }
    }

    public float PercentComplete 
    {
        get { return percentComplete; }
        set 
        {
            percentComplete = value;
            UpdatePercentDisplay();
        }
    }

    void UpdatePercentDisplay()
    {
        slider.value = percentComplete;

        if (totalDownloadSizeInMB == 0)
        {
            string percentString = (percentComplete * 100.0f).ToString("0.0");
            percentLabel.text = string.Format("{0}%", percentString);
        }
        else
        {
            string percentString = (percentComplete * 100.0f).ToString("0.0");
            string sizeString = totalDownloadSizeInMB.ToString();
            percentLabel.text = string.Format("{0}% - {1}MB", percentString, 
                sizeString);
        }
    }

    // Start is called before the first frame update
    void Start()
    {
        TotalDownloadSizeInMB = 0;
        PercentComplete = 0.0f;
    }

    // Update is called once per frame
    void Update()
    {
        
    }
}
