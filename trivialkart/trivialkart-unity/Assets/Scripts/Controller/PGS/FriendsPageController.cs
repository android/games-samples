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

using UnityEngine;
using UnityEngine.UI;
#if PLAY_GAMES_SERVICES
using GooglePlayGames;
using GooglePlayGames.BasicApi;
#endif

public class FriendsPageController : MonoBehaviour
{
    public GameObject friendsListText;
    public GameObject requestFriendsButton;

    private const int DefaultPageSize = 50;

    // true if the user has given consent to access their friends list
    private bool _hasFriendPermission = false;
    // true if the friends list has been successfully loaded and parsed
    private bool _friendsAvailable = false;
#if PLAY_GAMES_SERVICES
    // true if we are requesting a forced refresh, invalidating any local friends
    // list cache
    private bool _forceFriendListRefresh = false;
#endif
    // Comma delimited list of all the player's friends
    private string _friendsList = "";

    // Refresh the page when it becomes active
    private void OnEnable()
    {
        RefreshPage();
    }

    private void RefreshPage()
    {
        // Display the list of friends if available, otherwise hide the text object
        if (_friendsAvailable)
        {
            var textObject = friendsListText.GetComponent<Text>();
            textObject.text = _friendsList;
            friendsListText.SetActive(true);
        }
        else
        {
            friendsListText.SetActive(false);
        }

        // If we don't have permission to the friends list, enable the button that
        // triggers the request for access
        if (_hasFriendPermission)
        {
            requestFriendsButton.SetActive(false);
        }
        else
        {
            requestFriendsButton.SetActive(true);
        }
    }

    // Called upon successful signin. Checks to see if we have
    // access to the friends list, and load it if so.
    public void InitializeFriendsPage()
    {
#if PLAY_GAMES_SERVICES
        PlayGamesPlatform.Instance.GetFriendsListVisibility(_forceFriendListRefresh,
            (friendsListVisibilityStatus) =>
            {
                switch(friendsListVisibilityStatus)
                {
                    case FriendsListVisibilityStatus.Unknown:
                        // Force a full refresh if Unknown is returned
                        _forceFriendListRefresh = true;
                        _hasFriendPermission = true;
                        break;
                    case FriendsListVisibilityStatus.Visible:
                        _hasFriendPermission = true;
                        break;
                    case FriendsListVisibilityStatus.ResolutionRequired:
                        break;
                    default:
                        break;
                }

                if (_hasFriendPermission)
                {
                    LoadFriendsList();
                }
            });
#endif
    }

    // Call the PGS function to request access to the friends list.
    // Automatically load the friends list if consent is given.
    public void RequestFriendsAccess()
    {
#if PLAY_GAMES_SERVICES
        PlayGamesPlatform.Instance.AskForLoadFriendsResolution((result) =>
        {
            if (result == UIStatus.Valid) {
                // User agreed to share friends with the game. Reload friends.
                _hasFriendPermission = true;
                LoadFriendsList();
            } else {
                // User didn't’t agree to share the friends list.
                _hasFriendPermission = false;
            }
        });
#endif
    }

#if PLAY_GAMES_SERVICES
    // Load the friends list. If the user has more friends than our
    // default page size, use LoadAdditionalFriends to iterate
    // until all have been loaded.
    private void LoadFriendsList()
    {
        // Reset the list
        _friendsList = "";
        _friendsAvailable = false;

        PlayGamesPlatform.Instance.LoadFriends(DefaultPageSize, _forceFriendListRefresh,
            (status) =>
            {
                if (status == LoadFriendsStatus.LoadMore)
                {
                    AddFriendsFromList();
                    LoadAdditionalFriends();
                    _friendsAvailable = true;
                }
                else if (status == LoadFriendsStatus.Completed)
                {
                    AddFriendsFromList();
                    _friendsAvailable = true;
                    RefreshPage();
                }
                else if (status == LoadFriendsStatus.ResolutionRequired)
                {
                    _hasFriendPermission = false;
                }
                // Reset the forced refresh after calling LoadFriends
                _forceFriendListRefresh = false;
            });
    }

    // The user had more friends than our default page size, iterate
    // LoadMoreFriends until we have paged through the entire friends list
    private void LoadAdditionalFriends()
    {
        PlayGamesPlatform.Instance.LoadMoreFriends(DefaultPageSize, (status) =>
        {
            if (status == LoadFriendsStatus.LoadMore)
            {
                AddFriendsFromList();
                LoadAdditionalFriends();
            }
            else if (status == LoadFriendsStatus.Completed)
            {
                AddFriendsFromList();
                RefreshPage();
            }
        });
    }

    // Add the currently loaded set of the friends list to
    // the string listing all the user's friends, comma-delimited
    private void AddFriendsFromList()
    {
        foreach (var friend in Social.localUser.friends)
        {
            if (string.IsNullOrEmpty(_friendsList))
            {
                _friendsList = friend.userName;
            }
            else
            {
                _friendsList += (", " + friend.userName);
            }
        }
    }
#endif
}
