/**********************************************************************
 * File:        hashfn.h  (Formerly hash.h)
 * Description: Portability hacks for hash_map, hash_set and unique_ptr.
 * Author:      Ray Smith
 * Created:     Wed Jan 08 14:08:25 PST 2014
 *
 * (C) Copyright 2014, Google Inc.
 ** Licensed under the Apache License, Version 2.0 (the "License");
 ** you may not use this file except in compliance with the License.
 ** You may obtain a copy of the License at
 ** http://www.apache.org/licenses/LICENSE-2.0
 ** Unless required by applicable law or agreed to in writing, software
 ** distributed under the License is distributed on an "AS IS" BASIS,
 ** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 ** See the License for the specific language governing permissions and
 ** limitations under the License.
 *
 **********************************************************************/

#ifndef           HASHFN_H
#define           HASHFN_H

#include <unordered_map>
#include <unordered_set>
#define hash_map std::unordered_map

using std::unordered_map;
using std::unordered_set;
#include <memory>
#define SmartPtr std::unique_ptr
#define HAVE_UNIQUE_PTR

#endif  // HASHFN_H
