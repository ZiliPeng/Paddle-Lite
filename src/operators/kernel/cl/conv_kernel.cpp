/* Copyright (c) 2018 PaddlePaddle Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License. */

#ifdef CONV_OP

#include "operators/kernel/conv_kernel.h"

namespace paddle_mobile {
namespace operators {

template <>
bool ConvKernel<GPU_CL, float>::Init(ConvParam<GPU_CL> *param) {
  PADDLE_MOBILE_ENFORCE(
      param->Filter()->dims()[2] == param->Filter()->dims()[3] &&
          param->Paddings()[0] == param->Paddings()[1],
      "need equal");

  int offset = static_cast<int>(param->Filter()->dims()[2]) / 2 -
               static_cast<int>(param->Paddings()[1]);
  param->SetOffset(offset);

  if (param->Filter()->WidthOfOneBlock() == 1 &&
      param->Filter()->HeightOfOneBlock() == 1) {
    this->cl_helper_.AddKernel("conv_1x1", "conv_add_bn_relu_kernel.cl");
  } else if (param->Filter()->dims()[1] == 1) {
    this->cl_helper_.AddKernel("depth_conv_3x3", "conv_add_bn_relu_kernel.cl");
  } else if (param->Filter()->WidthOfOneBlock() == 3 &&
             param->Filter()->HeightOfOneBlock() == 3) {
    this->cl_helper_.AddKernel("conv_3x3", "conv_add_bn_relu_kernel.cl");
  } else {
    PADDLE_MOBILE_THROW_EXCEPTION(" not support ");
  }

  return true;
}

template <>
void ConvKernel<GPU_CL, float>::Compute(const ConvParam<GPU_CL> &param) {
  auto kernel = this->cl_helper_.KernelAt(0);
  auto default_work_size = this->cl_helper_.DefaultWorkSize(*param.Output());
  int c_block = default_work_size[0];
  int w = default_work_size[1];
  int nh = default_work_size[2];
  auto input = param.Input()->GetCLImage();
  auto filter = param.Filter()->GetCLImage();
  auto output = param.Output();
  int stride = param.Strides()[0];
  int offset = param.Offset();
  int input_c = param.Input()->CBlock();
  int dilation = param.Dilations()[0];
  int input_width = param.Input()->WidthOfOneBlock();
  int input_height = param.Input()->HeightOfOneBlock();

  cl_int status;

  status = clSetKernelArg(kernel, 0, sizeof(int), &c_block);
  status = clSetKernelArg(kernel, 1, sizeof(int), &w);
  status = clSetKernelArg(kernel, 2, sizeof(int), &nh);
  status = clSetKernelArg(kernel, 3, sizeof(cl_mem), &input);
  status = clSetKernelArg(kernel, 4, sizeof(cl_mem), &filter);
  status = clSetKernelArg(kernel, 5, sizeof(cl_mem), &output);
  status = clSetKernelArg(kernel, 6, sizeof(int), &stride);
  status = clSetKernelArg(kernel, 7, sizeof(int), &offset);
  status = clSetKernelArg(kernel, 8, sizeof(int), &input_c);
  status = clSetKernelArg(kernel, 9, sizeof(int), &dilation);
  status = clSetKernelArg(kernel, 10, sizeof(int), &input_width);
  status = clSetKernelArg(kernel, 11, sizeof(int), &input_height);

  CL_CHECK_ERRORS(status);

  status =
      clEnqueueNDRangeKernel(this->cl_helper_.CLCommandQueue(), kernel, 3, NULL,
                             default_work_size.data(), NULL, 0, NULL, NULL);

  CL_CHECK_ERRORS(status);
}

template class ConvKernel<GPU_CL, float>;

}  // namespace operators
}  // namespace paddle_mobile

#endif