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
#include <algorithm>
#include <cstring>
#include <vector>

namespace base_game_framework {

static constexpr DisplayManager::SwapchainFrameHandle kDefault_swapchain_handle = 1;

// The number of in-flight frames for a display buffer mode (anything but triple buffering is
// double buffering, as in RendererVk)
static uint32_t GetInFlightFrameCount(const DisplayManager::DisplayBufferMode buffer_mode) {
  return (buffer_mode == DisplayManager::kDisplay_Triple_Buffer) ? 3 : 2;
}

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
      swapchain_initialized_(false),
      in_flight_frame_fence_(),
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
      use_physical_device_properties2_(false),
      validation_layers_active_(false) {
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
          if (queue_indices.isComplete() && CreateDevice(true, queue_indices)) {
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
  enabled_device_extensions_.clear();
  supported_float16_int8_features_ = {};
  supported_16bit_storage_features_ = {};
  api_features_.SetFeatureBits(0);
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

  // The API version decides whether the reduced precision features are core or need extensions
  const uint32_t api_version = PlatformUtilVulkan::GetVulkanApiVersion();
  DetermineAPILevel(api_version, device_properties.properties.apiVersion);
  const uint32_t active_api_version = api_features_.GetActiveAPIVersion();

  // 16-bit storage is core in Vulkan 1.1. On 1.0 it needs VK_KHR_16bit_storage, which itself
  // requires VK_KHR_storage_buffer_storage_class.
  bool use_16bit_storage = true;
  std::vector<const char *> storage_16bit_extensions;
  if (active_api_version < VK_API_VERSION_1_1) {
    storage_16bit_extensions = {VK_KHR_16BIT_STORAGE_EXTENSION_NAME,
                                VK_KHR_STORAGE_BUFFER_STORAGE_CLASS_EXTENSION_NAME};
    for (const char *extension : storage_16bit_extensions) {
      use_16bit_storage &= VulkanAPIUtils::HasDeviceExtension(available_extensions, extension);
    }
  }
  // Float16/Int8 shader math is core in Vulkan 1.2, before that it needs
  // VK_KHR_shader_float16_int8.
  bool use_float16_int8 = true;
  std::vector<const char *> float16_int8_extensions;
  if (active_api_version < VK_API_VERSION_1_2) {
    float16_int8_extensions = {VK_KHR_SHADER_FLOAT16_INT8_EXTENSION_NAME};
    use_float16_int8 = VulkanAPIUtils::HasDeviceExtension(available_extensions,
                                                          VK_KHR_SHADER_FLOAT16_INT8_EXTENSION_NAME);
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
  void *feature_chain = nullptr;
  if (use_float16_int8) {
    shader_float_16_int_8_features.pNext = feature_chain;
    feature_chain = &shader_float_16_int_8_features;
  }
  if (use_16bit_storage) {
    device_16_bit_storage_features.pNext = feature_chain;
    feature_chain = &device_16_bit_storage_features;
  }
  device_features.pNext = feature_chain;

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

  // Determine if the device has ETC2 texture support
  if (device_features.features.textureCompressionETC2) {
      feature_flags_ |= DisplayManager::kVulkan_ETC2_Support;
  }

  // Ignore the feature structs entirely if they report no supported features, so
  // CreateDevice doesn't chain them or enable their extensions for nothing
  shader_float_16_int_8_features.pNext = nullptr;
  device_16_bit_storage_features.pNext = nullptr;
  if (!use_float16_int8 || !(shader_float_16_int_8_features.shaderFloat16 ||
                             shader_float_16_int_8_features.shaderInt8)) {
    shader_float_16_int_8_features = {};
    use_float16_int8 = false;
  }
  if (!use_16bit_storage || !(device_16_bit_storage_features.storageBuffer16BitAccess ||
                              device_16_bit_storage_features.uniformAndStorageBuffer16BitAccess ||
                              device_16_bit_storage_features.storagePushConstant16 ||
                              device_16_bit_storage_features.storageInputOutput16)) {
    device_16_bit_storage_features = {};
    use_16bit_storage = false;
  }

  DetermineNumericSupport(device_features.features.shaderInt16,
                          shader_float_16_int_8_features, device_16_bit_storage_features);

  // Keep the supported feature bits (and any extensions they need) so CreateDevice can enable
  // what we advertise
  if (use_float16_int8) {
    supported_float16_int8_features_ = shader_float_16_int_8_features;
    enabled_device_extensions_.insert(enabled_device_extensions_.end(),
                                      float16_int8_extensions.begin(),
                                      float16_int8_extensions.end());
  }
  if (use_16bit_storage) {
    supported_16bit_storage_features_ = device_16_bit_storage_features;
    enabled_device_extensions_.insert(enabled_device_extensions_.end(),
                                      storage_16bit_extensions.begin(),
                                      storage_16bit_extensions.end());
  }
  // TODO: Android Baseline Profile detection
}

void GraphicsAPIVulkan::QuerySurfaceCapabilities() {
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vk_physical_device_, vk_surface_,
                                            &surface_capabilities_);
  swapchain_min_frames_ = surface_capabilities_.minImageCount;
  swapchain_max_frames_ = surface_capabilities_.maxImageCount;
  swapchain_info_.swapchain_image_count_ = swapchain_min_frames_;
  pretransform_flags_ = surface_capabilities_.currentTransform;

  swapchain_present_modes_ = 0;
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
  // Use validation layers for this API session only if requested and available; don't
  // overwrite the caller's request, so a later session checks availability again
  validation_layers_active_ = false;
  if (GraphicsAPIVulkan::enable_validation_layers_) {
    validation_layers_active_ = PlatformUtilVulkan::GetValidationLayersAvailable();
    if (!validation_layers_active_) {
      DebugManager::Log(DebugManager::kLog_Channel_Default,
                        DebugManager::kLog_Level_Error,
                        BGM_CLASS_TAG,
                        "Vulkan validation layers were requested but unavailable!");
    }
  }
  CreateInstance(false);
  if (vk_instance_ != VK_NULL_HANDLE) {
    vk_surface_ = PlatformUtilVulkan::CreateSurface(vk_instance_);
    if (vk_surface_ != VK_NULL_HANDLE) {
      vk_physical_device_ = GetPhysicalDevice();
      if (vk_physical_device_ != VK_NULL_HANDLE) {
        // QueryCapabilities used a temporary instance, so query the capabilities again on
        // this instance's physical device; these decide what CreateDevice enables
        QueryDeviceCapabilities(vk_physical_device_);
        QueueFamilyIndices queue_indices = VulkanAPIUtils::FindQueueFamilies(
            vk_physical_device_, vk_surface_);
        if (queue_indices.isComplete() && CreateDevice(false, queue_indices)) {
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
          if (vma_result == VK_SUCCESS) {
            // The swapchain configurations come from this instance's surface: the one
            // QueryCapabilities used was temporary, and the window may have changed since
            // (e.g. rotated or resized)
            QuerySurfaceCapabilities();
            VulkanAPIUtils::GetDisplayFormats(vk_physical_device_, vk_surface_,
                                              display_formats_);
            PlatformUtilVulkan::GetScreenResolutions(surface_capabilities_,
                                                     display_resolutions_);
            api_status_ = kGraphicsAPI_Active;
            return true;
          }
          allocator_ = VK_NULL_HANDLE;
          DebugManager::Log(DebugManager::kLog_Channel_Default,
                            DebugManager::kLog_Level_Error,
                            BGM_CLASS_TAG,
                            "vmaCreateAllocator failed: %d", vma_result);
        }
      }
    }
  }
  // Release whatever was created before the failure
  ShutdownGraphicsAPI();
  return false;
}

void GraphicsAPIVulkan::ShutdownGraphicsAPI() {
  swapchain_initialized_ = false;
  TeardownSwapchain();
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

  // The physical device handle belongs to the instance
  vk_physical_device_ = VK_NULL_HANDLE;
  DestroyInstance();
  validation_layers_active_ = false;
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

  // One swapchain at a time: a second InitSwapchain would leak the first (shut it down first)
  if (swapchain_initialized_) {
    DebugManager::Log(DebugManager::kLog_Channel_Default,
                      DebugManager::kLog_Level_Error,
                      BGM_CLASS_TAG,
                      "InitSwapchain called with a swapchain already initialized");
    return DisplayManager::kInit_Swapchain_Failure;
  }

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

  // A maxImageCount of 0 means the surface has no upper limit on image count
  if (swapchain_frame_count < swapchain_min_frames_ ||
      (swapchain_max_frames_ != 0 && swapchain_frame_count > swapchain_max_frames_)) {
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

  swapchain_format_ = display_format;
  swapchain_resolution_ = display_resolution;
  swapchain_interval_ = display_swap_interval;
  swapchain_info_.swapchain_present_mode_ = kPresentModes[present_mode];
  swapchain_info_.swapchain_requested_image_count_ = swapchain_frame_count;
  // Fixed for the swapchain's lifetime (also when it is recreated), sizing the per-frame
  // synchronization objects
  swapchain_info_.swapchain_frame_count_ =
      GetInFlightFrameCount(DisplayManager::GetInstance().GetDisplayBufferMode());

  if (!BuildSwapchain()) {
    return DisplayManager::kInit_Swapchain_Failure;
  }
  swapchain_initialized_ = true;
  return DisplayManager::kInit_Swapchain_Success;
}

void GraphicsAPIVulkan::ShutdownSwapchain() {
  // Also when the window is lost (and the swapchain with it), so it isn't recreated when a
  // window is gained
  swapchain_initialized_ = false;
  TeardownSwapchain();
  swapchain_info_.swapchain_frame_count_ = 0;
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
    const uint32_t image_index = swapchain_info_.swapchain_current_image_index_;
    VkSemaphore wait_semaphores[] =
        {swapchain_info_.swapchain_render_complete_semaphores_[image_index]};

    VkPresentInfoKHR present_info{};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = wait_semaphores;

    VkSwapchainKHR swap_chains[] = {vk_swapchain_};
    present_info.swapchainCount = 1;
    present_info.pSwapchains = swap_chains;
    present_info.pImageIndices = &image_index;
    present_info.pResults = nullptr;

    VkResult result = PlatformUtilVulkan::PresentSwapchain(vk_present_queue_, &present_info);
    // The image is handed to the presentation engine even if the swapchain is suboptimal or
    // out of date
    swapchain_acquired_current_frame_image_ = false;
    if (result == VK_SUBOPTIMAL_KHR) {
      DebugManager::Log(DebugManager::kLog_Channel_Default,
                        DebugManager::kLog_Level_Warning,
                        BGM_CLASS_TAG,
                        "vkQueuePresentKHR returned VK_SUBOPTIMAL_KHR");
      // e.g. the surface transform changed; recreate for the current surface properties
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

    // The swapchain may have been recreated or shut down by a callback above
    if (swapchain_info_.swapchain_frame_count_ != 0) {
      swapchain_info_.swapchain_current_frame_index_ =
          (swapchain_info_.swapchain_current_frame_index_ + 1) %
              swapchain_info_.swapchain_frame_count_;
    }
  } else {
    // e.g. the frame's image couldn't be acquired because the swapchain was out of date
    DebugManager::Log(DebugManager::kLog_Channel_Default,
                      DebugManager::kLog_Level_Warning,
                      BGM_CLASS_TAG,
                      "PresentCurrentSwapchainFrame: no frame image was acquired, not presenting");
  }

  return kDefault_swapchain_handle;
}

bool GraphicsAPIVulkan::GetGraphicsAPIResourcesVk(GraphicsAPIResourcesVk &api_resources_vk) {
  api_resources_vk.device = vk_device_;
  api_resources_vk.instance = vk_instance_;
  api_resources_vk.physical_device = vk_physical_device_;
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
      vk_device_ != VK_NULL_HANDLE && vk_swapchain_ != VK_NULL_HANDLE &&
      swapchain_info_.swapchain_current_frame_index_ < in_flight_frame_fence_.size()) {

    if (acquire_frame_image && swapchain_acquired_current_frame_image_) {
      // This frame's image was already acquired (and maybe submitted) without being presented:
      // acquiring again would skip the wait for the frame fence, so the caller could reuse the
      // frame's resources while the GPU still uses them
      DebugManager::Log(DebugManager::kLog_Channel_Default,
                        DebugManager::kLog_Level_Error,
                        BGM_CLASS_TAG,
                        "GetSwapchainFrameResourcesVk: the current frame's image was already "
                        "acquired, present it first");
      return false;
    }

    // If we haven't acquired the image for this frame, do it now
    if (acquire_frame_image) {
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
    // Per swapchain image, not per frame (see swapchain_render_complete_semaphores_)
    const uint32_t image_index = swapchain_info_.swapchain_current_image_index_;
    const std::vector<VkSemaphore> &render_complete_semaphores =
        swapchain_info_.swapchain_render_complete_semaphores_;
    frame_resources.render_complete =
        (image_index < render_complete_semaphores.size()) ?
        render_complete_semaphores[image_index] : VK_NULL_HANDLE;
    frame_resources.swapchain_color_image_view =
        (image_index < swapchain_info_.swapchain_image_views_.size()) ?
        swapchain_info_.swapchain_image_views_[image_index] : VK_NULL_HANDLE;
    // Each swapchain image has its own depth/stencil image, so a depth/stencil image is never
    // shared by two frames in flight. None exist if the swapchain format has no depth or
    // stencil.
    const std::vector<VkImageView> &depth_stencil_views =
        swapchain_info_.swapchain_depth_stencil_image_views_;
    frame_resources.swapchain_depth_stencil_image_view =
        (image_index < depth_stencil_views.size()) ? depth_stencil_views[image_index] :
                                                     VK_NULL_HANDLE;
    frame_resources.swapchain = vk_swapchain_;
    frame_resources.swapchain_color_format = swapchain_info_.swapchain_color_format_;
    frame_resources.swapchain_depth_stencil_format =
        swapchain_info_.swapchain_depth_stencil_format_;
    frame_resources.swapchain_extent = swapchain_info_.swapchain_extent_;
    frame_resources.swapchain_frame_index = swapchain_info_.swapchain_current_frame_index_;
    frame_resources.swapchain_image_index = swapchain_info_.swapchain_current_image_index_;
    frame_resources.swapchain_frame_count = swapchain_info_.swapchain_frame_count_;
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
  if (message == DisplayManager::kSwapchain_Lost_Window) {
    // The window's surface goes away, and a swapchain must be destroyed before its surface
    // (VUID-vkDestroySurfaceKHR-surface-01266). It is recreated when a window is gained.
    const bool had_swapchain = (vk_swapchain_ != VK_NULL_HANDLE);
    TeardownSwapchain();
    if (vk_surface_ != VK_NULL_HANDLE) {
      PlatformUtilVulkan::DestroySurface(vk_instance_, vk_surface_);
      vk_surface_ = VK_NULL_HANDLE;
    }
    if (had_swapchain) {
      // Anything referring to the destroyed swapchain's images is invalid
      NotifySwapchainChanged(DisplayManager::kSwapchain_Needs_Recreation);
    }
  } else if (message == DisplayManager::kSwapchain_Gained_Window) {
    if (vk_surface_ == VK_NULL_HANDLE && vk_instance_ != VK_NULL_HANDLE) {
      vk_surface_ = PlatformUtilVulkan::CreateSurface(vk_instance_);
    }
    if (swapchain_initialized_ && vk_surface_ != VK_NULL_HANDLE) {
      RecreateSwapchain();
    }
  }
  NotifySwapchainChanged(message);
}

void GraphicsAPIVulkan::NotifySwapchainChanged(
    const DisplayManager::SwapchainChangeMessage message) {
  if (swapchain_changed_callback_ != nullptr) {
    swapchain_changed_callback_(message, swapchain_changed_user_data_);
  }
}

void GraphicsAPIVulkan::HandleWindowResized() {
  if (!swapchain_initialized_ || vk_surface_ == VK_NULL_HANDLE) {
    return;
  }
  if (vk_swapchain_ == VK_NULL_HANDLE) {
    // The swapchain couldn't be recreated before (e.g. the window had no size), try again
    RecreateSwapchain();
    return;
  }
  // The swapchain keeps its size when the window is resized (the image is scaled to the
  // window), so compare it with the surface's current size and transform
  VkSurfaceCapabilitiesKHR capabilities{};
  if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vk_physical_device_, vk_surface_,
                                                &capabilities) != VK_SUCCESS) {
    return;
  }
  const VkExtent2D &extent = capabilities.currentExtent;
  bool size_changed = false;
  if (extent.width == 0 || extent.height == 0) {
    // No size: recreating tears the swapchain down until the window has a size again
    size_changed = true;
  } else if (extent.width != UINT32_MAX) {
    // (0xFFFFFFFF: the swapchain decides the size, so there is nothing to follow)
    std::vector<DisplayManager::DisplayResolution> resolutions;
    PlatformUtilVulkan::GetScreenResolutions(capabilities, resolutions);
    size_changed = !resolutions.empty() &&
        (resolutions[0].display_width != swapchain_resolution_.display_width ||
         resolutions[0].display_height != swapchain_resolution_.display_height);
  }
  if (size_changed || capabilities.currentTransform != pretransform_flags_) {
    RecreateSwapchain();
  }
}

bool GraphicsAPIVulkan::CreateDevice(bool is_preflight_check,
                                     const QueueFamilyIndices &queue_indices) {
  // Skip validation layers if we are just creating a preflight device to query
  // device extensions and capabilities
  const bool enable_validation = !is_preflight_check && validation_layers_active_;

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

  std::vector<const char *> device_extensions =
      PlatformUtilVulkan::GetRequiredDeviceExtensions(vk_physical_device_);

  VkDeviceCreateInfo device_create_info{};
  device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

  // The real device enables the optional extensions QueryDeviceCapabilities found, plus the
  // supported features of each, so features advertised through api_features_ (F16/I8 math,
  // 16-bit storage) are actually usable. The preflight device only needs the basics.
  VkPhysicalDeviceShaderFloat16Int8FeaturesKHR float16_int8_features{};
  VkPhysicalDevice16BitStorageFeaturesKHR storage_16bit_features{};
  if (!is_preflight_check) {
    for (const char *extension : enabled_device_extensions_) {
      bool already_enabled = false;
      for (const char *enabled : device_extensions) {
        if (strcmp(enabled, extension) == 0) {
          already_enabled = true;
          break;
        }
      }
      if (!already_enabled) {
        device_extensions.push_back(extension);
      }
    }
    void *feature_chain = nullptr;
    if (supported_float16_int8_features_.sType != 0) {
      float16_int8_features = supported_float16_int8_features_;
      float16_int8_features.pNext = feature_chain;
      feature_chain = &float16_int8_features;
    }
    if (supported_16bit_storage_features_.sType != 0) {
      storage_16bit_features = supported_16bit_storage_features_;
      storage_16bit_features.pNext = feature_chain;
      feature_chain = &storage_16bit_features;
    }
    device_create_info.pNext = feature_chain;
  }

  device_create_info.queueCreateInfoCount =
      static_cast<uint32_t>(queue_create_infos.size());
  device_create_info.pQueueCreateInfos = queue_create_infos.data();
  device_create_info.pEnabledFeatures = &device_features;
  device_create_info.enabledExtensionCount = static_cast<uint32_t>(device_extensions.size());
  device_create_info.ppEnabledExtensionNames = device_extensions.data();
  // Device layers are deprecated and must not be set
  // (VUID-VkDeviceCreateInfo-enabledLayerCount-12384); the instance layers enabled in
  // CreateInstance validate device calls too
  device_create_info.enabledLayerCount = 0;
  device_create_info.ppEnabledLayerNames = nullptr;

  if (!VulkanAPIUtils::CheckVkResult(
      vkCreateDevice(vk_physical_device_, &device_create_info, nullptr, &vk_device_),
      "vkCreateDevice")) {
    vk_device_ = VK_NULL_HANDLE;
    return false;
  }

  vkGetDeviceQueue(vk_device_, queue_indices.graphics_family.value(), 0, &vk_graphics_queue_);
  vkGetDeviceQueue(vk_device_, queue_indices.present_family.value(), 0, &vk_present_queue_);

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
  // Swappy's device state (finding #22: it was never released, for the preflight device
  // created by QueryCapabilities or the real one)
  PlatformUtilVulkan::DeactivateDevice(vk_device_);
  vkDestroyDevice(vk_device_, nullptr);
  vk_device_ = VK_NULL_HANDLE;
  vk_graphics_queue_ = VK_NULL_HANDLE;
  vk_present_queue_ = VK_NULL_HANDLE;
}

void GraphicsAPIVulkan::CreateInstance(bool is_preflight_check) {
  // Skip validation layers if we are just creating a preflight instance to query
  // device extensions and capabilities
  const bool request_validation = !is_preflight_check && validation_layers_active_;

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
      VulkanAPIUtils::GetVkPixelFormat(swapchain_format_.display_pixel_format,
                                       swapchain_format_.display_color_space);
  swapchain_info_.swapchain_depth_stencil_format_ =
      VulkanAPIUtils::GetVkDepthStencilFormat(swapchain_format_.display_depth_format,
                                              swapchain_format_.display_stencil_format);

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
  // Within the surface's limits (VUID-VkSwapchainCreateInfoKHR-imageExtent-01274), when it
  // reports them
  const VkExtent2D &min_extent = surface_capabilities_.minImageExtent;
  const VkExtent2D &max_extent = surface_capabilities_.maxImageExtent;
  if (max_extent.width > 0 && max_extent.height > 0) {
    VkExtent2D &extent = swapchain_create_info.imageExtent;
    extent.width = std::min(std::max(extent.width, min_extent.width), max_extent.width);
    extent.height = std::min(std::max(extent.height, min_extent.height), max_extent.height);
  }
  swapchain_info_.swapchain_extent_ = swapchain_create_info.imageExtent;
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

bool GraphicsAPIVulkan::BuildSwapchain() {
  if (vk_device_ == VK_NULL_HANDLE || vk_surface_ == VK_NULL_HANDLE) {
    return false;
  }
  // The device's queues must be able to present to this surface (a new surface, e.g. after
  // the window was lost and gained, might not be supported)
  VkBool32 present_supported = VK_FALSE;
  vkGetPhysicalDeviceSurfaceSupportKHR(vk_physical_device_, present_queue_index_, vk_surface_,
                                       &present_supported);
  if (present_supported != VK_TRUE) {
    DebugManager::Log(DebugManager::kLog_Channel_Default,
                      DebugManager::kLog_Level_Error,
                      BGM_CLASS_TAG,
                      "BuildSwapchain: the present queue can't present to the surface");
    return false;
  }
  QueueFamilyIndices queue_indices;
  queue_indices.graphics_family = graphics_queue_index_;
  queue_indices.present_family = present_queue_index_;
  // Request the image count InitSwapchain asked for, not the count the previous swapchain
  // ended up with (which may be larger), within the surface's current limits
  // (VUID-VkSwapchainCreateInfoKHR-minImageCount-01271/-01272; a max of 0 is no limit)
  uint32_t image_count = std::max(swapchain_info_.swapchain_requested_image_count_,
                                  swapchain_min_frames_);
  if (swapchain_max_frames_ != 0) {
    image_count = std::min(image_count, swapchain_max_frames_);
  }
  swapchain_info_.swapchain_image_count_ = image_count;

  if (!CreateSwapchain(queue_indices)) {
    return false;
  }

  if (!CreateSwapchainImages()) {
    DestroySwapchain();
    return false;
  }

  if (!PlatformUtilVulkan::ActivateSwapchain(vk_physical_device_,
                                             vk_device_,
                                             vk_swapchain_,
                                             vk_present_queue_,
                                             present_queue_index_,
                                             swapchain_interval_)) {
    // Don't leave a half initialized swapchain that GetSwapchainValid reports as valid
    PlatformUtilVulkan::DeactivateSwapchain(vk_device_, vk_swapchain_);
    DestroySwapchain();
    return false;
  }
  return true;
}

void GraphicsAPIVulkan::TeardownSwapchain() {
  if (vk_swapchain_ == VK_NULL_HANDLE) {
    return;
  }
  // Nothing may still use the swapchain images, or the semaphores destroyed with them
  vkDeviceWaitIdle(vk_device_);
  PlatformUtilVulkan::DeactivateSwapchain(vk_device_, vk_swapchain_);
  DestroySwapchain();
}

bool GraphicsAPIVulkan::UpdateSurfaceProperties(bool *resolution_changed) {
  *resolution_changed = false;
  if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vk_physical_device_, vk_surface_,
                                                &surface_capabilities_) != VK_SUCCESS) {
    return false;
  }
  pretransform_flags_ = surface_capabilities_.currentTransform;
  swapchain_min_frames_ = surface_capabilities_.minImageCount;
  swapchain_max_frames_ = surface_capabilities_.maxImageCount;
  const VkExtent2D &extent = surface_capabilities_.currentExtent;
  if (extent.width == 0 || extent.height == 0) {
    // The window has no size (e.g. minimized): no swapchain can be created for it
    return false;
  }
  if (extent.width == UINT32_MAX) {
    // The swapchain decides the size: keep the current resolution
    return true;
  }
  // The same resolution GetSwapchainConfigurations offers for the surface
  PlatformUtilVulkan::GetScreenResolutions(surface_capabilities_, display_resolutions_);
  if (!display_resolutions_.empty()) {
    *resolution_changed = !(display_resolutions_[0] == swapchain_resolution_);
    swapchain_resolution_ = display_resolutions_[0];
  }
  return true;
}

void GraphicsAPIVulkan::RecreateSwapchain() {
  const bool had_swapchain = (vk_swapchain_ != VK_NULL_HANDLE);
  TeardownSwapchain();

  // Recreate for the surface's current size and transform (e.g. after a resize or rotation).
  // Without a window, or one that can't have a swapchain right now, it is recreated when a
  // window is gained or resized.
  bool resolution_changed = false;
  bool built = false;
  if (vk_surface_ != VK_NULL_HANDLE) {
    if (UpdateSurfaceProperties(&resolution_changed)) {
      built = BuildSwapchain();
      if (!built) {
        DebugManager::Log(DebugManager::kLog_Channel_Default,
                          DebugManager::kLog_Level_Error,
                          BGM_CLASS_TAG,
                          "RecreateSwapchain failed to create the swapchain!");
      }
    } else {
      DebugManager::Log(DebugManager::kLog_Channel_Default,
                        DebugManager::kLog_Level_Warning,
                        BGM_CLASS_TAG,
                        "RecreateSwapchain: the surface can't have a swapchain right now");
    }
  }

  // Notify once the new swapchain exists: previously the swapchain changed callback ran
  // before the old swapchain was even destroyed. Also if the old swapchain is gone and no new
  // one could be created, as anything referring to its images is invalid.
  if (resolution_changed && display_changed_callback_ != nullptr) {
    DisplayManager::DisplayChangeInfo change_info(swapchain_resolution_,
                                                  DisplayManager::kDisplay_Change_Window_Resized);
    display_changed_callback_(change_info, display_changed_user_data_);
  }
  if (had_swapchain || built) {
    NotifySwapchainChanged(DisplayManager::kSwapchain_Needs_Recreation);
  }
}

void GraphicsAPIVulkan::DestroySwapchain() {
  DestroySwapchainImages();
  vkDestroySwapchainKHR(vk_device_, vk_swapchain_, nullptr);
  vk_swapchain_ = VK_NULL_HANDLE;
  swapchain_acquired_current_frame_image_ = false;
}

bool GraphicsAPIVulkan::CreateSwapchainImages() {
  // A new set of images: no image is acquired yet, and the image count may differ from the
  // previous swapchain, so don't keep an index into the old image set
  swapchain_info_.swapchain_current_image_index_ = 0;
  swapchain_acquired_current_frame_image_ = false;

  // Get the images and create the image views and framebuffers
  uint32_t image_count = 0;
  if (!VulkanAPIUtils::CheckVkResult(
      vkGetSwapchainImagesKHR(vk_device_, vk_swapchain_, &image_count, nullptr),
      "vkGetSwapchainImagesKHR")) {
    return false;
  }
  swapchain_info_.swapchain_images_.resize(image_count);
  if (!VulkanAPIUtils::CheckVkResult(
      vkGetSwapchainImagesKHR(vk_device_, vk_swapchain_, &image_count,
                              swapchain_info_.swapchain_images_.data()),
      "vkGetSwapchainImagesKHR")) {
    swapchain_info_.swapchain_images_.clear();
    return false;
  }
  swapchain_info_.swapchain_image_count_ = image_count;

  // Render complete semaphores, one per image. Ones that fail to create stay VK_NULL_HANDLE.
  swapchain_info_.swapchain_render_complete_semaphores_.resize(image_count, VK_NULL_HANDLE);
  VkSemaphoreCreateInfo semaphore_info{};
  semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
  for (size_t i = 0; i < image_count; ++i) {
    if (!VulkanAPIUtils::CheckVkResult(
        vkCreateSemaphore(vk_device_, &semaphore_info, nullptr,
                          &swapchain_info_.swapchain_render_complete_semaphores_[i]),
        "vkCreateSemaphore")) {
      swapchain_info_.swapchain_render_complete_semaphores_[i] = VK_NULL_HANDLE;
      DestroySwapchainImages();
      return false;
    }
  }

  // Views/images that fail to create stay VK_NULL_HANDLE, which DestroySwapchainImages skips
  swapchain_info_.swapchain_image_views_.resize(image_count, VK_NULL_HANDLE);
  const VkFormat pixel_format = VulkanAPIUtils::GetVkPixelFormat(
      swapchain_format_.display_pixel_format, swapchain_format_.display_color_space);

  for (size_t i = 0; i < image_count; ++i) {
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
    if (!VulkanAPIUtils::CheckVkResult(
        vkCreateImageView(vk_device_, &create_info, nullptr,
                          &swapchain_info_.swapchain_image_views_[i]),
        "vkCreateImageView")) {
      swapchain_info_.swapchain_image_views_[i] = VK_NULL_HANDLE;
      DestroySwapchainImages();
      return false;
    }
  }

  if (swapchain_info_.swapchain_depth_stencil_format_ != VK_FORMAT_UNDEFINED) {
    // Create one depth/stencil image/imageview per swapchain image, used with that image
    swapchain_info_.swapchain_depth_stencil_images_.resize(image_count, VK_NULL_HANDLE);
    swapchain_info_.swapchain_depth_stencil_image_views_.resize(image_count, VK_NULL_HANDLE);
    swapchain_info_.swapchain_depth_stencil_allocs_.resize(image_count, VK_NULL_HANDLE);

    for (size_t i = 0; i < image_count; ++i) {
      VkImageCreateInfo ds_image_info{};
      ds_image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
      ds_image_info.imageType = VK_IMAGE_TYPE_2D;
      // The same (pre-rotated) extent as the swapchain images
      ds_image_info.extent.width = swapchain_info_.swapchain_extent_.width;
      ds_image_info.extent.height = swapchain_info_.swapchain_extent_.height;
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

      if (!VulkanAPIUtils::CheckVkResult(
          vmaCreateImage(allocator_, &ds_image_info, &ds_image_alloc_create_info,
                         &swapchain_info_.swapchain_depth_stencil_images_[i],
                         &swapchain_info_.swapchain_depth_stencil_allocs_[i], nullptr),
          "vmaCreateImage swap depth/stencil")) {
        swapchain_info_.swapchain_depth_stencil_images_[i] = VK_NULL_HANDLE;
        swapchain_info_.swapchain_depth_stencil_allocs_[i] = VK_NULL_HANDLE;
        DestroySwapchainImages();
        return false;
      }

      VkImageViewCreateInfo ds_image_view_info{};
      ds_image_view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
      ds_image_view_info.image = swapchain_info_.swapchain_depth_stencil_images_[i];
      ds_image_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
      ds_image_view_info.format = swapchain_info_.swapchain_depth_stencil_format_;
      ds_image_view_info.subresourceRange.aspectMask =
          VulkanAPIUtils::GetVkDepthStencilAspectMask(
              swapchain_info_.swapchain_depth_stencil_format_);
      ds_image_view_info.subresourceRange.baseMipLevel = 0;
      ds_image_view_info.subresourceRange.levelCount = 1;
      ds_image_view_info.subresourceRange.baseArrayLayer = 0;
      ds_image_view_info.subresourceRange.layerCount = 1;

      if (!VulkanAPIUtils::CheckVkResult(
          vkCreateImageView(vk_device_, &ds_image_view_info, nullptr,
                            &swapchain_info_.swapchain_depth_stencil_image_views_[i]),
          "vkCreateImageView swap depth/stencil")) {
        swapchain_info_.swapchain_depth_stencil_image_views_[i] = VK_NULL_HANDLE;
        DestroySwapchainImages();
        return false;
      }
    }
  }

  if (!CreateSynchronizationObjects()) {
    DestroySwapchainImages();
    return false;
  }
  return true;
}

void GraphicsAPIVulkan::DestroySwapchainImages() {
  // Safe after a partial CreateSwapchainImages: anything not created is VK_NULL_HANDLE
  for (const VkImageView view : swapchain_info_.swapchain_depth_stencil_image_views_) {
    if (view != VK_NULL_HANDLE) {
      vkDestroyImageView(vk_device_, view, nullptr);
    }
  }

  for (size_t i = 0; i < swapchain_info_.swapchain_depth_stencil_images_.size(); ++i) {
    if (swapchain_info_.swapchain_depth_stencil_images_[i] != VK_NULL_HANDLE) {
      vmaDestroyImage(allocator_,
                      swapchain_info_.swapchain_depth_stencil_images_[i],
                      swapchain_info_.swapchain_depth_stencil_allocs_[i]);
    }
  }

  for (const VkImageView view : swapchain_info_.swapchain_image_views_) {
    if (view != VK_NULL_HANDLE) {
      vkDestroyImageView(vk_device_, view, nullptr);
    }
  }

  for (const VkSemaphore semaphore : swapchain_info_.swapchain_render_complete_semaphores_) {
    if (semaphore != VK_NULL_HANDLE) {
      vkDestroySemaphore(vk_device_, semaphore, nullptr);
    }
  }

  // The swapchain images themselves are owned by the swapchain
  swapchain_info_.swapchain_images_.clear();
  swapchain_info_.swapchain_image_views_.clear();
  swapchain_info_.swapchain_render_complete_semaphores_.clear();
  swapchain_info_.swapchain_depth_stencil_images_.clear();
  swapchain_info_.swapchain_depth_stencil_image_views_.clear();
  swapchain_info_.swapchain_depth_stencil_allocs_.clear();

  DestroySynchronizationObjects();
}

bool GraphicsAPIVulkan::CreateSynchronizationObjects() {
  // Per in-flight frame (the render complete semaphores are per swapchain image). Created
  // with each swapchain, so a swapchain never inherits a fence left unsignaled or an image
  // available semaphore left signaled by a frame that acquired an image but wasn't submitted.
  swapchain_info_.swapchain_current_frame_index_ = 0;
  const uint32_t in_flight_frame_count = swapchain_info_.swapchain_frame_count_;
  if (in_flight_frame_count == 0) {
    return false;
  }
  // Ones that fail to create stay VK_NULL_HANDLE, which DestroySynchronizationObjects skips
  in_flight_frame_fence_.assign(in_flight_frame_count, VK_NULL_HANDLE);
  swapchain_image_semaphore_.assign(in_flight_frame_count, VK_NULL_HANDLE);

  VkSemaphoreCreateInfo semaphore_info{};
  semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fence_info{};
  fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  for (uint32_t i = 0; i < in_flight_frame_count; ++i) {
    if (!VulkanAPIUtils::CheckVkResult(vkCreateSemaphore(vk_device_, &semaphore_info, nullptr,
                                                         &swapchain_image_semaphore_[i]),
                                       "vkCreateSemaphore") ||
        !VulkanAPIUtils::CheckVkResult(vkCreateFence(vk_device_, &fence_info, nullptr,
                                                     &in_flight_frame_fence_[i]),
                                       "vkCreateFence")) {
      return false;
    }
  }
  return true;
}

void GraphicsAPIVulkan::DestroySynchronizationObjects() {
  for (const VkSemaphore semaphore : swapchain_image_semaphore_) {
    if (semaphore != VK_NULL_HANDLE) {
      vkDestroySemaphore(vk_device_, semaphore, nullptr);
    }
  }
  for (const VkFence fence : in_flight_frame_fence_) {
    if (fence != VK_NULL_HANDLE) {
      vkDestroyFence(vk_device_, fence, nullptr);
    }
  }
  swapchain_image_semaphore_.clear();
  in_flight_frame_fence_.clear();
}

} // namespace base_game_framework
