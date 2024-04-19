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

#include "graphics_api_vulkan.h"
#include "debug_manager.h"
#include "graphics_api_vulkan_utils.h"
#include "platform_util_vulkan.h"
#include <vector>

namespace base_game_framework {

static constexpr DisplayManager::SwapchainFrameHandle kDefault_swapchain_handle = 1;

bool GraphicsAPIVulkan::enable_validation_layers_ = false;

static VKAPI_ATTR VkBool32 VKAPI_CALL
messengerCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                  VkDebugUtilsMessageTypeFlagsEXT messageType,
                  const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                  void * /* pUserData */) {
  const char *ms = VulkanAPIUtils::GetMessageSeverityString(messageSeverity);
  const char *mt = VulkanAPIUtils::GetMessageTypeString(messageType);

  DebugManager::Log(DebugManager::kLog_Channel_Default,
                    DebugManager::kLog_Level_Error,
                    "Vulkan Validation",
                    "[%s: %s]\n%s\n", ms, mt, pCallbackData->pMessage);

  return VK_FALSE;
}

GraphicsAPIVulkan::GraphicsAPIVulkan()
    : GraphicsAPIBase(),
      display_formats_(),
      display_resolutions_(),
      swap_intervals_(),
      enabled_device_extensions_(),
      api_status_(kGraphicsAPI_ObtainingAvailability),
      api_features_(),
      display_changed_callback_(nullptr),
      display_changed_user_data_(nullptr),
      swapchain_changed_callback_(nullptr),
      swapchain_changed_user_data_(nullptr),
      feature_flags_(DisplayManager::kNo_Vulkan_Support),
      graphics_queue_index_(0),
      present_queue_index_(0),
      swapchain_format_(),
      swapchain_resolution_(0, 0, 0, DisplayManager::kDisplay_Orientation_Landscape),
      swapchain_interval_(DisplayManager::kDisplay_Swap_Interval_60FPS),
      swapchain_min_frames_(0),
      swapchain_max_frames_(0),
      swapchain_present_modes_(0),
      swapchain_acquired_current_frame_image_(false),
      in_flight_frame_fence_(),
      frame_render_completion_semaphore_(),
      swapchain_image_semaphore_(),
      swapchain_info_(),
      debug_messenger_(VK_NULL_HANDLE),
      vk_instance_(VK_NULL_HANDLE),
      vk_device_(VK_NULL_HANDLE),
      vk_physical_device_(VK_NULL_HANDLE),
      vk_graphics_queue_(VK_NULL_HANDLE),
      vk_present_queue_(VK_NULL_HANDLE),
      vk_surface_(VK_NULL_HANDLE),
      vk_swapchain_(VK_NULL_HANDLE),
      driver_id_(VK_DRIVER_ID_MAX_ENUM),
      pretransform_flags_(VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR),
      surface_capabilities_{},
      allocator_(VK_NULL_HANDLE),
      use_physical_device_properties2_(false) {
}

GraphicsAPIVulkan::~GraphicsAPIVulkan() {

}

VkPhysicalDevice GraphicsAPIVulkan::GetPhysicalDevice() {
  uint32_t device_count = 0;
  VulkanAPIUtils::CheckVkResult(vkEnumeratePhysicalDevices(vk_instance_, &device_count, nullptr),
                                "vkEnumeratePhysicalDevices");
  if (device_count > 0) {
    std::vector<VkPhysicalDevice> devices(device_count);
    vkEnumeratePhysicalDevices(vk_instance_, &device_count, devices.data());
    std::vector<VkPhysicalDevice> rendering_devices;
    // Screen the list of physical devices and filter out those
    // which we can't actually use for rendering
    for (const VkPhysicalDevice &device : devices) {
      const std::vector<const char *> required_device_extensions =
          PlatformUtilVulkan::GetRequiredDeviceExtensions(device);
      if (VulkanAPIUtils::IsRenderingDevice(device, vk_surface_,
                                            required_device_extensions)) {
        rendering_devices.push_back(device);
      }
    }
    if (!rendering_devices.empty()) {
      VkPhysicalDevice best_device = (rendering_devices.size() == 1) ? rendering_devices[0] :
                                     PlatformUtilVulkan::GetBestDevice(rendering_devices);
      return best_device;
    }
  }
  return VK_NULL_HANDLE;
}

void GraphicsAPIVulkan::QueryAvailability() {
  // There is a dependency on a native window being available to attach a surface
  // so we can't query for devices and their capabilities until the window is available to us
  if (PlatformUtilVulkan::HasNativeWindow()) {
    QueryCapabilities();
  }
}

void GraphicsAPIVulkan::QueryCapabilities() {
  use_physical_device_properties2_ = VulkanAPIUtils::GetUsePhysicalDeviceProperties2();
  // We need to create a temporary instance and surface in order to properly
  // enumerate physical devices and check their capabilities
  if (volkInitialize() == VK_SUCCESS) {
    CreateInstance(true);
    if (vk_instance_ != VK_NULL_HANDLE) {
      vk_surface_ = PlatformUtilVulkan::CreateSurface(vk_instance_);
      if (vk_surface_ != VK_NULL_HANDLE) {
        vk_physical_device_ = GetPhysicalDevice();
        if (vk_physical_device_ != VK_NULL_HANDLE) {
          QuerySurfaceCapabilities();
          QueryDeviceCapabilities(vk_physical_device_);
          VulkanAPIUtils::GetDisplayFormats(vk_physical_device_, vk_surface_, display_formats_);
          PlatformUtilVulkan::GetScreenResolutions(surface_capabilities_, display_resolutions_);

          QueueFamilyIndices queue_indices = VulkanAPIUtils::FindQueueFamilies(
              vk_physical_device_, vk_surface_);
          if (CreateDevice(true, queue_indices)) {
            // Temp formats for temp swapchain, on Android we need a swapchain
            // to pull refresh rates from Swappy
            if (!display_formats_.empty() && !display_resolutions_.empty()) {
              swapchain_format_ = display_formats_[0];
              swapchain_resolution_ = display_resolutions_[0];
              swapchain_interval_ = DisplayManager::kDisplay_Swap_Interval_60FPS;
              if (CreateSwapchain(queue_indices)) {
                PlatformUtilVulkan::GetRefreshRates(vk_physical_device_, vk_device_,
                                                    vk_swapchain_, vk_present_queue_,
                                                    present_queue_index_, swap_intervals_);
                DestroySwapchain();
              }
            }
            DestroyDevice();
          }
        }
        PlatformUtilVulkan::DestroySurface(vk_instance_, vk_surface_);
        vk_surface_ = VK_NULL_HANDLE;
      }
      vk_physical_device_ = VK_NULL_HANDLE;
      DestroyInstance();
    }
  }
  api_status_ = kGraphicsAPI_AvailabilityReady;
}

void GraphicsAPIVulkan::QueryDeviceCapabilities(VkPhysicalDevice physical_device) {
  // If we got this far, we at least support a Vulkan 1.0 device
  feature_flags_ = DisplayManager::kVulkan_1_0_Support;
  api_features_.SetActiveAPIVersion(VK_API_VERSION_1_0);

  if (!use_physical_device_properties2_) {
    // If VK_KHR_get_physical_device_properties2 isn't supported, assume a really old
    // 1.0.x device and early out.
    DebugManager::Log(DebugManager::kLog_Channel_Default,
                      DebugManager::kLog_Level_Error,
                      BGM_CLASS_TAG,
                      "No VK_KHR_get_physical_device_properties2, assume 1.0");
    return;
  }
  uint32_t extension_count;
  vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extension_count,
                                       nullptr);

  std::vector<VkExtensionProperties> available_extensions(extension_count);
  vkEnumerateDeviceExtensionProperties(physical_device, nullptr, &extension_count,
                                       available_extensions.data());

  // Optional extensions we may query
  const bool has_driver_properties = VulkanAPIUtils::HasDeviceExtension(available_extensions,
                                                                        VK_KHR_DRIVER_PROPERTIES_EXTENSION_NAME);
  const bool has_shader_float16_int8 = VulkanAPIUtils::HasDeviceExtension(available_extensions,
                                                                          VK_KHR_SHADER_FLOAT16_INT8_EXTENSION_NAME);
  if (has_shader_float16_int8) {
    enabled_device_extensions_.push_back(VK_KHR_SHADER_FLOAT16_INT8_EXTENSION_NAME);
  }
  const bool has_16bit_storage = VulkanAPIUtils::HasDeviceExtension(available_extensions,
                                                                    VK_KHR_16BIT_STORAGE_EXTENSION_NAME);
  if (has_16bit_storage) {
    enabled_device_extensions_.push_back(VK_KHR_16BIT_STORAGE_EXTENSION_NAME);
  }

  // Retrieve device properties, including driver properties if available
  VkPhysicalDeviceProperties2KHR device_properties{};
  device_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2_KHR;
  VkPhysicalDeviceDriverPropertiesKHR device_driver_properties{};
  device_driver_properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DRIVER_PROPERTIES_KHR;
  if (has_driver_properties) {
    // Chain the properties structures
    device_properties.pNext = &device_driver_properties;
  }
  if (vkGetPhysicalDeviceProperties2 != nullptr) {
    vkGetPhysicalDeviceProperties2(physical_device, &device_properties);
  } else if (vkGetPhysicalDeviceProperties2KHR != nullptr) {
    vkGetPhysicalDeviceProperties2KHR(physical_device, &device_properties);
  } else {
    DebugManager::Log(DebugManager::kLog_Channel_Default,
                      DebugManager::kLog_Level_Error,
                      BGM_CLASS_TAG,
                      "No vkGetPhysicalDeviceProperties2/KHR functions");
    return;
  }

  if (has_driver_properties) {
    driver_id_ = device_driver_properties.driverID;
  }

  // Retrieve device features, including optional extensions on reduced precision formats
  VkPhysicalDeviceFeatures2KHR device_features{};
  device_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2_KHR;
  VkPhysicalDeviceShaderFloat16Int8FeaturesKHR shader_float_16_int_8_features{};
  shader_float_16_int_8_features.sType =
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FLOAT16_INT8_FEATURES_KHR;
  VkPhysicalDevice16BitStorageFeaturesKHR device_16_bit_storage_features{};
  device_16_bit_storage_features.sType =
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_16BIT_STORAGE_FEATURES_KHR;
  // Chain the features structures
  if (has_16bit_storage) {
    device_features.pNext = &device_16_bit_storage_features;
    if (has_shader_float16_int8) {
      device_16_bit_storage_features.pNext = &shader_float_16_int_8_features;
    }
  } else if (has_shader_float16_int8) {
    device_features.pNext = &shader_float_16_int_8_features;
  }

  if (vkGetPhysicalDeviceFeatures2 != nullptr) {
    vkGetPhysicalDeviceFeatures2(physical_device, &device_features);
  } else if (vkGetPhysicalDeviceFeatures2KHR != nullptr) {
    vkGetPhysicalDeviceFeatures2KHR(physical_device, &device_features);
  } else {
    DebugManager::Log(DebugManager::kLog_Channel_Default,
                      DebugManager::kLog_Level_Error,
                      BGM_CLASS_TAG,
                      "No vkGetPhysicalDeviceFeatures2/KHR functions");
    return;
  }

  if (device_features.features.wideLines == VK_TRUE) {
    api_features_.SetGraphicsFeature(GraphicsAPIFeatures::kGraphicsFeature_Wide_Lines);
  }

  const uint32_t api_version = PlatformUtilVulkan::GetVulkanApiVersion();
  DetermineAPILevel(api_version, device_properties.properties.apiVersion);

  // Determine if the device has ETC2 texture support
  if (device_features.features.textureCompressionETC2) {
      feature_flags_ |= DisplayManager::kVulkan_ETC2_Support;
  }

  DetermineNumericSupport(device_features.features.shaderInt16,
                          shader_float_16_int_8_features, device_16_bit_storage_features);
  // TODO: Android Baseline Profile detection
}

void GraphicsAPIVulkan::QuerySurfaceCapabilities() {
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vk_physical_device_, vk_surface_,
                                            &surface_capabilities_);
  swapchain_min_frames_ = surface_capabilities_.minImageCount;
  swapchain_max_frames_ = surface_capabilities_.maxImageCount;
  swapchain_info_.swapchain_image_count_ = swapchain_min_frames_;
  pretransform_flags_ = surface_capabilities_.currentTransform;

  uint32_t present_mode_count = 0;
  vkGetPhysicalDeviceSurfacePresentModesKHR(vk_physical_device_, vk_surface_, &present_mode_count,
                                            nullptr);
  std::vector<VkPresentModeKHR> present_modes;
  present_modes.resize(present_mode_count);
  vkGetPhysicalDeviceSurfacePresentModesKHR(vk_physical_device_, vk_surface_, &present_mode_count,
                                            present_modes.data());
  for (const VkPresentModeKHR &mode : present_modes) {
    switch (mode) {
      // The DisplayManager enums map directly to the VkPresentMode values we currently
      // support
      case VK_PRESENT_MODE_IMMEDIATE_KHR:
      case VK_PRESENT_MODE_MAILBOX_KHR:
      case VK_PRESENT_MODE_FIFO_KHR:
      case VK_PRESENT_MODE_FIFO_RELAXED_KHR:
        swapchain_present_modes_ |= (1U << mode);
        break;
      default:break;
    }
  }
}

void GraphicsAPIVulkan::DetermineAPILevel(const uint32_t api_version,
                                          const uint32_t device_api_version) {
  uint32_t instance_api_version = 0;
  vkEnumerateInstanceVersion(&instance_api_version);
  const uint32_t instance_major_version = VK_VERSION_MAJOR(instance_api_version);
  const uint32_t instance_minor_version = VK_VERSION_MINOR(instance_api_version);

  const uint32_t device_major_version = VK_VERSION_MAJOR(device_api_version);
  const uint32_t device_minor_version = VK_VERSION_MINOR(device_api_version);

  // Instance and device versions don't have to match, use the lowest version number for
  // API support if they don't
  const uint32_t api_major_version = (instance_major_version < device_major_version) ?
                                     instance_major_version : device_major_version;
  const uint32_t api_minor_version = (instance_minor_version < device_minor_version) ?
                                     instance_minor_version : device_minor_version;

  DebugManager::Log(DebugManager::kLog_Channel_Default,
                    DebugManager::kLog_Level_Info,
                    BGM_CLASS_TAG,
                    "VULKAN_VERSIONS api %u.%u instance %u.%u device %u.%u real %u.%u",
                    VK_VERSION_MAJOR(api_version), VK_VERSION_MINOR(api_version),
                    instance_major_version, instance_minor_version,
                    device_major_version, device_minor_version,
                    api_major_version, api_minor_version);

  if (api_version >= VK_API_VERSION_1_3 && api_major_version >= 1 && api_minor_version >= 3) {
    feature_flags_ |= (DisplayManager::kVulkan_1_3_Support |
        DisplayManager::kVulkan_1_2_Support |
        DisplayManager::kVulkan_1_1_Support);
    api_features_.SetActiveAPIVersion(VK_API_VERSION_1_3);
  } else if (api_version >= VK_API_VERSION_1_2 && api_major_version >= 1 &&
      api_minor_version >= 2) {
    feature_flags_ |= (DisplayManager::kVulkan_1_2_Support |
        DisplayManager::kVulkan_1_1_Support);
    api_features_.SetActiveAPIVersion(VK_API_VERSION_1_2);
  } else if (api_version >= VK_API_VERSION_1_1 && api_major_version >= 1 &&
      api_minor_version >= 1) {
    feature_flags_ |= DisplayManager::kVulkan_1_1_Support;
    api_features_.SetActiveAPIVersion(VK_API_VERSION_1_1);
  }
}

void GraphicsAPIVulkan::DetermineNumericSupport(VkBool32 shader_int16,
                                                const VkPhysicalDeviceShaderFloat16Int8FeaturesKHR &shader_float_16_int_8_features,
                                                const VkPhysicalDevice16BitStorageFeaturesKHR &device_16_bit_storage_features) {
  if (shader_int16) {
    api_features_.SetGraphicsFeature(GraphicsAPIFeatures::kGraphicsFeature_I16_Math);
  }
  if (shader_float_16_int_8_features.shaderInt8) {
    api_features_.SetGraphicsFeature(GraphicsAPIFeatures::kGraphicsFeature_I8_Math);
  }
  if (shader_float_16_int_8_features.shaderFloat16) {
    api_features_.SetGraphicsFeature(GraphicsAPIFeatures::kGraphicsFeature_F16_Math);
  }
  if (device_16_bit_storage_features.storageBuffer16BitAccess) {
    api_features_.SetGraphicsFeature(GraphicsAPIFeatures::kGraphicsFeature_F16_I16_SSBO);
  }
  if (device_16_bit_storage_features.uniformAndStorageBuffer16BitAccess) {
    api_features_.SetGraphicsFeature(GraphicsAPIFeatures::kGraphicsFeature_F16_I16_SSBO);
    api_features_.SetGraphicsFeature(GraphicsAPIFeatures::kGraphicsFeature_F16_I16_UBO);
  }
  if (device_16_bit_storage_features.storagePushConstant16) {
    api_features_.SetGraphicsFeature(GraphicsAPIFeatures::kGraphicsFeature_F16_I16_Push_Constant);
  }
  if (device_16_bit_storage_features.storageInputOutput16) {
    api_features_.SetGraphicsFeature(GraphicsAPIFeatures::kGraphicsFeature_F16_I16_Input_Output);
  }
}

bool GraphicsAPIVulkan::DisplayFormatExists(const DisplayManager::DisplayFormat &display_format) {
  // Config list is small, just do a linear search
  for (auto &format : display_formats_) {
    if (format == display_format) {
      return true;
    }
  }
  return false;
}

bool GraphicsAPIVulkan::InitializeGraphicsAPI() {
  if (!PlatformUtilVulkan::GetValidationLayersAvailable()) {
    // Disable request for validation layers if they aren't available
    GraphicsAPIVulkan::enable_validation_layers_ = false;
    DebugManager::Log(DebugManager::kLog_Channel_Default,
                      DebugManager::kLog_Level_Error,
                      BGM_CLASS_TAG,
                      "Vulkan validation layers were requested but unavailable!");
  }
  CreateInstance(false);
  if (vk_instance_ != VK_NULL_HANDLE) {
    vk_surface_ = PlatformUtilVulkan::CreateSurface(vk_instance_);
    if (vk_surface_ != VK_NULL_HANDLE) {
      vk_physical_device_ = GetPhysicalDevice();
      if (vk_physical_device_ != VK_NULL_HANDLE) {
        QueueFamilyIndices queue_indices = VulkanAPIUtils::FindQueueFamilies(
            vk_physical_device_, vk_surface_);
        if (CreateDevice(false, queue_indices)) {
          volkLoadDevice(vk_device_);

          // Initialize Vulkan Memory Allocator library
          VmaAllocatorCreateInfo allocator_info = {};
          allocator_info.physicalDevice = vk_physical_device_;
          allocator_info.device = vk_device_;
          allocator_info.instance = vk_instance_;
          allocator_info.vulkanApiVersion = api_features_.GetActiveAPIVersion();
          static VmaVulkanFunctions vulkan_functions = {}; // TODO: cleanup
          vulkan_functions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
          vulkan_functions.vkGetDeviceProcAddr = vkGetDeviceProcAddr;
          allocator_info.pVulkanFunctions = &vulkan_functions;

          VkResult vma_result = vmaCreateAllocator(&allocator_info, &allocator_);
          if (vma_result != VK_SUCCESS) {
            DebugManager::Log(DebugManager::kLog_Channel_Default,
                              DebugManager::kLog_Level_Error,
                              BGM_CLASS_TAG,
                              "vmaCreateAllocator failed: %d", vma_result);
            return false;
          }
          api_status_ = kGraphicsAPI_Active;
          return true;
        }
      }
    }
  }
  return false;
}

void GraphicsAPIVulkan::ShutdownGraphicsAPI() {
  if (vk_swapchain_ != VK_NULL_HANDLE) {
    DestroySwapchain();
  }
  if (allocator_ != VK_NULL_HANDLE) {
    vmaDestroyAllocator(allocator_);
    allocator_ = VK_NULL_HANDLE;
  }

  if (vk_device_ != VK_NULL_HANDLE) {
    DestroyDevice();
  }
  PlatformUtilVulkan::DestroySurface(vk_instance_, vk_surface_);
  vk_surface_ = VK_NULL_HANDLE;

  if (debug_messenger_ != VK_NULL_HANDLE) {
    vkDestroyDebugUtilsMessengerEXT(vk_instance_, debug_messenger_, nullptr);
    debug_messenger_ = VK_NULL_HANDLE;
  }

  DestroyInstance();
  api_status_ = kGraphicsAPI_AvailabilityReady;
}

DisplayManager::SwapchainConfigurations *GraphicsAPIVulkan::GenerateSwapchainConfigurations() {
  DisplayManager::SwapchainConfigurations *swap_configs =
      new DisplayManager::SwapchainConfigurations(display_formats_, display_resolutions_,
                                                  swap_intervals_, swapchain_min_frames_,
                                                  swapchain_max_frames_, swapchain_present_modes_,
                                                  DisplayManager::kDefault_Display);
  return swap_configs;
}

DisplayManager::InitSwapchainResult GraphicsAPIVulkan::InitSwapchain(
    const DisplayManager::DisplayFormat &display_format,
    const DisplayManager::DisplayResolution &display_resolution,
    const DisplayManager::DisplaySwapInterval display_swap_interval,
    const uint32_t swapchain_frame_count,
    const DisplayManager::SwapchainPresentMode present_mode) {

  // Validate parameters
  bool found_format = false;
  for (const DisplayManager::DisplayFormat &iter_format : display_formats_) {
    if (iter_format == display_format) {
      found_format = true;
      break;
    }
  }
  if (!found_format) {
    DebugManager::Log(DebugManager::kLog_Channel_Default,
                      DebugManager::kLog_Level_Error,
                      BGM_CLASS_TAG,
                      "InitSwapchain invalid display_format");
    return DisplayManager::kInit_Swapchain_Failure;
  }

  bool found_resolution = false;
  for (const DisplayManager::DisplayResolution &iter_resolution : display_resolutions_) {
    if (iter_resolution == display_resolution) {
      found_resolution = true;
      break;
    }
  }
  if (!found_resolution) {
    DebugManager::Log(DebugManager::kLog_Channel_Default,
                      DebugManager::kLog_Level_Error,
                      BGM_CLASS_TAG,
                      "InitSwapchain invalid display_resolution");
    return DisplayManager::kInit_Swapchain_Failure;
  }

  bool found_interval = false;
  for (const DisplayManager::DisplaySwapInterval &iter_interval : swap_intervals_) {
    if (iter_interval == display_swap_interval) {
      found_interval = true;
      break;
    }
  }
  if (!found_interval) {
    DebugManager::Log(DebugManager::kLog_Channel_Default,
                      DebugManager::kLog_Level_Error,
                      BGM_CLASS_TAG,
                      "InitSwapchain invalid display_swap_interval");
    return DisplayManager::kInit_Swapchain_Failure;
  }

  if (!(swapchain_frame_count >= swapchain_min_frames_ &&
      swapchain_frame_count <= swapchain_max_frames_)) {
    DebugManager::Log(DebugManager::kLog_Channel_Default,
                      DebugManager::kLog_Level_Error,
                      BGM_CLASS_TAG,
                      "InitSwapchain invalid swapchain_frame_count");
    return DisplayManager::kInit_Swapchain_Failure;
  }

  if (((1U << present_mode) & swapchain_present_modes_) == 0) {
    DebugManager::Log(DebugManager::kLog_Channel_Default,
                      DebugManager::kLog_Level_Error,
                      BGM_CLASS_TAG,
                      "InitSwapchain invalid present_mode");
    return DisplayManager::kInit_Swapchain_Failure;
  }

  // Passed validation, set up the swapchain
  static constexpr VkPresentModeKHR kPresentModes[] = {
      VK_PRESENT_MODE_IMMEDIATE_KHR,
      VK_PRESENT_MODE_MAILBOX_KHR,
      VK_PRESENT_MODE_FIFO_KHR,
      VK_PRESENT_MODE_FIFO_RELAXED_KHR
  };

  QueueFamilyIndices queue_indices = VulkanAPIUtils::FindQueueFamilies(
      vk_physical_device_, vk_surface_);
  swapchain_format_ = display_format;
  swapchain_resolution_ = display_resolution;
  swapchain_interval_ = display_swap_interval;
  swapchain_info_.swapchain_present_mode_ = kPresentModes[present_mode];
  swapchain_info_.swapchain_image_count_ = swapchain_frame_count;

  if (DisplayManager::GetInstance().GetDisplayBufferMode() ==
      DisplayManager::kDisplay_Triple_Buffer) {
    swapchain_info_.swapchain_frame_count_ = 3;
  } else {
    swapchain_info_.swapchain_frame_count_ = 2;
  }

  if (!CreateSwapchain(queue_indices)) {
    return DisplayManager::kInit_Swapchain_Failure;
  }

  CreateSwapchainImages();

  if (!PlatformUtilVulkan::ActivateSwapchain(vk_physical_device_,
                                             vk_device_,
                                             vk_swapchain_,
                                             vk_present_queue_,
                                             present_queue_index_,
                                             swapchain_interval_)) {
    return DisplayManager::kInit_Swapchain_Failure;
  }
  return DisplayManager::kInit_Swapchain_Success;
}

void GraphicsAPIVulkan::ShutdownSwapchain() {
  vkDeviceWaitIdle(vk_device_);
  PlatformUtilVulkan::DeactivateSwapchain(vk_device_, vk_swapchain_);
  swapchain_info_.swapchain_frame_count_ = 0;
  DestroySwapchain();
}

bool GraphicsAPIVulkan::GetSwapchainValid() {
  return (vk_swapchain_ != VK_NULL_HANDLE);
}

DisplayManager::SwapchainRotationMode GraphicsAPIVulkan::GetSwapchainRotationMode() {
  DisplayManager::SwapchainRotationMode mode = DisplayManager::kSwapchain_Rotation_None;
  if (pretransform_flags_ & VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR) {
    mode = DisplayManager::kSwapchain_Rotation_90;
  } else if (pretransform_flags_ & VK_SURFACE_TRANSFORM_ROTATE_180_BIT_KHR) {
    mode = DisplayManager::kSwapchain_Rotation_180;
  } else if (pretransform_flags_ & VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR) {
    mode = DisplayManager::kSwapchain_Rotation_270;
  }
  return mode;
}

DisplayManager::SwapchainFrameHandle GraphicsAPIVulkan::GetCurrentSwapchainFrame() {
  return kDefault_swapchain_handle;
}

DisplayManager::SwapchainFrameHandle GraphicsAPIVulkan::PresentCurrentSwapchainFrame() {
  if (swapchain_acquired_current_frame_image_) {
    VkSemaphore wait_semaphores[] =
        {frame_render_completion_semaphore_[swapchain_info_.swapchain_current_frame_index_]};

    VkPresentInfoKHR present_info{};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = wait_semaphores;

    VkSwapchainKHR swap_chains[] = {vk_swapchain_};
    present_info.swapchainCount = 1;
    present_info.pSwapchains = swap_chains;
    present_info.pImageIndices = &swapchain_info_.swapchain_current_image_index_;
    present_info.pResults = nullptr;

    VkResult result = PlatformUtilVulkan::PresentSwapchain(vk_present_queue_, &present_info);
    if (result == VK_SUBOPTIMAL_KHR) {
      DebugManager::Log(DebugManager::kLog_Channel_Default,
                        DebugManager::kLog_Level_Warning,
                        BGM_CLASS_TAG,
                        "vkQueuePresentKHR returned VK_SUBOPTIMAL_KHR");
      // Update transform flags and recreate swapchain
      vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vk_physical_device_, vk_surface_,
                                                &surface_capabilities_);
      pretransform_flags_ = surface_capabilities_.currentTransform;
      RecreateSwapchain();
    } else if (result == VK_ERROR_OUT_OF_DATE_KHR) {
      DebugManager::Log(DebugManager::kLog_Channel_Default,
                        DebugManager::kLog_Level_Warning,
                        BGM_CLASS_TAG,
                        "vkQueuePresentKHR returned VK_ERROR_OUT_OF_DATE_KHR");
      RecreateSwapchain();
    } else if (result != VK_SUCCESS) {
      DebugManager::Log(DebugManager::kLog_Channel_Default,
                        DebugManager::kLog_Level_Error,
                        BGM_CLASS_TAG,
                        "vkQueuePresentKHR failed: %d", result);
    }

    swapchain_acquired_current_frame_image_ = false;
    swapchain_info_.swapchain_current_frame_index_ =
        (swapchain_info_.swapchain_current_frame_index_ + 1) %
            swapchain_info_.swapchain_frame_count_;
    swapchain_info_.swapchain_current_depth_stencil_frame_index_ =
        (swapchain_info_.swapchain_current_depth_stencil_frame_index_ + 1) %
            swapchain_info_.swapchain_image_count_;
  } else {
    DebugManager::Log(DebugManager::kLog_Channel_Default,
                      DebugManager::kLog_Level_Error,
                      BGM_CLASS_TAG,
                      "PresentCurrentSwapchainFrame called without valid frame image");
  }

  return kDefault_swapchain_handle;
}

bool GraphicsAPIVulkan::GetGraphicsAPIResourcesVk(GraphicsAPIResourcesVk &api_resources_vk) {
  api_resources_vk.device = vk_device_;
  api_resources_vk.instance = vk_instance_;
  api_resources_vk.render_queue = vk_graphics_queue_;
  api_resources_vk.allocator = allocator_;
  api_resources_vk.graphics_queue_index = graphics_queue_index_;
  api_resources_vk.pretransform_flags = pretransform_flags_;
  return (vk_device_ != VK_NULL_HANDLE);
}

bool GraphicsAPIVulkan::GetSwapchainFrameResourcesVk(
    const DisplayManager::SwapchainFrameHandle frame_handle,
    SwapchainFrameResourcesVk &frame_resources, bool acquire_frame_image) {
  if (frame_handle != DisplayManager::kInvalid_swapchain_handle &&
      vk_device_ != VK_NULL_HANDLE && vk_swapchain_ != VK_NULL_HANDLE) {

    // If we haven't acquired the image for this frame, do it now
    if (!swapchain_acquired_current_frame_image_ && acquire_frame_image) {
      vkWaitForFences(vk_device_, 1,
                      &in_flight_frame_fence_[swapchain_info_.swapchain_current_frame_index_],
                      VK_TRUE, UINT64_MAX);

      VkResult result = vkAcquireNextImageKHR(vk_device_,
                                              vk_swapchain_,
                                              UINT64_MAX,
                                              swapchain_image_semaphore_[swapchain_info_.swapchain_current_frame_index_],
                                              VK_NULL_HANDLE,
                                              &swapchain_info_.swapchain_current_image_index_);
      if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        RecreateSwapchain();

        DebugManager::Log(DebugManager::kLog_Channel_Default,
                          DebugManager::kLog_Level_Warning,
                          BGM_CLASS_TAG,
                          "vkAcquireNextImageKHR returned VK_ERROR_OUT_OF_DATE_KHR");
        return false;
      } else if (!(result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR)) {
        DebugManager::Log(DebugManager::kLog_Channel_Default,
                          DebugManager::kLog_Level_Error,
                          BGM_CLASS_TAG,
                          "vkAcquireNextImageKHR failed: %d", result);
        return false;
      }
      swapchain_acquired_current_frame_image_ = true;

      vkResetFences(vk_device_, 1,
                    &in_flight_frame_fence_[swapchain_info_.swapchain_current_frame_index_]);
    }

    frame_resources.frame_fence =
        in_flight_frame_fence_[swapchain_info_.swapchain_current_frame_index_];
    frame_resources.image_available =
        swapchain_image_semaphore_[swapchain_info_.swapchain_current_frame_index_];
    frame_resources.render_complete =
        frame_render_completion_semaphore_[swapchain_info_.swapchain_current_frame_index_];
    frame_resources.swapchain_color_image_view =
        swapchain_info_.swapchain_image_views_[swapchain_info_.swapchain_current_image_index_];
    frame_resources.swapchain_depth_stencil_image_view =
        swapchain_info_.swapchain_depth_stencil_image_views_[
            swapchain_info_.swapchain_current_depth_stencil_frame_index_];
    frame_resources.swapchain = vk_swapchain_;
    frame_resources.swapchain_color_format = swapchain_info_.swapchain_color_format_;
    frame_resources.swapchain_depth_stencil_format =
        swapchain_info_.swapchain_depth_stencil_format_;
    if (pretransform_flags_ & VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR ||
        pretransform_flags_ & VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR) {
      frame_resources.swapchain_extent.width = swapchain_resolution_.display_height;
      frame_resources.swapchain_extent.height = swapchain_resolution_.display_width;
    } else {
      frame_resources.swapchain_extent.width = swapchain_resolution_.display_width;
      frame_resources.swapchain_extent.height = swapchain_resolution_.display_height;
    }
    frame_resources.swapchain_frame_index = swapchain_info_.swapchain_current_frame_index_;
    frame_resources.swapchain_image_index = swapchain_info_.swapchain_current_image_index_;
    return true;
  }

  return false;
}

bool GraphicsAPIVulkan::SetDisplayChangedCallback(DisplayManager::DisplayChangedCallback callback,
                                                  void *user_data) {
  display_changed_callback_ = callback;
  display_changed_user_data_ = user_data;
  return true;
}

bool GraphicsAPIVulkan::SetSwapchainChangedCallback(
    DisplayManager::SwapchainChangedCallback callback, void *user_data) {
  swapchain_changed_callback_ = callback;
  swapchain_changed_user_data_ = user_data;
  return true;
}

void GraphicsAPIVulkan::SwapchainChanged(const DisplayManager::SwapchainChangeMessage message) {
  if (swapchain_changed_callback_ != nullptr) {
    swapchain_changed_callback_(message, swapchain_changed_user_data_);
  }
}

bool GraphicsAPIVulkan::CreateDevice(bool is_preflight_check,
                                     const QueueFamilyIndices &queue_indices) {
  // Skip validation layers if we are just creating a preflight device to query
  // device extensions and capabilities
  const bool enable_validation = !is_preflight_check && enable_validation_layers_;

  graphics_queue_index_ = queue_indices.graphics_family.value();
  present_queue_index_ = queue_indices.present_family.value();
  // Need a graphics and present queue
  std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
  const float queue_priority = 1.0f;

  queue_create_infos.emplace_back();
  queue_create_infos[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  queue_create_infos[0].pNext = nullptr;
  queue_create_infos[0].flags = 0;
  queue_create_infos[0].queueFamilyIndex = queue_indices.graphics_family.value();
  queue_create_infos[0].queueCount = 1;
  queue_create_infos[0].pQueuePriorities = &queue_priority;

  if (graphics_queue_index_ != present_queue_index_) {
    queue_create_infos.emplace_back();
    queue_create_infos[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_infos[1].pNext = nullptr;
    queue_create_infos[1].flags = 0;
    queue_create_infos[1].queueFamilyIndex =
        queue_indices.present_family.value();
    queue_create_infos[1].queueCount = 1;
    queue_create_infos[1].pQueuePriorities = &queue_priority;
  }

  VkPhysicalDeviceFeatures device_features{};
  if (api_features_.HasGraphicsFeature(GraphicsAPIFeatures::kGraphicsFeature_I16_Math)) {
    device_features.shaderInt16 = VK_TRUE;
  }
  if (api_features_.HasGraphicsFeature(GraphicsAPIFeatures::kGraphicsFeature_Wide_Lines)) {
    device_features.wideLines = VK_TRUE;
  }

  const std::vector<const char *> required_device_extensions =
      PlatformUtilVulkan::GetRequiredDeviceExtensions(vk_physical_device_);

  VkDeviceCreateInfo device_create_info{};
  device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  device_create_info.queueCreateInfoCount =
      static_cast<uint32_t>(queue_create_infos.size());
  device_create_info.pQueueCreateInfos = queue_create_infos.data();
  device_create_info.pEnabledFeatures = &device_features;
  device_create_info.enabledExtensionCount =
      static_cast<uint32_t>(required_device_extensions.size());
  device_create_info.ppEnabledExtensionNames = required_device_extensions.data();

  std::vector<const char *> validation_layers = PlatformUtilVulkan::GetValidationLayers();
  if (enable_validation) {
    device_create_info.enabledLayerCount =
        static_cast<uint32_t>(validation_layers.size());
    device_create_info.ppEnabledLayerNames = validation_layers.data();
  } else {
    device_create_info.enabledLayerCount = 0;
  }

  VulkanAPIUtils::CheckVkResult(
      vkCreateDevice(vk_physical_device_, &device_create_info, nullptr, &vk_device_),
      "vkCreateDevice");

  vkGetDeviceQueue(vk_device_, queue_indices.graphics_family.value(), 0, &vk_graphics_queue_);
  vkGetDeviceQueue(vk_device_, queue_indices.present_family.value(), 0, &vk_present_queue_);

  CreateSynchronizationObjects();

  if (enable_validation) {
    VkDebugUtilsMessengerCreateInfoEXT messenger_create_info{};
    PlatformUtilVulkan::InitMessengerCreateInfo(messenger_create_info);
    messenger_create_info.pfnUserCallback = messengerCallback;
    vkCreateDebugUtilsMessengerEXT(vk_instance_, &messenger_create_info, nullptr,
                                   &debug_messenger_);
  }
  return (vk_device_ != VK_NULL_HANDLE);
}

void GraphicsAPIVulkan::DestroyDevice() {
  vkDeviceWaitIdle(vk_device_);
  DestroySynchronizationObjects();
  vkDestroyDevice(vk_device_, nullptr);
  vk_device_ = VK_NULL_HANDLE;
  vk_graphics_queue_ = VK_NULL_HANDLE;
  vk_present_queue_ = VK_NULL_HANDLE;
}

void GraphicsAPIVulkan::CreateInstance(bool is_preflight_check) {
  // Skip validation layers if we are just creating a preflight instance to query
  // device extensions and capabilities
  const bool request_validation = !is_preflight_check && enable_validation_layers_;

  const std::vector<const char *> instance_extensions =
      PlatformUtilVulkan::GetRequiredInstanceExtensions(request_validation,
                                                        use_physical_device_properties2_);

  VkApplicationInfo app_info{};
  app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  app_info.pApplicationName = "GraphicsAPIVulkan";
  app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  app_info.pEngineName = "BaseGameFramework";
  app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  app_info.apiVersion = PlatformUtilVulkan::GetVulkanApiVersion();

  VkInstanceCreateInfo create_info{};
  create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  create_info.pApplicationInfo = &app_info;
  create_info.enabledExtensionCount = static_cast<uint32_t>(instance_extensions.size());
  create_info.ppEnabledExtensionNames = instance_extensions.data();
  create_info.pApplicationInfo = &app_info;

  const std::vector<const char *> validation_layers =
      PlatformUtilVulkan::GetValidationLayers();
  VkDebugUtilsMessengerCreateInfoEXT messenger_create_info{};

  if (request_validation) {
    create_info.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
    create_info.ppEnabledLayerNames = validation_layers.data();
    PlatformUtilVulkan::InitMessengerCreateInfo(messenger_create_info);
    messenger_create_info.pfnUserCallback = messengerCallback;
    create_info.pNext = &messenger_create_info;
  } else {
    create_info.enabledLayerCount = 0;
    create_info.ppEnabledLayerNames = nullptr;
    create_info.pNext = nullptr;
  }

  const VkResult result = vkCreateInstance(&create_info, nullptr, &vk_instance_);

  if (result == VK_SUCCESS) {
    volkLoadInstance(vk_instance_);
  } else {
    DebugManager::Log(DebugManager::kLog_Channel_Default,
                      DebugManager::kLog_Level_Error,
                      BGM_CLASS_TAG,
                      "vkCreateInstance failed: %d", result);
  }
}

void GraphicsAPIVulkan::DestroyInstance() {
  if (vk_instance_ != nullptr) {
    vkDestroyInstance(vk_instance_, nullptr);
    vk_instance_ = nullptr;
  }
}

bool GraphicsAPIVulkan::CreateSwapchain(const QueueFamilyIndices &queue_indices) {
  uint32_t queue_family_indices[] = {queue_indices.graphics_family.value(),
                                     queue_indices.present_family.value()};
  swapchain_info_.swapchain_color_format_ =
      VulkanAPIUtils::GetVkPixelFormat(swapchain_format_.display_pixel_format);
  swapchain_info_.swapchain_depth_stencil_format_ =
      VulkanAPIUtils::GetVkDepthFormat(swapchain_format_.display_depth_format);

  VkSwapchainCreateInfoKHR swapchain_create_info{};
  swapchain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  swapchain_create_info.surface = vk_surface_;
  swapchain_create_info.minImageCount = swapchain_info_.swapchain_image_count_;
  swapchain_create_info.imageFormat = swapchain_info_.swapchain_color_format_;
  swapchain_create_info.imageColorSpace =
      VulkanAPIUtils::GetVkColorSpace(swapchain_format_.display_color_space);
  if (pretransform_flags_ & VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR ||
      pretransform_flags_ & VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR) {
    // Use the identity resolution, so swap if we are rotated
    swapchain_create_info.imageExtent.width = swapchain_resolution_.display_height;
    swapchain_create_info.imageExtent.height = swapchain_resolution_.display_width;
  } else {
    swapchain_create_info.imageExtent.width = swapchain_resolution_.display_width;
    swapchain_create_info.imageExtent.height = swapchain_resolution_.display_height;
  }
  swapchain_create_info.imageArrayLayers = 1;
  swapchain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
  swapchain_create_info.preTransform = pretransform_flags_;

  if (queue_indices.graphics_family.value() != queue_indices.present_family.value()) {
    swapchain_create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    swapchain_create_info.queueFamilyIndexCount = 2;
    swapchain_create_info.pQueueFamilyIndices = queue_family_indices;
  } else {
    swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchain_create_info.queueFamilyIndexCount = 0;
    swapchain_create_info.pQueueFamilyIndices = nullptr;
  }
  swapchain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
  swapchain_create_info.presentMode = swapchain_info_.swapchain_present_mode_;
  swapchain_create_info.clipped = VK_TRUE;
  swapchain_create_info.oldSwapchain = VK_NULL_HANDLE;

  VulkanAPIUtils::CheckVkResult(
      vkCreateSwapchainKHR(vk_device_, &swapchain_create_info, nullptr, &vk_swapchain_),
      "vkCreateSwapchainKHR");
  return (vk_swapchain_ != VK_NULL_HANDLE);
}

void GraphicsAPIVulkan::RecreateSwapchain() {
  vkDeviceWaitIdle(vk_device_);
  SwapchainChanged(DisplayManager::kSwapchain_Needs_Recreation);
  PlatformUtilVulkan::DeactivateSwapchain(vk_device_, vk_swapchain_);
  DestroySwapchain();

  QueueFamilyIndices queue_indices = VulkanAPIUtils::FindQueueFamilies(
      vk_physical_device_, vk_surface_);

  if (!CreateSwapchain(queue_indices)) {
    DebugManager::Log(DebugManager::kLog_Channel_Default,
                      DebugManager::kLog_Level_Error,
                      BGM_CLASS_TAG,
                      "CreateSwapchain in RecreateSwapchain failed!");
  }

  CreateSwapchainImages();

  if (!PlatformUtilVulkan::ActivateSwapchain(vk_physical_device_,
                                             vk_device_,
                                             vk_swapchain_,
                                             vk_present_queue_,
                                             present_queue_index_,
                                             swapchain_interval_)) {
    DebugManager::Log(DebugManager::kLog_Channel_Default,
                      DebugManager::kLog_Level_Error,
                      BGM_CLASS_TAG,
                      "ActivateSwapchain in RecreateSwapchain failed!");
  }
}

void GraphicsAPIVulkan::DestroySwapchain() {
  DestroySwapchainImages();
  vkDestroySwapchainKHR(vk_device_, vk_swapchain_, nullptr);
  vk_swapchain_ = VK_NULL_HANDLE;
}

void GraphicsAPIVulkan::CreateSwapchainImages() {
  // Get the images and create the image views and framebuffers
  vkGetSwapchainImagesKHR(vk_device_,
                          vk_swapchain_,
                          &swapchain_info_.swapchain_image_count_,
                          nullptr);
  swapchain_info_.swapchain_images_.resize(swapchain_info_.swapchain_image_count_);
  vkGetSwapchainImagesKHR(vk_device_, vk_swapchain_, &swapchain_info_.swapchain_image_count_,
                          swapchain_info_.swapchain_images_.data());

  swapchain_info_.swapchain_image_views_.resize(swapchain_info_.swapchain_image_count_);
  VkImageView *base_view = swapchain_info_.swapchain_image_views_.data();
  const VkFormat pixel_format = VulkanAPIUtils::GetVkPixelFormat(
      swapchain_format_.display_pixel_format);

  for (size_t i = 0; i < swapchain_info_.swapchain_image_count_; ++i) {
    VkImageViewCreateInfo create_info{};
    create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    create_info.image = swapchain_info_.swapchain_images_[i];
    create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    create_info.format = pixel_format;
    create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    create_info.subresourceRange.baseMipLevel = 0;
    create_info.subresourceRange.levelCount = 1;
    create_info.subresourceRange.baseArrayLayer = 0;
    create_info.subresourceRange.layerCount = 1;
    VulkanAPIUtils::CheckVkResult(
        vkCreateImageView(vk_device_, &create_info, nullptr, &base_view[i]),
        "vkCreateImageView");
  }

  if (swapchain_info_.swapchain_depth_stencil_format_ != VK_FORMAT_UNDEFINED) {
    // Create the image/imageviews for the depth/stencil buffer
    swapchain_info_.swapchain_depth_stencil_images_.resize(
        swapchain_info_.swapchain_image_count_);
    swapchain_info_.swapchain_depth_stencil_image_views_.resize(
        swapchain_info_.swapchain_image_count_);
    swapchain_info_.swapchain_depth_stencil_allocs_.resize(
        swapchain_info_.swapchain_image_count_);

    VkImage *swap_images = swapchain_info_.swapchain_depth_stencil_images_.data();
    VkImageView *swap_image_views = swapchain_info_.swapchain_depth_stencil_image_views_.data();
    VmaAllocation *swap_allocs = swapchain_info_.swapchain_depth_stencil_allocs_.data();

    for (size_t i = 0; i < swapchain_info_.swapchain_image_count_; ++i) {
      VkImageCreateInfo ds_image_info = {VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO};
      ds_image_info.imageType = VK_IMAGE_TYPE_2D;
      if (pretransform_flags_ & VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR ||
          pretransform_flags_ & VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR) {
        // Use the identity resolution, so swap if we are rotated
        ds_image_info.extent.width = swapchain_resolution_.display_height;
        ds_image_info.extent.height = swapchain_resolution_.display_width;
      } else {
        ds_image_info.extent.width = swapchain_resolution_.display_width;
        ds_image_info.extent.height = swapchain_resolution_.display_height;
      }
      ds_image_info.extent.depth = 1;
      ds_image_info.mipLevels = 1;
      ds_image_info.arrayLayers = 1;
      ds_image_info.format = swapchain_info_.swapchain_depth_stencil_format_;
      ds_image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
      ds_image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      ds_image_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
      ds_image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
      ds_image_info.samples = VK_SAMPLE_COUNT_1_BIT;
      ds_image_info.flags = 0;

      VmaAllocationCreateInfo ds_image_alloc_create_info = {};
      ds_image_alloc_create_info.usage = VMA_MEMORY_USAGE_AUTO;

      VulkanAPIUtils::CheckVkResult(
          vmaCreateImage(allocator_, &ds_image_info,
                         &ds_image_alloc_create_info,
                         &swap_images[i], &swap_allocs[i], nullptr),
          "vmaCreateImage swap depth/stencil");

      VkImageViewCreateInfo ds_image_view_info = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
      ds_image_view_info.image = swap_images[i];
      ds_image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
      ds_image_view_info.format = swapchain_info_.swapchain_depth_stencil_format_;
      ds_image_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
      ds_image_view_info.subresourceRange.baseMipLevel = 0;
      ds_image_view_info.subresourceRange.levelCount = 1;
      ds_image_view_info.subresourceRange.baseArrayLayer = 0;
      ds_image_view_info.subresourceRange.layerCount = 1;

      VulkanAPIUtils::CheckVkResult(vkCreateImageView(
                                        vk_device_, &ds_image_view_info, nullptr, &swap_image_views[i]),
                                    "vkCreateImageView swap depth/stencil");
    }
  }
}

void GraphicsAPIVulkan::DestroySwapchainImages() {
  if (!swapchain_info_.swapchain_depth_stencil_image_views_.empty()) {
    for (const VkImageView &view : swapchain_info_.swapchain_depth_stencil_image_views_) {
      vkDestroyImageView(vk_device_, view, nullptr);
    }
  }

  if (!swapchain_info_.swapchain_depth_stencil_images_.empty()) {
    for (size_t i = 0; i < swapchain_info_.swapchain_depth_stencil_images_.size(); ++i) {
      vmaDestroyImage(allocator_,
                      swapchain_info_.swapchain_depth_stencil_images_[i],
                      swapchain_info_.swapchain_depth_stencil_allocs_[i]);
    }
  }

  if (!swapchain_info_.swapchain_image_views_.empty()) {
    for (size_t i = 0; i < swapchain_info_.swapchain_image_count_; ++i) {
      vkDestroyImageView(vk_device_, swapchain_info_.swapchain_image_views_[i], nullptr);
    }
  }

  swapchain_info_.swapchain_image_views_.clear();
  swapchain_info_.swapchain_depth_stencil_images_.clear();
  swapchain_info_.swapchain_depth_stencil_image_views_.clear();
  swapchain_info_.swapchain_depth_stencil_allocs_.clear();
}

void GraphicsAPIVulkan::CreateSynchronizationObjects() {
  const size_t in_flight_frame_count = DisplayManager::GetInstance().GetDisplayBufferMode();
  in_flight_frame_fence_.resize(in_flight_frame_count);
  frame_render_completion_semaphore_.resize(in_flight_frame_count);
  swapchain_image_semaphore_.resize(in_flight_frame_count);

  VkSemaphoreCreateInfo semaphore_info{};
  semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fence_info{};
  fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  for (size_t i = 0; i < in_flight_frame_count; ++i) {
    VulkanAPIUtils::CheckVkResult(vkCreateSemaphore(vk_device_, &semaphore_info, nullptr,
                                                    &frame_render_completion_semaphore_[i]),
                                  "vkCreateSemaphore");

    VulkanAPIUtils::CheckVkResult(vkCreateSemaphore(vk_device_, &semaphore_info, nullptr,
                                                    &swapchain_image_semaphore_[i]),
                                  "vkCreateSemaphore");

    VulkanAPIUtils::CheckVkResult(vkCreateFence(vk_device_, &fence_info, nullptr,
                                                &in_flight_frame_fence_[i]),
                                  "vkCreateFence");
  }
}

void GraphicsAPIVulkan::DestroySynchronizationObjects() {
  for (size_t i = 0; i < in_flight_frame_fence_.size(); ++i) {
    vkDestroySemaphore(vk_device_, frame_render_completion_semaphore_[i], nullptr);
    vkDestroySemaphore(vk_device_, swapchain_image_semaphore_[i], nullptr);
    vkDestroyFence(vk_device_, in_flight_frame_fence_[i], nullptr);
  }
  frame_render_completion_semaphore_.clear();
  swapchain_image_semaphore_.clear();
  in_flight_frame_fence_.clear();
}

} // namespace base_game_framework
