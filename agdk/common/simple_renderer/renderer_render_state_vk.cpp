/*
 * Copyright 2023 The Android Open Source Project
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

#include "renderer_render_state_vk.h"
#include "renderer_debug.h"
#include "renderer_render_pass_vk.h"
#include "renderer_shader_program_vk.h"
#include "renderer_uniform_buffer_vk.h"
#include "renderer_vk_includes.h"
#include "renderer_vk.h"

namespace simple_renderer {

static constexpr uint32_t kMaxAttributes = 3;

static constexpr VkPrimitiveTopology primitive_values[RenderState::kPrimitiveCount] = {
    VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
    VK_PRIMITIVE_TOPOLOGY_LINE_LIST
};

static constexpr VkCompareOp depth_function_values[RenderState::kDepthCompareCount] = {
    VK_COMPARE_OP_NEVER,
    VK_COMPARE_OP_ALWAYS,
    VK_COMPARE_OP_EQUAL,
    VK_COMPARE_OP_NOT_EQUAL,
    VK_COMPARE_OP_LESS,
    VK_COMPARE_OP_LESS_OR_EQUAL,
    VK_COMPARE_OP_GREATER,
    VK_COMPARE_OP_GREATER_OR_EQUAL
};

static constexpr VkCullModeFlagBits cull_modes[RenderState::kCullFaceCount] = {
    VK_CULL_MODE_FRONT_BIT,
    VK_CULL_MODE_BACK_BIT,
    VK_CULL_MODE_FRONT_AND_BACK
};

static constexpr uint32_t kPositionAttributeSize = (3 * sizeof(float));
static constexpr uint32_t kTextureAttributeSize = (2 * sizeof(float));
static constexpr uint32_t kColorAttributeSize = (4 * sizeof(float));

static void ConfigureAttachmentStates(const RenderState::RenderStateCreationParams& params,
    VkPipelineMultisampleStateCreateInfo& pipeline_multisample_state_info,
    VkPipelineColorBlendStateCreateInfo& pipeline_color_blend_state_info,
    VkPipelineDepthStencilStateCreateInfo& pipeline_depth_stencil_state_info,
    VkPipelineColorBlendAttachmentState& pipeline_color_blend_attachment_state) {
  pipeline_multisample_state_info.sampleShadingEnable = VK_FALSE;
  pipeline_multisample_state_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
  pipeline_multisample_state_info.minSampleShading = 1.f;
  pipeline_multisample_state_info.pSampleMask = nullptr;
  pipeline_multisample_state_info.alphaToCoverageEnable = VK_FALSE;
  pipeline_multisample_state_info.alphaToOneEnable = VK_FALSE;

  pipeline_color_blend_attachment_state.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT |
          VK_COLOR_COMPONENT_G_BIT |
          VK_COLOR_COMPONENT_B_BIT |
          VK_COLOR_COMPONENT_A_BIT;
  pipeline_color_blend_attachment_state.blendEnable = VK_FALSE;
  pipeline_color_blend_attachment_state.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
  pipeline_color_blend_attachment_state.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
  pipeline_color_blend_attachment_state.colorBlendOp = VK_BLEND_OP_ADD;
  pipeline_color_blend_attachment_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
  pipeline_color_blend_attachment_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
  pipeline_color_blend_attachment_state.alphaBlendOp = VK_BLEND_OP_ADD;

  pipeline_color_blend_state_info.logicOpEnable = VK_FALSE;
  pipeline_color_blend_state_info.logicOp = VK_LOGIC_OP_COPY;
  pipeline_color_blend_state_info.attachmentCount = 1;
  pipeline_color_blend_state_info.pAttachments = &pipeline_color_blend_attachment_state;
  pipeline_color_blend_state_info.blendConstants[0] = 0.0f;
  pipeline_color_blend_state_info.blendConstants[1] = 0.0f;
  pipeline_color_blend_state_info.blendConstants[2] = 0.0f;
  pipeline_color_blend_state_info.blendConstants[3] = 0.0f;

  pipeline_depth_stencil_state_info.depthTestEnable = params.depth_test ? VK_TRUE : VK_FALSE;
  pipeline_depth_stencil_state_info.depthWriteEnable = params.depth_write ? VK_TRUE : VK_FALSE;
  pipeline_depth_stencil_state_info.depthCompareOp = depth_function_values[params.depth_function];
  pipeline_depth_stencil_state_info.depthBoundsTestEnable = VK_FALSE;
  pipeline_depth_stencil_state_info.stencilTestEnable = VK_FALSE;
}

static uint32_t ConfigureAttributeDescriptions(const VertexBuffer::VertexFormat state_vertex_layout,
                                               const uint32_t max_attributes,
                                               VkVertexInputAttributeDescription* attributes) {
  uint32_t attribute_count = 0;
  uint32_t attribute_offset = 0;

  // Always have position
  attributes->binding = 0;
  attributes->location = attribute_count;
  attributes->format = VK_FORMAT_R32G32B32_SFLOAT;
  attributes->offset = attribute_offset;
  ++attributes;
  ++attribute_count;
  attribute_offset += kPositionAttributeSize;
  if (state_vertex_layout == VertexBuffer::kVertexFormat_P3T2 ||
      state_vertex_layout == VertexBuffer::kVertexFormat_P3T2C4) {
    RENDERER_ASSERT(attribute_count < max_attributes)
    attributes->binding = 0;
    attributes->location = attribute_count;
    attributes->format = VK_FORMAT_R32G32_SFLOAT;
    attributes->offset = attribute_offset;
    ++attributes;
    ++attribute_count;
    attribute_offset += kTextureAttributeSize;
  }
  if (state_vertex_layout == VertexBuffer::kVertexFormat_P3C4 ||
      state_vertex_layout == VertexBuffer::kVertexFormat_P3T2C4) {
    RENDERER_ASSERT(attribute_count < max_attributes)
    attributes->binding = 0;
    attributes->location = attribute_count;
    attributes->format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attributes->offset = attribute_offset;
    ++attributes;
    ++attribute_count;
    attribute_offset += kColorAttributeSize;
  }
  return attribute_count;
}

static void ConfigureRasterizationState(const RenderState::RenderStateCreationParams& params,
    VkPipelineRasterizationStateCreateInfo& pipeline_rasterization_state_info) {
  pipeline_rasterization_state_info.depthClampEnable = VK_FALSE;
  pipeline_rasterization_state_info.rasterizerDiscardEnable = VK_FALSE;
  pipeline_rasterization_state_info.polygonMode = VK_POLYGON_MODE_FILL;
  pipeline_rasterization_state_info.lineWidth = params.line_width;
  const VkCullModeFlagBits cull_mode = params.cull_enabled ?
                                       cull_modes[params.cull_face] : VK_CULL_MODE_NONE;
  pipeline_rasterization_state_info.cullMode = cull_mode;
  pipeline_rasterization_state_info.frontFace =
      (params.front_face == RenderState::kFrontFaceClockwise) ? VK_FRONT_FACE_CLOCKWISE :
                                                                VK_FRONT_FACE_COUNTER_CLOCKWISE;
  pipeline_rasterization_state_info.depthBiasEnable = VK_FALSE;
  pipeline_rasterization_state_info.depthBiasConstantFactor = 0.f;
  pipeline_rasterization_state_info.depthBiasClamp = 0.f;
  pipeline_rasterization_state_info.depthBiasSlopeFactor = 0.f;
}

static void ConfigureViewport(const RenderState::RenderStateCreationParams& params,
                              VkRect2D& scissor, VkViewport& viewport,
                              VkPipelineViewportStateCreateInfo& pipeline_viewport_state_info) {
  scissor.offset.x = params.scissor_rect.x;
  scissor.offset.y = params.scissor_rect.y;
  scissor.extent.width = params.scissor_rect.width;
  scissor.extent.height = params.scissor_rect.height;

  viewport.x = params.viewport.x;
  viewport.y = params.viewport.y;
  viewport.width = params.viewport.width;
  viewport.height = params.viewport.height;
  viewport.minDepth = params.viewport.min_depth;
  viewport.maxDepth = params.viewport.max_depth;

  pipeline_viewport_state_info.viewportCount = 1;
  pipeline_viewport_state_info.pViewports = &viewport;
  pipeline_viewport_state_info.scissorCount = 1;
  pipeline_viewport_state_info.pScissors = &scissor;
}


RenderStateVk::RenderStateVk(const RenderStateCreationParams& params) :
    render_pass_(params.render_pass),
    state_program_(params.state_program),
    state_uniform_(params.state_uniform),
    descriptor_set_layout_(VK_NULL_HANDLE),
    pipeline_(VK_NULL_HANDLE),
    pipeline_layout_(VK_NULL_HANDLE),
    state_vertex_layout_(params.state_vertex_layout),
    primitive_type_(primitive_values[params.primitive_type]) {
  RENDERER_LOG("Render state program: %s / %s uniform %s vertex format %u",
               state_program_->GetVertexDebugName().c_str(),
               state_program_->GetFragmentDebugName().c_str(),
               state_uniform_->GetBufferDebugName().c_str(),
               state_vertex_layout_)
  const RenderPassVk& render_pass_vk = *(static_cast<RenderPassVk*>(render_pass_.get()));
  VkRenderPass render_pass = render_pass_vk.GetRenderPassVk();
  RENDERER_ASSERT(render_pass != VK_NULL_HANDLE)

  descriptor_set_layout_ =
      RendererVk::GetInstanceVk().GetDescriptorSetLayout(params.state_vertex_layout);
  CreatePipelineLayout(params);

  VkVertexInputBindingDescription vertex_input_binding_description = {};
  vertex_input_binding_description.binding = 0;
  vertex_input_binding_description.stride =
      VertexBuffer::GetVertexFormatStride(state_vertex_layout_);
  vertex_input_binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

  VkVertexInputAttributeDescription attributes[kMaxAttributes];
  memset(attributes, 0, sizeof(attributes));
  const uint32_t attribute_count = ConfigureAttributeDescriptions(params.state_vertex_layout,
                                                                  kMaxAttributes, attributes);


  VkPipelineVertexInputStateCreateInfo pipeline_vertex_input_state_create_info =
      {VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
  pipeline_vertex_input_state_create_info.vertexBindingDescriptionCount = 1;
  pipeline_vertex_input_state_create_info.pVertexBindingDescriptions =
      &vertex_input_binding_description;
  pipeline_vertex_input_state_create_info.vertexAttributeDescriptionCount = attribute_count;
  pipeline_vertex_input_state_create_info.pVertexAttributeDescriptions = attributes;

  VkPipelineInputAssemblyStateCreateInfo pipeline_input_assembly_state_info =
      {VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
  pipeline_input_assembly_state_info.topology = primitive_type_;
  pipeline_input_assembly_state_info.primitiveRestartEnable = VK_FALSE;

  const ShaderProgramVk& shader_program = *(static_cast<ShaderProgramVk*>(state_program_.get()));

  VkPipelineShaderStageCreateInfo vertex_pipeline_info =
      { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
  vertex_pipeline_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
  vertex_pipeline_info.module = shader_program.GetVertexModule();
  vertex_pipeline_info.pName = "main";

  VkPipelineShaderStageCreateInfo fragment_pipeline_info =
      { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
  fragment_pipeline_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  fragment_pipeline_info.module = shader_program.GetFragmentModule();
  fragment_pipeline_info.pName = "main";

  VkPipelineShaderStageCreateInfo pipeline_shader_info[] = {
vertex_pipeline_info,
fragment_pipeline_info
  };

  VkRect2D scissor = {};
  VkViewport viewport = {};
  VkPipelineViewportStateCreateInfo pipeline_viewport_state_info =
      {VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
  ConfigureViewport(params, scissor, viewport, pipeline_viewport_state_info);

  VkPipelineRasterizationStateCreateInfo pipeline_rasterization_state_info =
      {VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
  ConfigureRasterizationState(params, pipeline_rasterization_state_info);

  VkPipelineMultisampleStateCreateInfo pipeline_multisample_state_info = {VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
  VkPipelineColorBlendStateCreateInfo pipeline_color_blend_state_info = {VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
  VkPipelineDepthStencilStateCreateInfo pipeline_depth_stencil_state_info = {VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
  VkPipelineColorBlendAttachmentState pipeline_color_blend_attachment_state = {};
  ConfigureAttachmentStates(params, pipeline_multisample_state_info, pipeline_color_blend_state_info, pipeline_depth_stencil_state_info, pipeline_color_blend_attachment_state);

  VkDynamicState dynamic_states[2] = {
      VK_DYNAMIC_STATE_VIEWPORT,
      VK_DYNAMIC_STATE_SCISSOR
  };

  VkPipelineDynamicStateCreateInfo dynamic_state_create_info = {
      VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO
  };
  dynamic_state_create_info.dynamicStateCount = 2;
  dynamic_state_create_info.pDynamicStates = dynamic_states;

  VkGraphicsPipelineCreateInfo pipeline_create_info =
      {VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
  pipeline_create_info.stageCount = 2;
  pipeline_create_info.pStages = pipeline_shader_info;
  pipeline_create_info.pVertexInputState = &pipeline_vertex_input_state_create_info;
  pipeline_create_info.pInputAssemblyState = &pipeline_input_assembly_state_info;
  pipeline_create_info.pViewportState = &pipeline_viewport_state_info;
  pipeline_create_info.pRasterizationState = &pipeline_rasterization_state_info;
  pipeline_create_info.pMultisampleState = &pipeline_multisample_state_info;
  pipeline_create_info.pDepthStencilState = &pipeline_depth_stencil_state_info;
  pipeline_create_info.pColorBlendState = &pipeline_color_blend_state_info;
  pipeline_create_info.pDynamicState = &dynamic_state_create_info;
  pipeline_create_info.layout = pipeline_layout_;
  pipeline_create_info.renderPass = render_pass;
  pipeline_create_info.subpass = 0;
  pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
  pipeline_create_info.basePipelineIndex = -1;

  RendererVk &renderer = RendererVk::GetInstanceVk();
  VkResult pipeline_result = vkCreateGraphicsPipelines(renderer.GetDevice(),
    VK_NULL_HANDLE, 1, &pipeline_create_info, nullptr, &pipeline_);
  RENDERER_CHECK_VK(pipeline_result, "vkCreateGraphicsPipelines");

}

RenderStateVk::~RenderStateVk() {
  VkDevice device = RendererVk::GetInstanceVk().GetDevice();
  if (pipeline_ != VK_NULL_HANDLE) {
    vkDestroyPipeline(device, pipeline_, nullptr);
    pipeline_ = VK_NULL_HANDLE;
  }
  if (pipeline_layout_ != VK_NULL_HANDLE) {
    vkDestroyPipelineLayout(device, pipeline_layout_, nullptr);
    pipeline_layout_ = VK_NULL_HANDLE;
  }
  render_pass_ = nullptr;
  state_program_ = nullptr;
  state_uniform_ = nullptr;
}

void RenderStateVk::CreatePipelineLayout(const RenderStateCreationParams& params) {
  UniformBufferVk& buffer = *(static_cast<UniformBufferVk *>(state_uniform_.get()));
  const UniformBuffer::UniformBufferStageRanges& stage_ranges = buffer.GetStageRanges();

  uint32_t range_count = 0;

  const uint32_t max_range_count = 2; // vertex and fragment
  VkPushConstantRange push_constant_ranges[max_range_count];
  memset(push_constant_ranges, 0, sizeof(VkPushConstantRange) * max_range_count);

  if (stage_ranges.vertex_stage_offset != UniformBufferVk::kUnusedStageRange) {
    push_constant_ranges[range_count].offset = stage_ranges.vertex_stage_offset;
    push_constant_ranges[range_count].size = stage_ranges.vertex_stage_size;
    push_constant_ranges[range_count].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    ++range_count;
  }

  if (stage_ranges.fragment_stage_offset != UniformBufferVk::kUnusedStageRange) {
    push_constant_ranges[range_count].offset = stage_ranges.fragment_stage_offset;
    push_constant_ranges[range_count].size = stage_ranges.fragment_stage_size;
    push_constant_ranges[range_count].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    ++range_count;
  }

  VkDescriptorSetLayout descriptor_set_layouts[] = {descriptor_set_layout_};
  VkPipelineLayoutCreateInfo pipeline_layout_info{};
  pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipeline_layout_info.setLayoutCount = (descriptor_set_layout_ != VK_NULL_HANDLE) ? 1 : 0;
  pipeline_layout_info.pSetLayouts = (descriptor_set_layout_ != VK_NULL_HANDLE) ?
      descriptor_set_layouts : nullptr;
  pipeline_layout_info.pushConstantRangeCount = range_count;
  pipeline_layout_info.pPushConstantRanges = (range_count > 0) ? push_constant_ranges : nullptr;
  RendererVk &renderer = RendererVk::GetInstanceVk();
  const VkResult layout_result = vkCreatePipelineLayout(renderer.GetDevice(), &pipeline_layout_info,
                                                        nullptr, &pipeline_layout_);
  RENDERER_CHECK_VK(layout_result, "vkCreatePipelineLayout");
}

void RenderStateVk::UpdateUniformData(VkCommandBuffer command_buffer, bool force_update) {
  UniformBufferVk& buffer = *(static_cast<UniformBufferVk *>(state_uniform_.get()));
  if (buffer.GetBufferDirty() || force_update) {
    const UniformBuffer::UniformBufferStageRanges& stage_ranges = buffer.GetStageRanges();
    const uint8_t *buffer_data = reinterpret_cast<const uint8_t*>(buffer.GetBufferData());
    vkCmdPushConstants(command_buffer, pipeline_layout_, VK_SHADER_STAGE_VERTEX_BIT,
                       0, stage_ranges.vertex_stage_size, buffer_data);
    if (stage_ranges.fragment_stage_offset != UniformBufferVk::kUnusedStageRange) {
      vkCmdPushConstants(command_buffer, pipeline_layout_, VK_SHADER_STAGE_FRAGMENT_BIT,
                         stage_ranges.fragment_stage_offset,
                         stage_ranges.fragment_stage_size,
                         buffer_data + stage_ranges.fragment_stage_offset);
    }
    buffer.SetBufferDirty(false);
  }
}

}