// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Serialization warnings are only recorded when DLOG is enabled.
#if !defined(NDEBUG) || defined(DCHECK_ALWAYS_ON)

#include <stddef.h>
#include <utility>

#include "mojo/public/cpp/bindings/array.h"
#include "mojo/public/cpp/bindings/lib/array_internal.h"
#include "mojo/public/cpp/bindings/lib/fixed_buffer.h"
#include "mojo/public/cpp/bindings/lib/serialization.h"
#include "mojo/public/cpp/bindings/lib/validation_errors.h"
#include "mojo/public/cpp/bindings/string.h"
#include "mojo/public/cpp/system/message_pipe.h"
#include "mojo/public/interfaces/bindings/tests/serialization_test_structs.mojom.h"
#include "mojo/public/interfaces/bindings/tests/test_unions.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace mojo {
namespace test {
namespace {

using mojo::internal::ContainerValidateParams;

// Creates an array of arrays of handles (2 X 3) for testing.
Array<Array<ScopedHandle>> CreateTestNestedHandleArray() {
  Array<Array<ScopedHandle>> array(2);
  for (size_t i = 0; i < array.size(); ++i) {
    Array<ScopedHandle> nested_array(3);
    for (size_t j = 0; j < nested_array.size(); ++j) {
      MessagePipe pipe;
      nested_array[j] = ScopedHandle::From(std::move(pipe.handle1));
    }
    array[i] = std::move(nested_array);
  }

  return array;
}

class SerializationWarningTest : public testing::Test {
 public:
  ~SerializationWarningTest() override {}

 protected:
  template <typename T>
  void TestWarning(T obj, mojo::internal::ValidationError expected_warning) {
    warning_observer_.set_last_warning(mojo::internal::VALIDATION_ERROR_NONE);

    mojo::internal::SerializationContext context;
    mojo::internal::FixedBufferForTesting buf(
        mojo::internal::PrepareToSerialize<T>(obj, &context));
    typename mojo::internal::MojomTypeTraits<T>::Data* data;
    mojo::internal::Serialize<T>(obj, &buf, &data, &context);

    EXPECT_EQ(expected_warning, warning_observer_.last_warning());
  }

  template <typename T>
  void TestArrayWarning(T obj,
                        mojo::internal::ValidationError expected_warning,
                        const ContainerValidateParams* validate_params) {
    warning_observer_.set_last_warning(mojo::internal::VALIDATION_ERROR_NONE);

    mojo::internal::SerializationContext context;
    mojo::internal::FixedBufferForTesting buf(
        mojo::internal::PrepareToSerialize<T>(obj, &context));
    typename mojo::internal::MojomTypeTraits<T>::Data* data;
    mojo::internal::Serialize<T>(obj, &buf, &data, validate_params, &context);

    EXPECT_EQ(expected_warning, warning_observer_.last_warning());
  }

  template <typename T>
  void TestUnionWarning(T obj,
                        mojo::internal::ValidationError expected_warning) {
    warning_observer_.set_last_warning(mojo::internal::VALIDATION_ERROR_NONE);

    mojo::internal::SerializationContext context;
    mojo::internal::FixedBufferForTesting buf(
        mojo::internal::PrepareToSerialize<T>(obj, false, &context));
    typename mojo::internal::MojomTypeTraits<T>::Data* data;
    mojo::internal::Serialize<T>(obj, &buf, &data, false, &context);

    EXPECT_EQ(expected_warning, warning_observer_.last_warning());
  }

  mojo::internal::SerializationWarningObserverForTesting warning_observer_;
};

TEST_F(SerializationWarningTest, HandleInStruct) {
  Struct2Ptr test_struct(Struct2::New());
  EXPECT_FALSE(test_struct->hdl.is_valid());

  TestWarning(std::move(test_struct),
              mojo::internal::VALIDATION_ERROR_UNEXPECTED_INVALID_HANDLE);

  test_struct = Struct2::New();
  MessagePipe pipe;
  test_struct->hdl = ScopedHandle::From(std::move(pipe.handle1));

  TestWarning(std::move(test_struct), mojo::internal::VALIDATION_ERROR_NONE);
}

TEST_F(SerializationWarningTest, StructInStruct) {
  Struct3Ptr test_struct(Struct3::New());
  EXPECT_TRUE(!test_struct->struct_1);

  TestWarning(std::move(test_struct),
              mojo::internal::VALIDATION_ERROR_UNEXPECTED_NULL_POINTER);

  test_struct = Struct3::New();
  test_struct->struct_1 = Struct1::New();

  TestWarning(std::move(test_struct), mojo::internal::VALIDATION_ERROR_NONE);
}

TEST_F(SerializationWarningTest, ArrayOfStructsInStruct) {
  Struct4Ptr test_struct(Struct4::New());
  EXPECT_TRUE(!test_struct->data);

  TestWarning(std::move(test_struct),
              mojo::internal::VALIDATION_ERROR_UNEXPECTED_NULL_POINTER);

  test_struct = Struct4::New();
  test_struct->data.resize(1);

  TestWarning(std::move(test_struct),
              mojo::internal::VALIDATION_ERROR_UNEXPECTED_NULL_POINTER);

  test_struct = Struct4::New();
  test_struct->data.resize(0);

  TestWarning(std::move(test_struct), mojo::internal::VALIDATION_ERROR_NONE);

  test_struct = Struct4::New();
  test_struct->data.resize(1);
  test_struct->data[0] = Struct1::New();

  TestWarning(std::move(test_struct), mojo::internal::VALIDATION_ERROR_NONE);
}

TEST_F(SerializationWarningTest, FixedArrayOfStructsInStruct) {
  Struct5Ptr test_struct(Struct5::New());
  EXPECT_TRUE(!test_struct->pair);

  TestWarning(std::move(test_struct),
              mojo::internal::VALIDATION_ERROR_UNEXPECTED_NULL_POINTER);

  test_struct = Struct5::New();
  test_struct->pair.resize(1);
  test_struct->pair[0] = Struct1::New();

  TestWarning(std::move(test_struct),
              mojo::internal::VALIDATION_ERROR_UNEXPECTED_ARRAY_HEADER);

  test_struct = Struct5::New();
  test_struct->pair.resize(2);
  test_struct->pair[0] = Struct1::New();
  test_struct->pair[1] = Struct1::New();

  TestWarning(std::move(test_struct), mojo::internal::VALIDATION_ERROR_NONE);
}

TEST_F(SerializationWarningTest, StringInStruct) {
  Struct6Ptr test_struct(Struct6::New());
  EXPECT_TRUE(!test_struct->str);

  TestWarning(std::move(test_struct),
              mojo::internal::VALIDATION_ERROR_UNEXPECTED_NULL_POINTER);

  test_struct = Struct6::New();
  test_struct->str = "hello world";

  TestWarning(std::move(test_struct), mojo::internal::VALIDATION_ERROR_NONE);
}

TEST_F(SerializationWarningTest, ArrayOfArraysOfHandles) {
  Array<Array<ScopedHandle>> test_array = CreateTestNestedHandleArray();
  test_array[0] = nullptr;
  test_array[1][0] = ScopedHandle();

  ContainerValidateParams validate_params_0(
      0, true, new ContainerValidateParams(0, true, nullptr));
  TestArrayWarning(std::move(test_array), mojo::internal::VALIDATION_ERROR_NONE,
                   &validate_params_0);

  test_array = CreateTestNestedHandleArray();
  test_array[0] = nullptr;
  ContainerValidateParams validate_params_1(
      0, false, new ContainerValidateParams(0, true, nullptr));
  TestArrayWarning(std::move(test_array),
                   mojo::internal::VALIDATION_ERROR_UNEXPECTED_NULL_POINTER,
                   &validate_params_1);

  test_array = CreateTestNestedHandleArray();
  test_array[1][0] = ScopedHandle();
  ContainerValidateParams validate_params_2(
      0, true, new ContainerValidateParams(0, false, nullptr));
  TestArrayWarning(std::move(test_array),
                   mojo::internal::VALIDATION_ERROR_UNEXPECTED_INVALID_HANDLE,
                   &validate_params_2);
}

TEST_F(SerializationWarningTest, ArrayOfStrings) {
  Array<String> test_array(3);
  for (size_t i = 0; i < test_array.size(); ++i)
    test_array[i] = "hello";

  ContainerValidateParams validate_params_0(
      0, true, new ContainerValidateParams(0, false, nullptr));
  TestArrayWarning(std::move(test_array), mojo::internal::VALIDATION_ERROR_NONE,
                   &validate_params_0);

  test_array = Array<String>(3);
  for (size_t i = 0; i < test_array.size(); ++i)
    test_array[i] = nullptr;
  ContainerValidateParams validate_params_1(
      0, false, new ContainerValidateParams(0, false, nullptr));
  TestArrayWarning(std::move(test_array),
                   mojo::internal::VALIDATION_ERROR_UNEXPECTED_NULL_POINTER,
                   &validate_params_1);

  test_array = Array<String>(2);
  ContainerValidateParams validate_params_2(
      3, true, new ContainerValidateParams(0, false, nullptr));
  TestArrayWarning(std::move(test_array),
                   mojo::internal::VALIDATION_ERROR_UNEXPECTED_ARRAY_HEADER,
                   &validate_params_2);
}

TEST_F(SerializationWarningTest, StructInUnion) {
  DummyStructPtr dummy(nullptr);
  ObjectUnionPtr obj(ObjectUnion::New());
  obj->set_f_dummy(std::move(dummy));

  TestUnionWarning(std::move(obj),
                   mojo::internal::VALIDATION_ERROR_UNEXPECTED_NULL_POINTER);
}

TEST_F(SerializationWarningTest, UnionInUnion) {
  PodUnionPtr pod(nullptr);
  ObjectUnionPtr obj(ObjectUnion::New());
  obj->set_f_pod_union(std::move(pod));

  TestUnionWarning(std::move(obj),
                   mojo::internal::VALIDATION_ERROR_UNEXPECTED_NULL_POINTER);
}

TEST_F(SerializationWarningTest, HandleInUnion) {
  ScopedMessagePipeHandle pipe;
  HandleUnionPtr handle(HandleUnion::New());
  handle->set_f_message_pipe(std::move(pipe));

  TestUnionWarning(std::move(handle),
                   mojo::internal::VALIDATION_ERROR_UNEXPECTED_INVALID_HANDLE);
}

}  // namespace
}  // namespace test
}  // namespace mojo

#endif
