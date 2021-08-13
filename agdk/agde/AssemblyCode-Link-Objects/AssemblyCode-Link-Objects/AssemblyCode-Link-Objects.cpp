/*
 * Copyright (C) 2021 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <string>
#include "static_lib.h"

// Declare the function implemented inside local shared lib module with assembly code.
extern "C" int get_id(void);

// example of using the functions implemented in static lib and inside this shared lib.
std::string get_platfom_info(void) {

    std::string str("Platform ID = ");
    str += std::to_string(get_id()) + ", Magic Number = "
        + std::to_string(get_sum(12, 2021));

    return str;
}
