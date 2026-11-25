# SimpleRenderer

## Overview

SimpleRenderer is a very minimal low-level graphics renderer used by the Android C++ game samples.
SimpleRenderer supports using either GLES 3.0 or Vulkan 1.0 as its low-level rendering API.

## Limitations

SimpleRenderer is intentionally minimally featured and the implementation is focused on being
compact and simple to understand. It is intended for basic use cases that only require a small
number of shader programs, uniform buffers, and vertex buffer formats.

The current implementation only supports a single bound vertex buffer, texture sampler,
uniform buffer, and an optional index buffer. Vertex buffer, index buffer, and texture data is
treated as static and dynamic updates after resource creation is not currently supported.

Multiple render targets are not currently supported, it is assumed rendering is happening against
the 'drawable'/swapchain surfaces for color/depth.

SimpleRenderer does not implement context or device creation, surface/swapchain creation or
present operations. The Android C++ game samples use the DisplayManager class of the
BaseGameFramework library for that functionality.

The SimpleRenderer instance is not currently designed to be called from multiple threads.

## Usage

The base renderer interface is identical regardless of whether GLES or Vulkan is
being used. However, you need to specify which API you wish to use when initializing the renderer.
Each API requires calling a static configuration functions with some API specific values prior
to initializing the renderer.

The base renderer assumes the OpenGL coordinate system. The Vulkan renderer will flip the viewport
internally to compensate. This feature requires the `VK_KHR_Maintenance1` extension, which is
required by the Android Baseline Profile for Vulkan.

### Initializing the renderer

```c++
// The general renderer interface
#include "renderer_interface.h"
// Contains static function for setting up GLES initialization
#include "renderer_gles.h"
// Contains static function for setting up Vulkan initialization
#include "renderer_vk.h"

...
  if (use_vulkan) {
    Renderer::SetRendererAPI(Renderer::kAPI_Vulkan);
    // Set the Vulkan resources that will be used by the renderer
    RendererVk::SetVkResources(vulkan_device, vulkan_physical_device, vulkan_instance);
  } else {
    Renderer::SetRendererAPI(Renderer::kAPI_GLES);
    // Set the EGL resources that will be used by the renderer
    RendererGLES::SetEGLResources(egl_context, egl_display, egl_surface);
  }
  my_renderer_ = Renderer::GetInstance();
```

### Resources

Each render resource in SimpleRenderer has an associated class. Resource objects are created
through the Renderer interface instead of calling resource constructors directly, and destroyed
when the last shared pointer to them is released. The resource classes are:

* IndexBuffer
* UniformBuffer
* VertexBuffer
* ShaderProgram
* Texture
* RenderPass
* RenderState

#### Resource creation

The renderer has `Create...` calls for each resource type. Each create call takes a reference
parameter to a creation parameters structure specific to each resource type:

```c++
  virtual std::shared_ptr<IndexBuffer> CreateIndexBuffer(
      const IndexBuffer::IndexBufferCreationParams& params) = 0;

  virtual std::shared_ptr<RenderPass> CreateRenderPass(
      const RenderPass::RenderPassCreationParams& params) = 0;

  virtual std::shared_ptr<RenderState> CreateRenderState(
      const RenderState::RenderStateCreationParams& params) = 0;

  virtual std::shared_ptr<ShaderProgram> CreateShaderProgram(
      const ShaderProgram::ShaderProgramCreationParams& params) = 0;

  virtual std::shared_ptr<Texture> CreateTexture(
      const Texture::TextureCreationParams& params) = 0;

  virtual std::shared_ptr<UniformBuffer> CreateUniformBuffer(
      const UniformBuffer::UniformBufferCreationParams& params) = 0;

  virtual std::shared_ptr<VertexBuffer> CreateVertexBuffer(
      const VertexBuffer::VertexBufferCreationParams& params) = 0;
```

Data pointers in a `...CreationParams` struct are not referenced post-creation and are not
required to persist after return from the `Create...` call (a `UniformBuffer` copies its
`element_array`).

With Vulkan, buffer and texture data is uploaded through staging buffers. The uploads are batched:
their copies are recorded into one command buffer, which is submitted ahead of the next frame's
commands, so creating many resources (e.g. a scene's geometry) doesn't stall on each upload. A
large batch is submitted early, and waited for.

#### Resource destruction

A resource is destroyed when the last shared pointer to it is released (the `Create...` calls
return a shared pointer with a custom deleter). Destruction is deferred until it is safe:

* Vulkan: until every frame that was in flight when the resource was released has finished
  executing on the GPU, so a resource can be released right after its last draw call.
* OpenGL ES: until the next render frame (Renderer::BeginFrame), when the context is current.

Remaining deferred destructions happen when the swapchain is recreated (Vulkan) and when the
renderer instance is shut down. Release all pointers to a renderer's resources before shutting it
down: a resource released after shutdown can't be destroyed (its device or context is gone), and is
leaked with an error. Resources still referenced at shutdown are logged.

The `Destroy...` calls are optional and do nothing; they are kept for compatibility.

Resources hold references to the resources they use (e.g. a `RenderState` to its
`ShaderProgram` and `UniformBuffer`), so those live at least as long.

#### Resource dependencies

Most resource types do not have any hard dependencies on other resources. The main exception is
the RenderState object. Building a render state object requires linking to the following
resource objects:

* RenderPass
* ShaderProgram
* UniformBuffer

An instance of these objects can be used by multiple RenderState objects.

### Rendering

A typical rendering flow might resemble this:

* Begin render frame
    - Begin Render Pass A
        - Set Render State A
            - Bind Index/Vertex/Texture resources
            - Update render state uniform data
            - Draw 1
            - Update render state uniform data
            - Draw 2
            - Bind Index/Vertex/Texture resources
            - Update render state uniform data
            - Draw 3
        - Set Render State B
            - Bind Index/Vertex/Texture resources
            - Update render state uniform data
            - Draw 4
  - Begin Render Pass B
      - Set Render State C
        - Bind Index/Vertex/Texture resources
        - Update render state uniform data
        - Draw 5
* End render frame

A minimal code example:

```c++
using namespace simple_renderer

  Renderer renderer = Renderer::GetInstance();

  renderer.BeginFrame();
  renderer.SetRenderPass(render_pass);
  renderer.SetRenderState(render_state);
  renderer.BindIndexBuffer(index_buffer);
  renderer.BindVertexBuffer(vertex_buffer);
  renderer.BindTexture(texture);
  uniform_buffer.SetBufferElementData(element_index, element_data, element_size);
  renderer.DrawIndexed(index_count, first_index);
  renderer.EndFrame();
```