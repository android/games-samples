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

#ifndef BASEGAMEFRAMEWORK_GRAPHICSAPI_VULKAN_H_
#define BASEGAMEFRAMEWORK_GRAPHICSAPI_VULKAN_H_

#include "platform_defines.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <vector>
#include "volk.h"
#include "vk_mem_alloc.h"
#include "graphics_api_base.h"
#include "graphics_api_status.h"
#include "graphics_api_vulkan_resources.h"

namespace base_game_framework {

class GraphicsAPIVulkan : public GraphicsAPIBase {
 public:

  struct QueueFamilyIndices {
    std::optional<uint32_t> graphics_family;
    std::optional<uint32_t> present_family;
    bool isComplete() {
      return graphics_family.has_value() && present_family.has_value();
    }
  };

  struct SwapchainInfo {
    VkFormat swapchain_color_format_ = VK_FORMAT_UNDEFINED;
    VkFormat swapchain_depth_stencil_format_ = VK_FORMAT_UNDEFINED;
    // This is the index of the current in-flight frame:
    // i.e. 0/1 for double buffer, or 0/1/2 for triple buffer
    uint32_t swapchain_current_frame_index_ = 0;
    // This is the index of the current active swapchain image,
    // which may be greater than the number of max in-flight frames.
    // Each swapchain image has its own depth/stencil image, also selected by this index.
    uint32_t swapchain_current_image_index_ = 0;
    // Number of max in-flight frames
    uint32_t swapchain_frame_count_ = 0;
    // Number of swapchain images
    uint32_t swapchain_image_count_ = 0;
    // Number of swapchain images requested by InitSwapchain (the minimum image count used
    // when the swapchain is recreated, within the surface's limits; the swapchain may have
    // more)
    uint32_t swapchain_requested_image_count_ = 0;
    // The extent the swapchain images were created with (pre-rotated for 90/270 degree
    // transforms)
    VkExtent2D swapchain_extent_ = {0, 0};
    VkPresentModeKHR swapchain_present_mode_ = VK_PRESENT_MODE_FIFO_KHR;
    std::vector<VkImage> swapchain_images_;
    std::vector<VkImageView> swapchain_image_views_;
    std::vector<VkImage> swapchain_depth_stencil_images_;
    std::vector<VmaAllocation> swapchain_depth_stencil_allocs_;
    std::vector<VkImageView> swapchain_depth_stencil_image_views_;
    // One render complete semaphore per swapchain image, signaled by the frame's submit and
    // waited on by its present. A semaphore can only be signaled again once the presentation
    // engine is done with it, which is known once its image is acquired again, so these
    // can't be per in-flight frame (VUID-vkQueueSubmit-pSignalSemaphores-00067).
    std::vector<VkSemaphore> swapchain_render_complete_semaphores_;
  };

  GraphicsAPIVulkan();
  virtual ~GraphicsAPIVulkan();

  GraphicsAPIVulkan(const GraphicsAPIVulkan &) = delete;
  GraphicsAPIVulkan &operator=(const GraphicsAPIVulkan &) = delete;

  virtual DisplayManager::GraphicsAPI GetAPI() const { return DisplayManager::kGraphicsAPI_Vulkan; }
  virtual GraphicsAPIStatus GetAPIStatus() const { return api_status_; }
  virtual const GraphicsAPIFeatures &GetAPIFeatures() const { return api_features_; }

  virtual uint32_t GetFeatureFlags() const { return feature_flags_; }

  virtual void QueryAvailability();

  virtual bool InitializeGraphicsAPI();
  virtual void ShutdownGraphicsAPI();

  virtual bool SetDisplayChangedCallback(DisplayManager::DisplayChangedCallback callback,
                                         void *user_data);

  virtual bool SetSwapchainChangedCallback(DisplayManager::SwapchainChangedCallback callback,
                                           void *user_data);

  virtual void SwapchainChanged(const DisplayManager::SwapchainChangeMessage message);

  virtual DisplayManager::SwapchainConfigurations *GenerateSwapchainConfigurations();
  virtual DisplayManager::InitSwapchainResult InitSwapchain(
      const DisplayManager::DisplayFormat &display_format,
      const DisplayManager::DisplayResolution &display_resolution,
      const DisplayManager::DisplaySwapInterval display_swap_interval,
      const uint32_t swapchain_frame_count,
      const DisplayManager::SwapchainPresentMode present_mode);
  virtual void ShutdownSwapchain();
  virtual bool GetSwapchainValid();
  virtual DisplayManager::SwapchainFrameHandle GetCurrentSwapchainFrame();
  virtual DisplayManager::SwapchainFrameHandle PresentCurrentSwapchainFrame();

  DisplayManager::SwapchainRotationMode GetSwapchainRotationMode();

  // The platform window was resized: recreate the swapchain if the surface size or
  // transform no longer match it
  void HandleWindowResized();

  bool GetGraphicsAPIResourcesVk(GraphicsAPIResourcesVk &api_resources_vk);

  bool GetSwapchainFrameResourcesVk(const DisplayManager::SwapchainFrameHandle frame_handle,
                                    SwapchainFrameResourcesVk &frame_resources,
                                    bool acquire_frame_image);

  static void SetValidationLayersEnabled(bool enable) {
    GraphicsAPIVulkan::enable_validation_layers_ = enable;
  }

 private:
  bool CreateDevice(bool is_preflight_check, const QueueFamilyIndices &queue_indices);
  void DestroyDevice();

  void CreateInstance(bool is_preflight_check);
  void DestroyInstance();

  bool CreateSwapchain(const QueueFamilyIndices &queue_indices);
  // Creates and activates the swapchain and its images for the current surface, cleaning up
  // after itself on failure
  bool BuildSwapchain();
  // Deactivates and destroys the swapchain (if any) once the device is idle
  void TeardownSwapchain();
  // Recreates the swapchain for the current surface size and transform, then notifies the
  // display changed (if the resolution changed) and swapchain changed callbacks
  void RecreateSwapchain();
  void DestroySwapchain();
  bool CreateSwapchainImages();
  void DestroySwapchainImages();
  // Re-queries the surface capabilities and updates the transform, image count limits, the
  // display resolutions and the swapchain resolution. Returns false if the surface can't be
  // used for a swapchain right now (e.g. a zero size window); resolution_changed is set if
  // the swapchain resolution changed.
  bool UpdateSurfaceProperties(bool *resolution_changed);
  void NotifySwapchainChanged(const DisplayManager::SwapchainChangeMessage message);

  // Per in-flight frame fences and semaphores, created and destroyed with the swapchain
  // images, so each swapchain starts with signaled fences and unsignaled semaphores even if
  // a frame was interrupted between acquiring an image and submitting
  bool CreateSynchronizationObjects();
  void DestroySynchronizationObjects();

  VkPhysicalDevice GetPhysicalDevice();

  void QueryCapabilities();
  void QueryDeviceCapabilities(VkPhysicalDevice physical_device);
  void QuerySurfaceCapabilities();
  void DetermineAPILevel(const uint32_t api_version,
                         const uint32_t device_api_version);
  void DetermineNumericSupport(VkBool32 shader_int16,
                               const VkPhysicalDeviceShaderFloat16Int8FeaturesKHR &shader_float_16_int_8_features,
                               const VkPhysicalDevice16BitStorageFeaturesKHR &device_16_bit_storage_features);

  bool DisplayFormatExists(const DisplayManager::DisplayFormat &display_format);

  std::vector<DisplayManager::DisplayFormat> display_formats_;
  std::vector<DisplayManager::DisplayResolution> display_resolutions_;
  std::vector<DisplayManager::DisplaySwapInterval> swap_intervals_;

  // Optional device extensions found by QueryDeviceCapabilities, enabled on the real device
  // in CreateDevice along with the matching supported features below.
  std::vector<const char *> enabled_device_extensions_;
  VkPhysicalDeviceShaderFloat16Int8FeaturesKHR supported_float16_int8_features_{};
  VkPhysicalDevice16BitStorageFeaturesKHR supported_16bit_storage_features_{};

  GraphicsAPIStatus api_status_;
  GraphicsAPIFeatures api_features_;
  DisplayManager::DisplayChangedCallback display_changed_callback_;
  void *display_changed_user_data_;
  DisplayManager::SwapchainChangedCallback swapchain_changed_callback_;
  void *swapchain_changed_user_data_;
  uint32_t feature_flags_;
  uint32_t graphics_queue_index_;
  uint32_t present_queue_index_;
  DisplayManager::DisplayFormat swapchain_format_;
  DisplayManager::DisplayResolution swapchain_resolution_;
  DisplayManager::DisplaySwapInterval swapchain_interval_;
  uint32_t swapchain_min_frames_;
  uint32_t swapchain_max_frames_;
  uint32_t swapchain_present_modes_;
  bool swapchain_acquired_current_frame_image_;
  // InitSwapchain succeeded and ShutdownSwapchain wasn't called yet. The swapchain itself
  // doesn't exist while the window is lost, and is recreated when a window is gained.
  bool swapchain_initialized_;

  // Synchronization objects, per in-flight frame
  std::vector<VkFence> in_flight_frame_fence_;
  std::vector<VkSemaphore> swapchain_image_semaphore_;

  // Active swapchain variables
  SwapchainInfo swapchain_info_;

  VkDebugUtilsMessengerEXT debug_messenger_;
  VkInstance vk_instance_;
  VkDevice vk_device_;
  VkPhysicalDevice vk_physical_device_;
  VkQueue vk_graphics_queue_;
  VkQueue vk_present_queue_;
  VkSurfaceKHR vk_surface_;
  VkSwapchainKHR vk_swapchain_;
  VkDriverId driver_id_;
  VkSurfaceTransformFlagBitsKHR pretransform_flags_;
  VkSurfaceCapabilitiesKHR surface_capabilities_;

  VmaAllocator allocator_;

  // Extension bools
  bool use_physical_device_properties2_;
  // Validation layers are in use by the active instance/device: requested via
  // SetValidationLayersEnabled and available when the API was initialized
  bool validation_layers_active_;

  static constexpr const char *BGM_CLASS_TAG = "BGF::GraphicsAPIVulkan";
  static bool enable_validation_layers_;
};

}

#endif //BASEGAMEFRAMEWORK_GRAPHICSAPI_VULKAN_H_
