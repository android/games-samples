/*
 * Copyright 2023 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef BASEGAMEFRAMEWORK_FILESYSTEMMANAGER_H_
#define BASEGAMEFRAMEWORK_FILESYSTEMMANAGER_H_

#include <cstdint>
#include <string>

namespace base_game_framework {

/**
 * @brief The base class definition for the `FilesystemManager` class of BaseGameFramework.
 * This class is used to retrieve filesystem directories for the game, and exposes an
 * interface to load 'package' files which are embedded in the application package
 * (i.e. Assets in Android packages/app bundles).
 */
class FilesystemManager {
 public:

  /**
   * @brief Enum for which type of filesystem directory path to request from
   * the FilesystemManager. Depending on the platform, some path types may
   * map to the same filesystem directory.
   */
  enum RootPathType : int32_t {
    /** @brief Filesystem path to internal device storage for the game */
    kRootPathInternalStorage = 0,
    /** @brief Filesystem path to external device storage for the game */
    kRootPathExternalStorage,
    /** @brief Filesystem path to a file cache directory for the game */
    kRootPathCache
  };

/**
 * @brief Retrieve an instance of the `FilesystemManager`. The first time this is called
 * it will construct and initialize the manager.
 * @return Reference to the `FilesystemManager` class.
 */
  static FilesystemManager &GetInstance();

/**
 * @brief Shuts down the `FilesystemManager`.
 */
  static void ShutdownInstance();

/**
 * @brief Not all platforms or devices have a true 'external' removable storage.
 * volume (i.e. physical sdcard). This function can be used to retrieve whether a device.
 * has external storage, or emulates it using its own internal storage.
 * @return true if device has external storage, false if the device emulates using internal storage.
 */
  bool GetExternalStorageEmulated() const;

/**
 * @brief Retrieves the file system path for the specified directory path type.
 * @param path_type A `RootPathType` enum specifying which directory path type to retrieve.
 * @return A string containing a stdio compatible filesystem path to the requested directory.
 */
  const std::string &GetRootPath(const RootPathType path_type) const;

/**
  * @brief Retrieves the file system path for the specified directory path type.
  * @param path_type A `RootPathType` enum specifying which directory path type to use.
  * @return The free size in bytes available to the requested path type.
  */
  int64_t GetFreeSpace(const RootPathType path_type) const;

/**
  * @brief Retrieves the size of an internal package file.
  * @param file_path A path to the internal package file.
  * @return The size in bytes of the specified file. Returns 0 if the file doesn't exist.
  */
  uint64_t GetPackageFileSize(const std::string &file_path);

/**
  * @brief Load an internal package file into a buffer.
  * @param file_path A path to the internal package file.
  * @param buffer_size The size of the `load_buffer` buffer. Expected to be the file size.
  * @param load_buffer A buffer the file will be loaded into.
  * @return The number of bytes read into the provided buffer. Returns 0 if the file doesn't exist.
  */
  uint64_t LoadPackageFile(const std::string file_path, const uint64_t buffer_size,
                           void *load_buffer);

/**
 * @brief Class destructor, do not call directly, use ::ShutdownInstance.
 */
  ~FilesystemManager() = default;

  FilesystemManager(const FilesystemManager &) = delete;
  FilesystemManager &operator=(const FilesystemManager &) = delete;

 private:
  FilesystemManager();

  std::string root_path_internal_;
  std::string root_path_external_;
  std::string root_path_cache_;
  static std::unique_ptr<FilesystemManager> instance_;
};

} // namespace base_game_framework

#endif // BASEGAMEFRAMEWORK_FILESYSTEMMANAGER_H_