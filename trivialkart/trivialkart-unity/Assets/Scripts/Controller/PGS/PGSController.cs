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

#if PLAY_GAMES_SERVICES
using GooglePlayGames;
using GooglePlayGames.BasicApi;
#endif

public class PGSController : MonoBehaviour
{
    public enum PgsSigninStatus
    {
        PgsSigninDisabled = 0,
        PgsSigninNotLoggedIn = 1,
        PgsSigninLoggedIn = 2
    }

    public bool PgsEnabled { get; private set; }
    public PgsSigninStatus CurrentSignInStatus { get; set; }

    // Sign-in
    public GameObject signinPage;

#if PLAY_GAMES_SERVICES
    private SigninPageController _signinPageController;
    private bool _launchedStartupSignin;
#endif

    // Start is called before the first frame update
    void Start()
    {
#if PLAY_GAMES_SERVICES
        _signinPageController = signinPage.GetComponent<SigninPageController>();
        _launchedStartupSignin = false;
        CurrentSignInStatus = PgsSigninStatus.PgsSigninNotLoggedIn;
        PgsEnabled = true;
#else
        CurrentSignInStatus = PgsSigninStatus.PgsSigninDisabled;
        PgsEnabled = false;
#endif
    }

#if PLAY_GAMES_SERVICES
    // Update is called once per frame
    void Update()
    {
        if (!_launchedStartupSignin)
        {
            _launchedStartupSignin = true;
            RunStartupSignin();
        }
    }

    public void RunStartupSignin()
    {
        PlayGamesPlatform.Instance.Authenticate(ProcessAuthentication);
    }

    public void RunManualSignin()
    {
        PlayGamesPlatform.Instance.ManuallyAuthenticate(ProcessAuthentication);
    }

    private void ProcessAuthentication(SignInStatus status)
    {
        if (status == SignInStatus.Success)
        {
            _signinPageController.CurrentUserName =
                PlayGamesPlatform.Instance.GetUserDisplayName();
            CurrentSignInStatus = PgsSigninStatus.PgsSigninLoggedIn;
            _signinPageController.CurrentSignInStatus = CurrentSignInStatus;
            _signinPageController.RefreshPage();
        }
        else
        {
            CurrentSignInStatus = PgsSigninStatus.PgsSigninNotLoggedIn;
            _signinPageController.CurrentSignInStatus = CurrentSignInStatus;
            _signinPageController.RefreshPage();
        }
    }
#endif // PLAY_GAMES_SERVICES
}
