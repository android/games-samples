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

#ifndef BASEGAMEFRAMEWORK_DISPLAYMANAGER_H_
#define BASEGAMEFRAMEWORK_DISPLAYMANAGER_H_

#include <cstdint>
#include <functional>
#include <memory>
#include <vector>
#include "platform_defines.h"
#include "graphics_api_features.h"

namespace base_game_framework {

// Forward declarations to avoid including the API specific headers and the
// corresponding API headers
class GraphicsAPIBase;
struct PlatformDisplayChange;
#if defined BGF_DISPLAY_MANAGER_GLES
class GraphicsAPIGLES;
struct GraphicsAPIResourcesGLES;
struct SwapchainFrameResourcesGLES;
#endif
#if defined BGF_DISPLAY_MANAGER_VULKAN
class GraphicsAPIVulkan;
struct GraphicsAPIResourcesVk;
struct SwapchainFrameResourcesVk;
#endif

/**
 * @brief The base class definition for the `DisplayManager` class of BaseGameFramework.
 * This class is used to:
 *   * Query graphics API availability
 *   * Query graphics API version and feature support
 *   * Initialize and shutdown graphics APIs for rendering use
 *   * Initialize, present, and shutdown swapchains for rendering presentation to a
 *     device screen
 *   * Manage display and swapchain configuration changes
 */
class DisplayManager {
 public:
  /** @brief Typedef for identifier of a unique display */
  typedef uint32_t DisplayId;
  /** @brief Typedef for handle to a swapchain active frame */
  typedef uint32_t SwapchainFrameHandle;
  /** @brief Typedef for handle to an active swapchain */
  typedef uint32_t SwapchainHandle;

  /** @brief Constant specifying the default, primary display of a device */
  static constexpr DisplayId kDefault_Display = 0;

  /** @brief Constant returned by ::GetGraphicsAPISupportFlags if an API is unsupported */
  static constexpr uint32_t kGraphics_API_Unsupported = 0;
  /**
   * @brief Constant returned by ::GetGraphicsAPISupportFlags if availability data
   * for a specified API is not yet ready.
   */
  static constexpr uint32_t kGraphics_API_Waiting = (1U << 31);

  /** @brief Constant specifying an invalid swapchain or swapchain frame handle */
  static constexpr SwapchainFrameHandle kInvalid_swapchain_handle = 0xFFFFFFFF;

  /** @brief Enum of graphics APIs supported by DisplayManager */
  enum GraphicsAPI : int32_t {
    /** @brief No graphics API, used internally when no API has been initialized */
    kGraphicsAPI_None = 0,
    /** @brief OpenGL ES graphics API */
    kGraphicsAPI_GLES,
    /** @brief Vulkan graphics API */
    kGraphicsAPI_Vulkan
  };

  /** @brief Enum of bit flags specifying levels of OpenGL ES support */
  enum GLESFeatureFlags : uint32_t {
    /** @brief Constant if no version of OpenGL ES is supported */
    kNo_GLES_Support = 0,
    /** @brief Bit flag if OpenGL ES 3.0 is supported */
    kGLES_3_0_Support = (1U << 0),
    /** @brief Bit flag if OpenGL ES 3.1 is supported */
    kGLES_3_1_Support = (1U << 1),
    /** @brief Bit flag if OpenGL ES 3.2 is supported */
    kGLES_3_2_Support = (1U << 2),
    /** @brief Bit flag if the Android Extension Pack for OpenGL ES is supported */
    kGLES_AEP_Support = (1U << 16),
    /** @brief Bit flag if ANGLE is the OpenGL ES implementation TODO: */
    kGLES_Uses_ANGLE = (1U << 24)
  };

  /** @brief Enum of bit flags specifying levels of OpenGL ES support */
  enum VulkanFeatureFlags : uint32_t {
    /** @brief Constant if no version of Vulkan is supported */
    kNo_Vulkan_Support = 0,
    /** @brief Bit flag if Vulkan 1.0 is supported */
    kVulkan_1_0_Support = (1U << 0),
    /** @brief Bit flag if Vulkan 1.1 is supported */
    kVulkan_1_1_Support = (1U << 1),
    /** @brief Bit flag if Vulkan 1.2 is supported */
    kVulkan_1_2_Support = (1U << 2),
    /** @brief Bit flag if Vulkan 1.3 is supported */
    kVulkan_1_3_Support = (1U << 3),
    /** @brief Bit flag if device meets Android Baseline Profile for Vulkan 2021 TODO: */
    kVulkan_Android_Baseline_Profile_2021 = (1U << 16),
    /** @brief Bit flag if device meets Android Baseline Profile for Vulkan 2022 TODO: */
    kVulkan_Android_Baseline_Profile_2022 = (1U << 17),
    /** @brief Require Vulkan device to include hardware support for ETC2 textures */
    kVulkan_ETC2_Support = (1U << 18)
  };

  /** @brief Enum of result values from ::InitGraphicsAPI */
  enum InitGraphicsAPIResult : int32_t {
    /** @brief Graphics API successfully initialized */
    kInit_GraphicsAPI_Success = 0,
    /** @brief Failure due to API missing a requested feature */
    kInit_GraphicsAPI_Failure_Missing_Feature = -1000,
    /** @brief Failure due to a graphics API already being initialized */
    kInit_GraphicsAPI_Failure_Already_Initialized = -1001,
    /** @brief Failure due to the graphics API not being supported */
    kInit_GraphicsAPI_Failure_Unsupported = -1002,
    /** @brief Failure due to API not yet being ready for initialization */
    kInit_GraphicsAPI_Failure_Not_Ready = -1003,
    /** @brief Failure due to API not yet being ready for initialization */
    kInit_GraphicsAPI_Failure_Initialization = -1004
  };

  /** @brief Enum of possible depth buffer formats */
  enum DisplayDepthFormat : int32_t {
    /** @brief Packed 24-bit depth/8-bit stencil buffer */
    kDisplay_Depth_Format_D24S8_Packed = 0,
    /** @brief 16-bit unsigned depth buffer */
    kDisplay_Depth_Format_16U,
    /** @brief 16-bit float depth buffer */
    kDisplay_Depth_Format_16F,
    /** @brief 32-bit float depth buffer */
    kDisplay_Depth_Format_32F,
    /** @brief Format count enum */
    kDisplay_Depth_Format_Count,
    /** @brief No selected depth buffer format */
    kDisplay_Depth_Format_None = -1
  };

  /** @brief Enum of possible stencil buffer formats */
  enum DisplayStencilFormat : int32_t {
    /** @brief Packed 24-bit depth/8-bit stencil buffer */
    kDisplay_Stencil_Format_D24S8_Packed = 0,
    /** 8-bit stencil buffer */
    kDisplay_Stencil_Format_S8,
    /** @brief Format count enum */
    kDisplay_Stencil_Format_Count,
    /** @brief No selected stencil buffer format */
    kDisplay_Stencil_Format_None = -1
  };

  /** @brief Enum of possible color pixel buffer formats */
  enum DisplayPixelFormat : int32_t {
    /** @brief RGBA order, 8-bits per channel, unsigned byte */
    kDisplay_Pixel_Format_RGBA8 = 0,
    /** @brief BGRA order, 8-bits per channel, unsigned byte */
    kDisplay_Pixel_Format_BGRA8,
    /** @brief RGBA order, 16-bits per channel, float */
    kDisplay_Pixel_Format_RGBA_16F,
    /** @brief RGBA order, 32-bits per channel, float */
    kDisplay_Pixel_Format_RGBA_32F,
    /** @brief RGBA order, 4-bits per channel, unsigned */
    kDisplay_Pixel_Format_RGBA4,
    /** @brief RGBA order, 5-bits color channel, 1-bit alpha, unsigned */
    kDisplay_Pixel_Format_RGBA5551,
    /** @brief RGB order, 5/6/5 bits R/G/B, unsigned */
    kDisplay_Pixel_Format_RGB565,
    /** @brief ABGR order, 2-bits alpha, 10-bits per color channel, unsigned */
    kDisplay_Pixel_Format_ABGR2_10_10_10,
    /** @brief Format count enum */
    kDisplay_Pixel_Format_Count,
    /** @brief No selected pixel buffer format */
    kDisplay_Pixel_Format_None = -1
  };

  /** @brief Enum of possible display color spaces */
  enum DisplayColorSpace : int32_t {
    /** @brief Linear color space */
    kDisplay_Color_Space_Linear = 0,
    /** @brief SRGB color space */
    kDisplay_Color_Space_SRGB
  };

  /** @brief Enum of possible display orientations */
  enum DisplayOrientation : int32_t {
    /** @brief Landscape orientation */
    kDisplay_Orientation_Landscape = (1 << 0),
    /** @brief Portrait orientation */
    kDisplay_Orientation_Portrait = (1 << 1)
  };

  /** @brief Enum of possible display frame swap intervals */
  enum DisplaySwapInterval : uint64_t {
    /** 165 frames per second swap interval */
    kDisplay_Swap_Interval_165FPS = 6060606L,
    /** 120 frames per second swap interval */
    kDisplay_Swap_Interval_120FPS = 8333333L,
    /** 90 frames per second swap interval */
    kDisplay_Swap_Interval_90FPS = 11111111L,
    /** 60 frames per second swap interval */
    kDisplay_Swap_Interval_60FPS = 16666667L,
    /** 45 frames per second swap interval */
    kDisplay_Swap_Interval_45FPS = 22222222L,
    /** 30 frames per second swap interval */
    kDisplay_Swap_Interval_30FPS = 33333333L
  };

  /** @brief Enum of result values from ::InitSwapchain */
  enum InitSwapchainResult : int32_t {
    /** @brief Swapchain initialization successful */
    kInit_Swapchain_Success = 0,
    /** @brief Swapchain initialization failed */
    kInit_Swapchain_Failure = -2000,
    /** @brief Invalid display id for swapchain */
    kInit_Swapchain_Invalid_DisplayId = -2001
  };

  /** @brief Enum of display changed callback messages */
  enum DisplayChangeMessage : int32_t {
    /** @brief Display platform window was initialized */
    kDisplay_Change_Window_Init = 0,
    /** @brief Display platform window was terminated */
    kDisplay_Change_Window_Terminate,
    /** @brief Display platform window was resized */
    kDisplay_Change_Window_Resized,
    /** @brief Display platform window needs to be redrawn */
    kDisplay_Change_Window_Redraw_Needed,
    /** @brief Display platform window content rectangle changed */
    kDisplay_Change_Window_Content_Rect_Changed,
    /** @brief Display platform window insets changed */
    kDisplay_Change_Window_Insets_Changed
  };

  /** @brief Enum of possible display buffer modes */
  enum DisplayBufferMode : int32_t {
    /** Double buffer mode, one present frame, one render frame */
    kDisplay_Double_Buffer = 2,
    /** Triple buffer mode, one present frame, two render frames */
    kDisplay_Triple_Buffer = 3
  };

  /** @brief Enum of swapchain changed callback messages */
  enum SwapchainChangeMessage : int32_t {
    /** @brief Swapchain lost its window */
    kSwapchain_Lost_Window = 0,
    /** @brief Swapchain gained a window */
    kSwapchain_Gained_Window = 1,
    /** @brief Swapchain was lost and needs to be recreated */
    kSwapchain_Needs_Recreation = 2,
  };

  /** @brief Enum of possible swapchain present modes */
  enum SwapchainPresentMode : int32_t {
    /** @brief Present immediately without waiting for vblank */
    kSwapchain_Present_Immediate = 0,
    /** @brief Present next vblank, evict any currently waiting frames */
    kSwapchain_Present_Mailbox,
    /** @brief Present next vblank, chain behind waiting frames */
    kSwapchain_Present_Fifo,
    /** @brief Present next vblank unless previously missed, chain behind waiting frames */
    kSwapchain_Present_Fifo_Relaxed
  };

  /** @brief Enum of possible swapchain rotation modes, used for projection
   * pre-rotation when using Vulkan to match swapchain display native resolution */
  enum SwapchainRotationMode : int32_t {
    /** @brief No rotation needed */
    kSwapchain_Rotation_None = 0,
    /** @brief 90 degree rotation needed */
    kSwapchain_Rotation_90,
    /** @brief 180 degree rotation needed */
    kSwapchain_Rotation_180,
    /** @brief 270 degree rotation needed */
    kSwapchain_Rotation_270
  };

  /** @brief Structure specifying the display format of a swapchain configuration */
  struct DisplayFormat {
    DisplayFormat()
      : display_color_space(kDisplay_Color_Space_Linear)
      , display_depth_format(kDisplay_Depth_Format_None)
      , display_pixel_format(kDisplay_Pixel_Format_RGBA8)
      , display_stencil_format(kDisplay_Stencil_Format_None) {
    }

    DisplayFormat(const DisplayColorSpace color_space,
                  const DisplayDepthFormat depth_format,
                  const DisplayPixelFormat pixel_format,
                  const DisplayStencilFormat stencil_format)
        : display_color_space(color_space)
        , display_depth_format(depth_format)
        , display_pixel_format(pixel_format)
        , display_stencil_format(stencil_format) {
    }

    bool operator==(const DisplayFormat &b) const {
      return (display_color_space == b.display_color_space &&
              display_depth_format == b.display_depth_format &&
              display_pixel_format == b.display_pixel_format &&
              display_stencil_format == b.display_stencil_format);
    }

    /** @brief The color space of a swapchain configuration */
    DisplayColorSpace display_color_space;
    /** @brief The depth buffer format (if any) of a swapchain configuration */
    DisplayDepthFormat display_depth_format;
    /** @brief The color pixel buffer format of a swapchain configuration */
    DisplayPixelFormat display_pixel_format;
    /** @brief The stencil buffer format (if any) of a swapchain configuration */
    DisplayStencilFormat display_stencil_format;
  };

  /** @brief Structure specifying the display resolution of a swapchain configuration */
  struct DisplayResolution {
    DisplayResolution(const int32_t width, const int32_t height, const int32_t dpi,
                      const DisplayOrientation orientation)
      : display_width(width)
      , display_height(height)
      , display_dpi(dpi)
      , display_orientation(orientation) {
    }

    bool operator==(const DisplayResolution &b) const {
      return (display_width == b.display_width &&
              display_height == b.display_height &&
              display_dpi == b.display_dpi &&
              display_orientation == b.display_orientation);
    }

    /** @brief The display width in pixels */
    int32_t display_width;
    /** @brief The display height in pixels */
    int32_t display_height;
    /** @brief The display density in dots per square inch */
    int32_t display_dpi;
    /** @brief The orientation of this display resolution */
    DisplayOrientation display_orientation;
  };

  /** @brief Structure holding available configuration options for a display swapchain */
  struct SwapchainConfigurations {
    SwapchainConfigurations(const std::vector<DisplayFormat>& formats,
                            const std::vector<DisplayResolution>& resolutions,
                            const std::vector<DisplaySwapInterval>& swap_intervals,
                            const uint32_t min_count, const uint32_t max_count,
                            const uint32_t present_modes, const DisplayId display_id)
      : display_formats(formats)
      , display_resolutions(resolutions)
      , display_swap_intervals(swap_intervals)
      , min_swapchain_frame_count(min_count)
      , max_swapchain_frame_count(max_count)
      , swapchain_present_modes(present_modes)
      , swapchain_display_id(display_id) {
    }
    /** @brief An array of display formats supported by the display swapchain */
    const std::vector<DisplayFormat>& display_formats;
    /** @brief An array of display resolutions supported by the display swapchain */
    const std::vector<DisplayResolution>& display_resolutions;
    /** @brief An array of display swap intervals supported by the display swapchain */
    const std::vector<DisplaySwapInterval>& display_swap_intervals;
    /** @brief The minimum number of frame images configurable by the display swapchain */
    const uint32_t min_swapchain_frame_count;
    /** @brief The maximum number of frame images configurable by the display swapchain */
    const uint32_t max_swapchain_frame_count;
    /** @brief A bitmask of `SwapchainPresentMode` values of supported present modes */
    const uint32_t swapchain_present_modes;
    /** @brief The display ID that the swapchain belongs to */
    const DisplayId swapchain_display_id;
  };

  /** @brief Structure for communicating display changed info in a callback */
  struct DisplayChangeInfo {
    DisplayChangeInfo(const DisplayResolution &resolution, const DisplayChangeMessage message)
      : display_resolution(resolution)
      , change_message(message) {}
    /** @brief Current resolution of the display */
    const DisplayResolution &display_resolution;
    /** @brief The `DisplayChangeMessage` being sent to the callback */
    DisplayChangeMessage change_message;
  };

  /** @brief Definition of the DisplayChangeCallback to be used with ::SetDisplayChangedCalllback */
  typedef std::function<void(const DisplayChangeInfo& display_change_info, void* user_data)>
      DisplayChangedCallback;

  /**
   * @brief Definition of the SwapchainChangedCallback to be used with ::SetSwapchainChangedCallback
   */
  typedef std::function<void(const SwapchainChangeMessage reason, void* user_data)>
      SwapchainChangedCallback;

/**
 * @brief Retrieve an instance of the `DisplayManager`. The first time this is called
 * it will construct and initialize the manager.
 * @return Reference to the `DisplayManager` class.
 */
  static DisplayManager& GetInstance();

/**
 * @brief Shuts down the `DisplayManager`.
 */
  static void ShutdownInstance();

/**
 * @brief Retrieve an array of common frame swap interval constants. The
 * array is terminated by a 0 value.
 * @return Start of a 0 terminated array of swap interval constants in nanoseconds.
 */
  static const uint64_t *GetSwapIntervalConstants();

/**
 * @brief Class destructor, do not call directly, use ::ShutdownInstance.
 */
  ~DisplayManager();

  DisplayManager(const DisplayManager &) = delete;
  DisplayManager& operator=(const DisplayManager &) = delete;

/**
 * @brief Get support and version information for a specified graphics API
 * @param api A `GraphicsAPI` enum specifying which API to get information about
 * @return A bitmask of feature bits, dependent on which API was being queried.
 * For OpenGL ES this maps to `GLESFeatureFlags` and for Vulkan to `VulkanFeatureFlags`.
 * A return value of 0 indicates no support for the specified graphics API.
 * A value of `kGraphics_API_Waiting` is returned, the application
 * should call ::GetGraphicsAPISupportFlags again after a short interval. Some platforms cannot
 * initialize a graphics context until a native window becomes available.
 */
  uint32_t GetGraphicsAPISupportFlags(const GraphicsAPI api);

/**
 * @brief Initialize the specified graphics API
 * @param api A `GraphicsAPI` enum specifying which API to get information about
 * @param requested_features A bitmask of required features (i.e. version support), specific to the
 * specified graphics API. For OpenGL ES this maps to `GLESFeatureFlags` and for Vulkan to
 * `VulkanFeatureFlags`.
 * @return A `InitGraphicsAPIResult` enum with the result of the initialization.
 */
  InitGraphicsAPIResult InitGraphicsAPI(const GraphicsAPI api,
                                        const uint32_t requested_features);

/**
 * @brief Shuts down the active graphics API.
 */
  void ShutdownGraphicsAPI();

/**
 * @brief Retrieves feature information about the active graphics API.
 * @return A `GraphicsAPIFeatures` reference with feature information. If no graphics API
 * is active, this will be a placeholder 'empty' feature struct.
 */
  const GraphicsAPIFeatures& GetGraphicsAPIFeatures();

/**
 * @brief Get the number of active displays of the device.
 * @return A count of active displays on the device, can be used with ::GetDisplayId
 * to retrieve IDs for a display. There will always be at least one primary display
 * which can be specified with the `DisplayManager::kDefault_Display` constant.
 */
  uint32_t GetDisplayCount();

/**
 * @brief Retrieves the display id for the specified display
 * @param display_index Index of the display, must be less than the number
 * returned by ::GetDisplayCount
 * @return A `DisplayId` id value for the specified display index
 */
  DisplayId GetDisplayId(const uint32_t display_index);

/**
 * @brief Retrieves the current frame buffer mode.
 * @return A `DisplayBufferMode` enum with the current buffer mode
 */
  DisplayBufferMode GetDisplayBufferMode() const { return buffer_mode_; }

/**
 * @brief Set the frame buffer mode to be used by any graphics APIs and swapchains. This
 * should be called prior to any graphics API initialization. If not called, initialization
 * will default to a double-buffer mode.
 * @param buffer_mode A `DisplayBufferMode` enum with the buffer mode to use.
 */
  void SetDisplayBufferMode(const DisplayBufferMode buffer_mode) { buffer_mode_ = buffer_mode; }

/**
 * @brief Set a callback to be called when a configuration of the active display changes.
 * @param callback A function object to use as the callback. Passing nullptr will clear
 * any currently registered callback.
 * @param user_data A pointer to user data to be passed to the callback
 */
  bool SetDisplayChangedCallback(DisplayChangedCallback callback, void* user_data);

/**
 * @brief Set a callback to be called when a change related to the active swapchain occurs.
 * @param callback A function object to use as the callback. Passing nullptr will clear
 * any currently registered callback.
 * @param user_data A pointer to user data to be passed to the callback
 */
  bool SetSwapchainChangedCallback(SwapchainChangedCallback callback, void* user_data);

/**
 * @brief Retrieves the current configurations available for swapchain creation by the
 * active graphics API.
 * @param The ID of the display to retrieve swapchain configurations from
 * @return A `SwapchainConfigurations` unique pointer, or nullptr if no graphics
 * API is active
 */
  std::unique_ptr<SwapchainConfigurations> GetSwapchainConfigurations(const DisplayId display_id);

/**
 * @brief Initialize a swapchain for the active graphics API
 * @param display_format A `DisplayFormat` struct specifying formats and color space.
 * @param display_resolution A `DisplayResolution` struct specifying the display resolution
 * @param display_swap_interval A `DisplaySwapInterval` enum specifying the  display swap interval
 * @param swapchain_frame_count The number of image frames to use in the swapchain
 * @param present_mode A `SwapchainPresentMode` enum specifying the present mode for the swapchain
 * @param swapchain_handle A pointer to return a handle to the new swapchain in.
 * @return A `InitSwapchainResult` enum with the result of the initialization.
 */
  InitSwapchainResult InitSwapchain(const DisplayFormat& display_format,
                                    const DisplayResolution& display_resolution,
                                    const DisplaySwapInterval display_swap_interval,
                                    const uint32_t swapchain_frame_count,
                                    const SwapchainPresentMode present_mode,
                                    const DisplayId display_id,
                                    SwapchainHandle* swapchain_handle);

/**
 * @brief Determine if a swapchain handle is for a valid swapchain
 * @param swapchain_handle Handle to the specified swapchain
 * @return true if the handle is for a valid swapchain
 */
  bool GetSwapchainValid(const SwapchainHandle swapchain_handle);

/**
 * @brief Shutdown an active swapchain
 * @param swapchain_handle Handle to the specified swapchain
 */
  void ShutdownSwapchain(const SwapchainHandle swapchain_handle);

/**
 * @brief Get a handle to the next pending frame of an active swapchain
 * @param swapchain_handle Handle to the specified swapchain
 * @return A `SwapchainFrameHandle` reference to a swapchain frame
 */
  SwapchainFrameHandle GetCurrentSwapchainFrame(const SwapchainHandle swapchain_handle);

/**
 * @brief Get the current rotation mode of an active swapchain
 * @param swapchain_handle Handle to the specified swapchain
 * @return A `SwapchainRotationMode` enum of the rotation mode of the swapchain
 */
  SwapchainRotationMode GetSwapchainRotationMode(const SwapchainHandle swapchain_handle);

/**
 * @brief Present the pending frame of a swapchain
 * @param swapchain_handle Handle to the specified swapchain
 * @return A `SwapchainFrameHandle` reference to the new pending swapchain frame
 */
  SwapchainFrameHandle PresentCurrentSwapchainFrame(const SwapchainHandle swapchain_handle);

#if defined BGF_DISPLAY_MANAGER_GLES
/**
 * @brief Get OpenGL ES specific resources related to the global graphics API
 * @param api_resources_gles A reference to a `GraphicsAPIResourcesGLES` structure to populate
 * with global resource information related to rendering using OpenGL ES
 * @return True if resource information was successfully retrieved.
 */
  bool GetGraphicsAPIResourcesGLES(GraphicsAPIResourcesGLES& api_resources_gles);

/**
 * @brief Get OpenGL ES specific resources related to the pending swapchain frame
 * @param frame_handle Handle to the specified swapchain
 * @param frame_resources A reference to a `SwapchainFrameResourcesGLES` structure to populate
 * with swapchain frame resources related to rendering using OpenGL ES
 * @return True if resource information was successfully retrieved.
 */
  bool GetSwapchainFrameResourcesGLES(const SwapchainFrameHandle frame_handle,
                                      SwapchainFrameResourcesGLES& frame_resources);
#endif // BGF_DISPLAY_MANAGER_GLES

#if defined BGF_DISPLAY_MANAGER_VULKAN
/**
 * @brief Get Vulkan specific resources related to the global graphics API
 * @param api_resources_vk A reference to a `GraphicsAPIResourcesVk` structure to populate
 * with global resource information related to rendering using Vulkan
 * @return True if resource information was successfully retrieved.
 */
  bool GetGraphicsAPIResourcesVk(GraphicsAPIResourcesVk& api_resources_vk);

/**
 * @brief Get Vulkan specific resources related to the pending swapchain frame
 * @param frame_handle Handle to the specified swapchain
 * @param frame_resources A reference to a `SwapchainFrameResourcesVk` structure to populate
 * with swapchain frame resources related to rendering using Vulkan
 * @return True if resource information was successfully retrieved.
 */
  bool GetSwapchainFrameResourcesVk(const SwapchainFrameHandle frame_handle,
                                    SwapchainFrameResourcesVk& frame_resources,
                                    bool acquire_frame_image);
#endif // BGF_DISPLAY_MANAGER_VULKAN

/**
 * @brief Internal function, do not call directly
 * @param change_message A `DisplayChangeMessage` enum with the change message
 */
  void HandlePlatformDisplayChange(const DisplayChangeMessage& change_message);

 private:
  DisplayManager();

  GraphicsAPI active_api_;
  std::shared_ptr<GraphicsAPIBase> api_;
#if defined BGF_DISPLAY_MANAGER_GLES
  std::shared_ptr<GraphicsAPIGLES> api_gles_;
#endif
#if defined BGF_DISPLAY_MANAGER_VULKAN
  std::shared_ptr<GraphicsAPIVulkan> api_vulkan_;
#endif
  DisplayBufferMode buffer_mode_;
  GraphicsAPIFeatures null_features_;
  static std::unique_ptr<DisplayManager> instance_;
  static constexpr const char* BGM_CLASS_TAG = "BGF::DisplayManager";
};

} // namespace base_game_framework

#endif //BASEGAMEFRAMEWORK_DISPLAYMANAGER_H_
