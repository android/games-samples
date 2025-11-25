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

public class BuildMenuItem
{
    public readonly string ItemName;
    public bool IsEnabled { get; set; }
    private string _enabledDirectiveName;
    private string _disabledDirectiveName;

    public BuildMenuItem(string itemName, bool isEnabled, string enabledDirectiveName)
    {
        ItemName = itemName;
        IsEnabled = isEnabled;
        _enabledDirectiveName = enabledDirectiveName;
        _disabledDirectiveName = string.Empty;
    }

    public BuildMenuItem(string itemName, bool isEnabled, string enabledDirectiveName,
            string disabledDirectiveName) : this(itemName, isEnabled, enabledDirectiveName)
    {
        _disabledDirectiveName = disabledDirectiveName;
    }

    public string GetDirective
    {
        get { return IsEnabled ? _enabledDirectiveName : _disabledDirectiveName; }
    }
}