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

using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;

public class SigninPageController : MonoBehaviour
{
    public Text signInText;
    public GameObject signInButton;

    private string _currentUserName = "";
    public string CurrentUserName
    {
        get { return _currentUserName; }
        set { _currentUserName = value; }
    }

    public PGSController.PgsSigninStatus CurrentSignInStatus { get; set; }

    // Refresh the page when it becomes active
    private void OnEnable()
    {
        RefreshPage();
    }

    public void RefreshPage()
    {
        switch (CurrentSignInStatus)
        {
            case PGSController.PgsSigninStatus.PgsSigninDisabled:
                signInText.text = "PGS Disabled";
                signInButton.SetActive(false);
                break;
            case PGSController.PgsSigninStatus.PgsSigninNotLoggedIn:
                signInText.text = "PGS Not Logged In";
                signInButton.SetActive(true);
                break;
            case PGSController.PgsSigninStatus.PgsSigninLoggedIn:
                signInText.text = CurrentUserName;
                signInButton.SetActive(false);
                break;
        }
    }
}
