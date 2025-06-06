// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#include "core/providers/cpu/tensor/pad.h"

#include "core/framework/op_kernel_type_control_utils.h"
#include "core/providers/common.h"
#include "core/providers/cpu/tensor/utils.h"
#include "core/providers/op_kernel_type_control.h"
#include "core/util/math.h"

#include <functional>

// there's no way to use a raw pointer as the copy destination with std::copy_n
// (which gsl::copy uses with span::data() which returns a raw pointer) with the 14.11 toolset
// without generating a 4996 warning. going through an iterator is way too much overhead so turn off the warning.
#ifdef _MSC_VER
#pragma warning(disable : 4996)
#endif

namespace onnxruntime {

// Register a kernel for kMsDomain (contrib op) Pad
#ifndef DISABLE_CONTRIB_OPS

namespace contrib {
// TODO: Remove this contrib kernel registration and the schema from the appropriate places
// once Keras Mask RCNN is shipped with all ONNX domain ops

// Currently this kernel is required to support Keras Mask-RCNN
// only float type is supported
ONNX_OPERATOR_KERNEL_EX(Pad,
                        kMSDomain,
                        1,
                        kCpuExecutionProvider,
                        KernelDefBuilder().TypeConstraint("T", DataTypeImpl::GetTensorType<float>()),
                        onnxruntime::Pad);

}  // namespace contrib

#endif

namespace op_kernel_type_control {
ORT_SPECIFY_OP_KERNEL_ARG_DEFAULT_TYPES(
    kCpuExecutionProvider, kOnnxDomain, Pad, 2, Input, 0,
    float,
    double);

ORT_SPECIFY_OP_KERNEL_ARG_DEFAULT_TYPES(
    kCpuExecutionProvider, kOnnxDomain, Pad, 11, Input, 0,
    float,
    double,
    int32_t,
    int64_t,
    uint32_t,
    uint64_t,
    int8_t,
    uint8_t);

ORT_SPECIFY_OP_KERNEL_ARG_DEFAULT_TYPES(
    kCpuExecutionProvider, kOnnxDomain, Pad, 13, Input, 0,
    float,
    double,
    int32_t,
    int64_t,
    uint32_t,
    uint64_t,
    int8_t,
    uint8_t,
    bool);

ORT_SPECIFY_OP_KERNEL_ARG_DEFAULT_TYPES(
    kCpuExecutionProvider, kOnnxDomain, Pad, 18, Input, 0,
    float,
    double,
    int32_t,
    int64_t,
    uint32_t,
    uint64_t,
    int8_t,
    uint8_t,
    bool);

ORT_SPECIFY_OP_KERNEL_ARG_DEFAULT_TYPES(
    kCpuExecutionProvider, kOnnxDomain, Pad, 19, Input, 0,
    float,
    double,
    int32_t,
    int64_t,
    uint32_t,
    uint64_t,
    int8_t,
    uint8_t,
    bool);

// Opset 21 added int4 and uint4.
// TODO(adrianlizarraga): Implement int4 and uint4 support.
ORT_SPECIFY_OP_KERNEL_ARG_DEFAULT_TYPES(
    kCpuExecutionProvider, kOnnxDomain, Pad, 21, Input, 0,
    float,
    double,
    int32_t,
    int64_t,
    uint32_t,
    uint64_t,
    int8_t,
    uint8_t,
    bool);

// Opset 23 added support for float4e2m1.
// TODO(titaiwang): Add support for float4e2m1.
ORT_SPECIFY_OP_KERNEL_ARG_DEFAULT_TYPES(
    kCpuExecutionProvider, kOnnxDomain, Pad, 23, Input, 0,
    float,
    double,
    int32_t,
    int64_t,
    uint32_t,
    uint64_t,
    int8_t,
    uint8_t,
    bool);

ORT_SPECIFY_OP_KERNEL_ARG_REQUIRED_TYPES(
    kCpuExecutionProvider, kOnnxDomain, Pad, 11, Input, 0, int32_t, int64_t);
ORT_SPECIFY_OP_KERNEL_ARG_REQUIRED_TYPES(
    kCpuExecutionProvider, kOnnxDomain, Pad, 13, Input, 0, int32_t, int64_t);
ORT_SPECIFY_OP_KERNEL_ARG_REQUIRED_TYPES(
    kCpuExecutionProvider, kOnnxDomain, Pad, 18, Input, 0, int32_t, int64_t);
ORT_SPECIFY_OP_KERNEL_ARG_REQUIRED_TYPES(
    kCpuExecutionProvider, kOnnxDomain, Pad, 19, Input, 0, int32_t, int64_t);
ORT_SPECIFY_OP_KERNEL_ARG_REQUIRED_TYPES(
    kCpuExecutionProvider, kOnnxDomain, Pad, 21, Input, 0, int32_t, int64_t);
ORT_SPECIFY_OP_KERNEL_ARG_REQUIRED_TYPES(
    kCpuExecutionProvider, kOnnxDomain, Pad, 23, Input, 0, int32_t, int64_t);
}  // namespace op_kernel_type_control

using EnabledPad2Types = ORT_OP_KERNEL_ARG_ENABLED_TYPE_LIST(
    kCpuExecutionProvider, kOnnxDomain, Pad, 2, Input, 0);
using EnabledPad11Types = ORT_OP_KERNEL_ARG_ENABLED_TYPE_LIST(
    kCpuExecutionProvider, kOnnxDomain, Pad, 11, Input, 0);
using EnabledPad13Types = ORT_OP_KERNEL_ARG_ENABLED_TYPE_LIST(
    kCpuExecutionProvider, kOnnxDomain, Pad, 13, Input, 0);
using EnabledPad18Types = ORT_OP_KERNEL_ARG_ENABLED_TYPE_LIST(
    kCpuExecutionProvider, kOnnxDomain, Pad, 18, Input, 0);
using EnabledPad19Types = ORT_OP_KERNEL_ARG_ENABLED_TYPE_LIST(
    kCpuExecutionProvider, kOnnxDomain, Pad, 19, Input, 0);
using EnabledPad21Types = ORT_OP_KERNEL_ARG_ENABLED_TYPE_LIST(
    kCpuExecutionProvider, kOnnxDomain, Pad, 21, Input, 0);
using EnabledPad23Types = ORT_OP_KERNEL_ARG_ENABLED_TYPE_LIST(
    kCpuExecutionProvider, kOnnxDomain, Pad, 23, Input, 0);

using AllEnabledPadTypes =
    utils::TypeSetUnion<
        EnabledPad2Types,
        EnabledPad11Types,
        EnabledPad13Types>;

// only float type is supported for opset-10
ONNX_CPU_OPERATOR_VERSIONED_KERNEL(
    Pad,
    2, 10,
    KernelDefBuilder().TypeConstraint(
        "T",
        BuildKernelDefConstraintsFromTypeList<EnabledPad2Types>()),
    Pad);

// The interface for the 'Pad' op was changed in opset-11
// 'pads' and 'value' (attributes previously) became inputs in this version
// The core logic remains the same

ONNX_CPU_OPERATOR_VERSIONED_KERNEL(
    Pad,
    11, 12,
    KernelDefBuilder().TypeConstraint(
        "T",
        BuildKernelDefConstraintsFromTypeList<EnabledPad11Types>()),
    Pad);

ONNX_CPU_OPERATOR_VERSIONED_KERNEL(
    Pad,
    13, 17,
    KernelDefBuilder().TypeConstraint(
        "T",
        BuildKernelDefConstraintsFromTypeList<EnabledPad13Types>()),
    Pad);

ONNX_CPU_OPERATOR_VERSIONED_KERNEL(
    Pad,
    18, 18,
    KernelDefBuilder()
        .TypeConstraint(
            "T",
            BuildKernelDefConstraintsFromTypeList<EnabledPad18Types>()),
    Pad);

ONNX_CPU_OPERATOR_VERSIONED_KERNEL(
    Pad,
    19, 20,
    KernelDefBuilder()
        .TypeConstraint(
            "T",
            BuildKernelDefConstraintsFromTypeList<EnabledPad19Types>()),
    Pad);

ONNX_CPU_OPERATOR_VERSIONED_KERNEL(
    Pad,
    21, 22,
    KernelDefBuilder()
        .TypeConstraint(
            "T",
            BuildKernelDefConstraintsFromTypeList<EnabledPad21Types>()),
    Pad);

ONNX_CPU_OPERATOR_KERNEL(
    Pad,
    23,
    KernelDefBuilder()
        .TypeConstraint(
            "T",
            BuildKernelDefConstraintsFromTypeList<EnabledPad23Types>()),
    Pad);

using PadsVector = PadBase::PadsVector;

Status PadBase::HandleDimValueZero(const Mode& mode, const TensorShape& input_shape, const TensorShape& output_shape) {
  switch (mode) {
    case Mode::Constant: {
      // default behavior is fine
      break;
    }
    case Mode::Edge: {
      // match numpy behavior of failing if mode is 'edge' and there's an attempt to pad a dimension with value of 0
      for (size_t i = 0, end = input_shape.NumDimensions(); i < end; ++i) {
        if (input_shape[i] == 0 && output_shape[i] > 0) {
          return ORT_MAKE_STATUS(ONNXRUNTIME, FAIL,
                                 "Cannot use 'edge' mode to pad dimension with a value of 0. Input shape:",
                                 input_shape);
        }
      }
      break;
    }
    case Mode::Reflect: {
      // match numpy behavior of failing if mode is 'reflect' and there's an attempt to pad a dimension with value of 0
      for (size_t i = 0, end = input_shape.NumDimensions(); i < end; ++i) {
        if (input_shape[i] == 0 && output_shape[i] > 0) {
          return ORT_MAKE_STATUS(ONNXRUNTIME, FAIL,
                                 "Cannot use 'reflect' mode to pad dimension with a value of 0. Input shape:",
                                 input_shape);
        }
      }
      break;
    }
    default:
      return ORT_MAKE_STATUS(ONNXRUNTIME, FAIL, "Unexpected mode of ", static_cast<int>(mode));
  }

  return Status::OK();
}

static void ComputePadWithAxes(
    gsl::span<const int64_t> pads_tensor_raw_data,
    std::function<int64_t(size_t)> get_axis,
    size_t axes_size,
    size_t data_rank,
    PadsVector& pads) {
  for (size_t i = 0; i < axes_size; ++i) {
    const size_t axis = onnxruntime::narrow<size_t>(HandleNegativeAxis(get_axis(i), data_rank));
    pads[axis] = pads_tensor_raw_data[i];                          // xi_begin
    pads[data_rank + axis] = pads_tensor_raw_data[axes_size + i];  // xi_end
  }
}

void PadBase::ComputePads(OpKernelContext& ctx, size_t data_rank, gsl::span<const int64_t> pads_data,
                          PadsVector& pads) {
  pads.reserve(2 * data_rank);
  const Tensor* axes_tensor = ctx.Input<Tensor>(3);
  if (axes_tensor) {
    const size_t num_axes_dims = axes_tensor->Shape().NumDimensions();
    ORT_ENFORCE(num_axes_dims == 1, "Axes tensor should be a 1D tensor ");

    const int64_t num_axes = axes_tensor->Shape().Size();
    ORT_ENFORCE(pads_data.size() == narrow<size_t>(2 * num_axes),
                "Pads tensor size should be equal to twice the number of explicitly provided axes.");

    pads.resize(2 * data_rank, 0);
    if (axes_tensor->IsDataType<int32_t>()) {
      auto axes_data = axes_tensor->DataAsSpan<int32_t>();
      ComputePadWithAxes(
          pads_data,
          [axes_data](size_t idx) -> int64_t {
            return axes_data[idx];
          },
          axes_data.size(),
          data_rank,
          pads);
    } else if (axes_tensor->IsDataType<int64_t>()) {
      auto axes_data = axes_tensor->DataAsSpan<int64_t>();
      ComputePadWithAxes(
          pads_data,
          [axes_data](size_t idx) {
            return axes_data[idx];
          },
          axes_data.size(),
          data_rank,
          pads);
    }
  } else {
    ORT_ENFORCE(pads_data.size() == 2 * data_rank,
                "Pads tensor size should be equal to twice the input dimension count ");
    pads.assign(pads_data.begin(), pads_data.end());
  }
}

// Flatten no padding inner most Axis, so one memcpy cover multiple Axis.
// For example, for a shape of [1,224,224,3] with padding [0,3,3,0,0,3,3,0], can be flatten as
// [1,224,224*3] with padding [0,3,3*3,0,3,3*3].
void PadBase::FlattenInnerShape(gsl::span<const int64_t> input_dims, gsl::span<const int64_t> pads,
                                gsl::span<const int64_t> slices, TensorShapeVector& reshaped_dims) {
  const size_t dims_count = input_dims.size();
  size_t inner_axis = dims_count - 1;
  size_t inner_size = 1;

  // Find all inner most dimensions that can be flattened.
  do {
    inner_size *= static_cast<size_t>(input_dims[inner_axis]);

    if (inner_axis == 0)
      break;

    // Break on first Axis that has padding
    if (!(pads[inner_axis] == 0 && pads[inner_axis + dims_count] == 0 &&
          slices[inner_axis] == 0 && slices[inner_axis + dims_count] == 0))
      break;

  } while (inner_axis-- > 0);

  reshaped_dims.reserve(inner_axis + 1);
  std::copy(input_dims.begin(), input_dims.begin() + inner_axis + 1, std::back_inserter(reshaped_dims));

  // Flatten inner axis.
  reshaped_dims[inner_axis] = inner_size;
}

void PadBase::ReshapePads(gsl::span<const int64_t> src_pad, size_t src_dim_count, size_t new_dim_count,
                          size_t inner_no_pad_size, PadsVector& reshaped_pad) {
  size_t inner_axis = new_dim_count - 1;
  std::copy(src_pad.begin(), src_pad.begin() + inner_axis, reshaped_pad.begin());
  std::copy(src_pad.begin() + src_dim_count, src_pad.begin() + src_dim_count + inner_axis,
            reshaped_pad.begin() + new_dim_count);

  // Flatten inner axis.
  reshaped_pad[inner_axis] = src_pad[inner_axis] * inner_no_pad_size;
  reshaped_pad[inner_axis + new_dim_count] = src_pad[inner_axis + src_dim_count] * inner_no_pad_size;
}

// special handling for edge case where the input has one or more dims with value of 0
template <typename T>
static Status PadInputWithDimValueOfZero(OpKernelContext* ctx,
                                         const Mode& mode,
                                         const TensorShape& input_shape,
                                         TensorShapeVector& output_dims,
                                         T value) {
  TensorShape output_shape(output_dims);
  ORT_RETURN_IF_ERROR(PadBase::HandleDimValueZero(mode, input_shape, output_shape));

  auto& output_tensor = *ctx->Output(0, output_shape);

  // we need to add pads if mode is constant, otherwise the output has one or more dim values of 0 so is empty
  if (mode == Mode::Constant) {
    // we add pads with the default value to all dims including those with a value of 0
    auto* output = reinterpret_cast<T*>(output_tensor.MutableDataRaw());
    std::fill_n(output, output_shape.Size(), value);
  }

  return Status::OK();
}

// This is the general padding method to n-dimensionally do edge or reflection padding (based on the inputDelta values)
template <typename T>
static void PadAxis(T* output, T* input, ptrdiff_t input_delta, ptrdiff_t input_pitch,
                    size_t block_size, size_t block_count) {
  for (size_t block_index = 0; block_index < block_count; block_index++) {
    for (size_t i = 0; i < block_size; i++) {
      *output++ = *input;
      input += input_delta;
    }
    input += input_pitch;
  }
}

// These are optimizations of PadAxis. The inner loop is removed since the innermost axis has a blockSize of 1,
// and inputPitch and inputDelta are just a single value added each iteration.
template <typename T>
static void PadInnermostAxis(T* output, T* input, ptrdiff_t input_delta, size_t block_count) {
  for (size_t block_index = 0; block_index < block_count; block_index++) {
    *output++ = *input;
    input += input_delta;
  }
}

// For constant padding, there is no input, just a size to write the constant to
template <typename T>
static void PadAxisConstant(T* output, T constant, size_t size) {
  if (size == 1) {
    *output = constant;
  } else if (size == 2) {
    *output = constant;
    *(output + 1) = constant;
  } else {
    // This would be faster with SSE instructions.
    // That would mean to have an implementation for each type (uint8, uint32, uint64).
    T* end = output + size;
    for (; output != end;)
      *output++ = constant;
  }
}

template <typename T>
static Status PadImpl(OpKernelContext* ctx,
                      const PadsVector& pads,
                      const PadsVector& slices,
                      const Mode& mode,
                      T value) {
  if (!utils::HasTypeWithSameSize<AllEnabledPadTypes, T>()) {
    return ORT_MAKE_STATUS(ONNXRUNTIME, FAIL, "Input data type not supported in this build.");
  }

  const auto& input_tensor = *ctx->Input<Tensor>(0);
  const auto& orig_input_shape = input_tensor.Shape();
  auto output_dims(orig_input_shape.AsShapeVector());
  size_t data_rank = output_dims.size();

  // make copy of raw_pads as it may be mutated below
  ORT_ENFORCE(data_rank > 0, "Input tensor has no dimensions");
  ORT_ENFORCE(data_rank * 2 == pads.size(), "'pads' has wrong number of values");

  // Reshape input dims
  TensorShapeVector reshaped_input_dims;
  PadBase::FlattenInnerShape(output_dims, pads, slices, reshaped_input_dims);

  // Reshape padding
  size_t new_dims_count = reshaped_input_dims.size();
  size_t inner_axis = new_dims_count - 1;
  size_t inner_no_pad_size = onnxruntime::narrow<size_t>(output_dims[inner_axis] > 0
                                                             ? reshaped_input_dims[inner_axis] / output_dims[inner_axis]
                                                             : 0);
  PadsVector reshaped_pad(2 * new_dims_count), reshaped_slice(2 * new_dims_count);
  PadBase::ReshapePads(pads, data_rank, new_dims_count, inner_no_pad_size, reshaped_pad);
  PadBase::ReshapePads(slices, data_rank, new_dims_count, inner_no_pad_size, reshaped_slice);

  TensorShapeVector reshaped_output_dims = reshaped_input_dims;
  TensorShapeVector input_starts;
  TensorShapeVector input_extents;

  // Calculate output dimensions, and handle any negative padding
  input_starts.reserve(new_dims_count);
  input_extents.reserve(new_dims_count);
  for (size_t i = 0; i < new_dims_count; i++) {
    input_starts.push_back(-1 * reshaped_slice[i]);
    input_extents.push_back(reshaped_input_dims[i] + reshaped_slice[i] + reshaped_slice[i + new_dims_count]);
    reshaped_output_dims[i] += reshaped_pad[i] + reshaped_pad[i + new_dims_count] +
                               reshaped_slice[i] + reshaped_slice[i + new_dims_count];
  }

  for (size_t i = 0; i < data_rank; i++) {
    output_dims[i] += pads[i] + pads[i + data_rank] + slices[i] + slices[i + data_rank];
  }

  // special case an input with one or more dim values of 0. edge case that is easier to handle
  // separately than to complicate all the code for normal usage.
  if (orig_input_shape.Size() == 0) {
    return PadInputWithDimValueOfZero(ctx, mode, orig_input_shape, output_dims, value);
  }

  TensorShape input_shape(reshaped_input_dims);
  SliceIterator<T> input(input_tensor, input_shape, input_starts, input_extents, {});

  // output_shape need to keep original.
  TensorShape output_shape(output_dims);
  auto& output_tensor = *ctx->Output(0, output_shape);
  auto* output = reinterpret_cast<T*>(output_tensor.MutableDataRaw());

  TensorPitches output_pitches(reshaped_output_dims);
  size_t alignSkip = 0;  // Amount to skip to align to where the next input tensor data needs to be written

  // Initial skip, sum up the begin padding on each axis
  for (size_t i = 0; i < new_dims_count; i++)
    alignSkip += SafeInt<size_t>(reshaped_pad[i]) * output_pitches[i];

  ExtentAxisCounters input_counters(input_extents);

  switch (mode) {
    case Mode::Constant:
      // Loop over the output tensor, writing out padding between the blocks of copied data
      // On loop entry, 'pad' is already set to the first continuous block of padding, and
      // after every pass through the inner loop it gets set to the next continuous pad size.
      while (input_counters) {
        output += alignSkip;
        {
          T* axisStart = output;
          output = input.CopyInnermostAxisSolitaryInnerStep(output);

          int64_t prePad = reshaped_pad[inner_axis];
          int64_t postPad = reshaped_pad[inner_axis + new_dims_count];
          PadAxisConstant(axisStart - prePad, value, onnxruntime::narrow<size_t>(prePad));
          PadAxisConstant(output, value, onnxruntime::narrow<size_t>(postPad));
          output += postPad;
          alignSkip = onnxruntime::narrow<size_t>(prePad);
        }
        // Calculate the size of the next block of padding (skipping over the innermost axis since that's already done)
        while (input_counters.Increment()) {
          ptrdiff_t inner_pitch = onnxruntime::narrow<std::ptrdiff_t>(output_pitches[input_counters.Axis()]);
          T* axisStart = output - inner_pitch * input_extents[input_counters.Axis()];
          int64_t prePad = reshaped_pad[input_counters.Axis()];
          int64_t postPad = reshaped_pad[input_counters.Axis() + new_dims_count];
          PadAxisConstant(axisStart - prePad * inner_pitch, value, SafeInt<std::ptrdiff_t>(prePad) * inner_pitch);
          PadAxisConstant(output, value, SafeInt<ptrdiff_t>(postPad) * inner_pitch);
          output += inner_pitch * postPad;
          alignSkip += inner_pitch * SafeInt<size_t>(prePad);
        }
      }
      break;

    case Mode::Edge:
      // Loop over the output tensor, writing out padding between the blocks of copied data
      // On loop entry, 'pad' is already set to the first continuous block of padding, and
      // after every pass through the inner loop it gets set to the next continuous pad size.
      while (input_counters) {
        output += alignSkip;
        {
          T* axisStart = output;
          output = input.CopyInnermostAxisSolitaryInnerStep(output);

          int64_t prePad = reshaped_pad[inner_axis];
          int64_t postPad = reshaped_pad[inner_axis + new_dims_count];
          if (inner_no_pad_size == 1) {
            PadAxisConstant(axisStart - prePad, *axisStart, onnxruntime::narrow<size_t>(prePad));
            PadAxisConstant(output, *(output - 1), onnxruntime::narrow<size_t>(postPad));
          } else {
            // When inner_most axis(es) do not need pad, above PadAxisConstant() do not fit for Edge mode.
            // Also general loop below after handling first pad axis with non-pad axis works fine.
            PadAxis(axisStart - prePad, axisStart, 1, -ptrdiff_t(inner_no_pad_size), inner_no_pad_size, onnxruntime::narrow<size_t>(pads[inner_axis]));
            PadAxis(output, output - inner_no_pad_size, 1, -ptrdiff_t(inner_no_pad_size), inner_no_pad_size, onnxruntime::narrow<size_t>(pads[inner_axis + data_rank]));
          }
          output += postPad;
          alignSkip = onnxruntime::narrow<size_t>(prePad);
        }
        // Calculate the size of the next block of padding (skipping over the innermost axis since that's already done)
        while (input_counters.Increment()) {
          ptrdiff_t inner_pitch = onnxruntime::narrow<std::ptrdiff_t>(output_pitches[input_counters.Axis()]);
          T* axisStart = output - inner_pitch * input_extents[input_counters.Axis()];
          int64_t prePad = reshaped_pad[input_counters.Axis()];
          int64_t postPad = reshaped_pad[input_counters.Axis() + new_dims_count];
          PadAxis(axisStart - prePad * inner_pitch, axisStart, 1, -inner_pitch, inner_pitch, onnxruntime::narrow<size_t>(prePad));
          PadAxis(output, output - inner_pitch, 1, -inner_pitch, inner_pitch, onnxruntime::narrow<size_t>(postPad));
          output += inner_pitch * postPad;
          alignSkip += inner_pitch * SafeInt<size_t>(prePad);
        }
      }
      break;

    case Mode::Reflect:
    case Mode::Wrap:
      // Loop over the output tensor, writing out padding between the blocks of copied data
      // On loop entry, 'pad' is already set to the first continuous block of padding, and
      // after every pass through the inner loop it gets set to the next continuous pad size.
      while (input_counters) {
        output += alignSkip;
        {
          T* axisStart = output;
          output = input.CopyInnermostAxisSolitaryInnerStep(output);

          int64_t prePad = reshaped_pad[inner_axis];
          int64_t postPad = reshaped_pad[inner_axis + new_dims_count];
          if (inner_no_pad_size == 1) {
            if (mode == Mode::Reflect) {
              PadInnermostAxis(axisStart - prePad, axisStart + prePad, -1 /* inputDelta */, onnxruntime::narrow<size_t>(prePad));
              PadInnermostAxis(output, output - 2, -1 /* inputDelta */, onnxruntime::narrow<size_t>(postPad));
            } else {
              PadInnermostAxis(axisStart - prePad, output - prePad, 1 /* inputDelta */, onnxruntime::narrow<size_t>(prePad));
              PadInnermostAxis(output, axisStart, 1 /* inputDelta */, onnxruntime::narrow<size_t>(postPad));
            }
          } else {
            // When inner_most axis(es) do not need pad, Above PadInnermostAxis() do not fit for Reflect mode.
            if (mode == Mode::Reflect) {
              PadAxis(
                  axisStart - prePad,
                  axisStart + prePad,
                  1,
                  -ptrdiff_t(inner_no_pad_size * 2),
                  inner_no_pad_size,
                  onnxruntime::narrow<size_t>(pads[inner_axis]));
              PadAxis(
                  output,
                  output - 2 * inner_no_pad_size,
                  1,
                  -ptrdiff_t(inner_no_pad_size * 2),
                  inner_no_pad_size,
                  onnxruntime::narrow<size_t>(pads[inner_axis + data_rank]));
            } else {
              PadAxis(
                  axisStart - prePad,
                  output - pads[inner_axis] * inner_no_pad_size,
                  1,
                  0,
                  inner_no_pad_size,
                  onnxruntime::narrow<size_t>(pads[inner_axis]));
              PadAxis(
                  output,
                  axisStart,
                  1,
                  0,
                  inner_no_pad_size,
                  onnxruntime::narrow<size_t>(pads[inner_axis + data_rank]));
            }
          }
          output += postPad;
          alignSkip = onnxruntime::narrow<size_t>(prePad);
        }
        // Calculate the size of the next block of padding (skipping over the innermost axis since that's already done)
        while (input_counters.Increment()) {
          ptrdiff_t inner_pitch = onnxruntime::narrow<std::ptrdiff_t>(output_pitches[input_counters.Axis()]);
          T* axisStart = output - inner_pitch * input_extents[input_counters.Axis()];
          int64_t prePad = reshaped_pad[input_counters.Axis()];
          int64_t postPad = reshaped_pad[input_counters.Axis() + new_dims_count];
          if (mode == Mode::Reflect) {
            PadAxis(
                axisStart - prePad * inner_pitch,
                axisStart + prePad * inner_pitch,
                1,
                -inner_pitch * 2,
                inner_pitch,
                onnxruntime::narrow<size_t>(prePad));
            PadAxis(
                output,
                output - 2 * inner_pitch,
                1,
                -inner_pitch * 2,
                inner_pitch,
                onnxruntime::narrow<size_t>(postPad));
          } else {
            PadAxis(
                axisStart - prePad * inner_pitch,
                output - prePad * inner_pitch,
                1,
                0,
                inner_pitch,
                onnxruntime::narrow<size_t>(prePad));
            PadAxis(
                output,
                axisStart,
                1,
                0,
                inner_pitch,
                onnxruntime::narrow<size_t>(postPad));
          }
          output += inner_pitch * postPad;
          alignSkip += inner_pitch * SafeInt<size_t>(prePad);
        }
      }
      break;
  }

  return Status::OK();
}

union PadValue {
  uint64_t u64;
  uint32_t u32;
  uint8_t u8;
  double f64;
  float f32;
};

static PadValue PadValueFromFloat(float value, MLDataType data_type) {
  PadValue result;
  if (data_type == DataTypeImpl::GetType<float>()) {
    result.f32 = value;
  } else if (data_type == DataTypeImpl::GetType<double>()) {
    result.f64 = value;
  } else {
    ORT_THROW("Unsupported input data type of ", data_type);
  }
  return result;
}

Status Pad::Compute(OpKernelContext* ctx) const {
  const Tensor& input_tensor = *ctx->Input<Tensor>(0);
  MLDataType data_type = input_tensor.DataType();
  const auto element_size = data_type->Size();
  PadsVector pads;
  PadsVector slices;
  const PadsVector* pads_to_use;
  const PadsVector* slices_to_use;
  PadValue value;

  // kOnnxDomain Pad opset >= 11 (Or) kMsDomain opset == 1
  if (is_dynamic_) {
    size_t data_rank = input_tensor.Shape().NumDimensions();

    const Tensor& pads_tensor = *ctx->Input<Tensor>(1);
    auto pads_tensor_dims = pads_tensor.Shape().GetDims();
    ORT_ENFORCE(pads_tensor_dims.size() == 1 || (pads_tensor_dims.size() == 2 && pads_tensor_dims[0] == 1),
                "Pads tensor should be a 1D tensor of shape [2 * num_axes] "
                "or a 2D tensor of shape [1, 2 * num_axes]");

    const auto pads_data = pads_tensor.DataAsSpan<int64_t>();

    // Compute Pads by applying axes if specified otherwise copy the supplied pads.
    PadBase::ComputePads(*ctx, data_rank, pads_data, pads);

    // Separate out any negative pads into the slices array
    PadBase::SeparateNegativeToSlices(pads, slices);

    value.u64 = 0U;
    const Tensor* value_tensor = ctx->Input<Tensor>(2);
    if (nullptr != value_tensor) {
      ORT_ENFORCE(value_tensor->DataType() == data_type &&
                      value_tensor->Shape().Size() == 1,
                  "Value tensor should be a 1D tensor of size 1 with the same type as that of the input tensor");
      const void* value_data = value_tensor->DataRaw();
      switch (element_size) {
        case sizeof(uint32_t):
          value.u32 = reinterpret_cast<const uint32_t*>(value_data)[0];
          break;
        case sizeof(uint64_t):
          value.u64 = reinterpret_cast<const uint64_t*>(value_data)[0];
          break;
        case sizeof(uint8_t):
          value.u8 = reinterpret_cast<const uint8_t*>(value_data)[0];
          break;
        default:
          ORT_THROW("Unsupported input data type of ", data_type);
      }
    }

    pads_to_use = &pads;
    slices_to_use = &slices;
  } else {
    // kOnnxDomain Pad opset < 11
    // In the earlier opset versions of Pad, the type for 'value' attribute was always float,
    // irrespective of the data type of the actual input to be padded
    value = PadValueFromFloat(value_, data_type);
    pads_to_use = &pads_;
    slices_to_use = &slices_;
  }

  Status pad_status{};
  switch (element_size) {
    case sizeof(uint32_t):
      pad_status = PadImpl<uint32_t>(ctx, *pads_to_use, *slices_to_use, mode_, value.u32);
      break;
    case sizeof(uint64_t):
      pad_status = PadImpl<uint64_t>(ctx, *pads_to_use, *slices_to_use, mode_, value.u64);
      break;
    case sizeof(uint8_t):
      pad_status = PadImpl<uint8_t>(ctx, *pads_to_use, *slices_to_use, mode_, value.u8);
      break;
    default:
      pad_status = ORT_MAKE_STATUS(ONNXRUNTIME, FAIL, "Unsupported input data type of ", data_type);
      break;
  }
  return pad_status;
}
};  // namespace onnxruntime
