/// @file
/// Test binding helper methods in `pydrake_pybind_test`.
/// @note `check_copy` is defind and documented in
/// `_pydrake_pybind_test_extra.py`.
#include "drake/bindings/pydrake/pydrake_pybind.h"

#include <string>

#include <gtest/gtest.h>
#include "pybind11/embed.h"
#include "pybind11/eval.h"
#include "pybind11/operators.h"
#include "pybind11/pybind11.h"

#include "drake/bindings/pydrake/test/test_util_pybind.h"
#include "drake/common/drake_copyable.h"

namespace drake {
namespace pydrake {
namespace {

// Expects that a given Python expression `expr` evalulates to true, using
// globals and the variables available in `m`.
void PyExpectTrue(py::module m, const char* expr) {
  const bool value =
      py::eval(expr, py::globals(), m.attr("__dict__")).cast<bool>();
  EXPECT_TRUE(value) << expr;
}

// TODO(eric.cousineau): Test coverage of `py_reference`,
// `py_reference_internal`, `py_keep_alive`, etc.

// Class which has a copy constructor, for testing `DefCopyAndDeepCopy`.
struct ExampleDefCopyAndDeepCopy {
  DRAKE_DEFAULT_COPY_AND_MOVE_AND_ASSIGN(ExampleDefCopyAndDeepCopy);
  int value{};
  bool operator==(const ExampleDefCopyAndDeepCopy& other) const {
    return value == other.value;
  }
};

GTEST_TEST(PydrakePybindTest, DefCopyAndDeepCopy) {
  py::module m("test");
  {
    using Class = ExampleDefCopyAndDeepCopy;
    py::class_<Class> cls(m, "ExampleDefCopyAndDeepCopy");
    cls  // BR
        .def(py::init([](int value) { return Class{value}; }))
        .def(py::self == py::self);
    DefCopyAndDeepCopy(&cls);
  }

  PyExpectTrue(m, "check_copy(copy.copy, ExampleDefCopyAndDeepCopy(10))");
  PyExpectTrue(m, "check_copy(copy.deepcopy, ExampleDefCopyAndDeepCopy(20))");
}

// Class which has a `Clone()` method and whose copy constructor is explicitly
// disabled, for testing `DefClone`.
class ExampleDefClone {
 public:
  explicit ExampleDefClone(int value) : value_(value) {}
  ExampleDefClone(ExampleDefClone&&) = delete;
  ExampleDefClone& operator=(ExampleDefClone&) = delete;

  std::unique_ptr<ExampleDefClone> Clone() const {
    return std::unique_ptr<ExampleDefClone>(new ExampleDefClone(*this));
  }

  bool operator==(const ExampleDefClone& other) const {
    return value_ == other.value_;
  }

 private:
  ExampleDefClone(const ExampleDefClone&) = default;
  ExampleDefClone& operator=(const ExampleDefClone&) = default;

  int value_{};
};

GTEST_TEST(PydrakePybindTest, DefClone) {
  py::module m("test");
  {
    using Class = ExampleDefClone;
    py::class_<Class> cls(m, "ExampleDefClone");
    cls  // BR
        .def(py::init<double>())
        .def(py::self == py::self);
    DefClone(&cls);
  }

  PyExpectTrue(m, "check_copy(ExampleDefClone.Clone, ExampleDefClone(5))");
  PyExpectTrue(m, "check_copy(copy.copy, ExampleDefClone(10))");
  PyExpectTrue(m, "check_copy(copy.deepcopy, ExampleDefClone(20))");
}

// Struct which defines attributes which are to be exposed with
// `.def_readwrite`, for testing `ParamInit`.
struct ExampleParamInit {
  int a{0};
  int b{1};
};

GTEST_TEST(PydrakePybindTest, ParamInit) {
  py::module m("test");
  {
    using Class = ExampleParamInit;
    py::class_<Class>(m, "ExampleParamInit")
        .def(ParamInit<Class>())
        .def_readwrite("a", &Class::a)
        .def_readwrite("b", &Class::b)
        // This is purely a sugar method for testing the values.
        .def("compare_values", [](const Class& self, int a, int b) {
          return self.a == a && self.b == b;
        });
  }

  PyExpectTrue(m, "ExampleParamInit().compare_values(0, 1)");
  PyExpectTrue(m, "ExampleParamInit(a=10).compare_values(10, 1)");
  PyExpectTrue(m, "ExampleParamInit(b=20).compare_values(0, 20)");
  PyExpectTrue(m, "ExampleParamInit(a=10, b=20).compare_values(10, 20)");
}

int DoMain(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  // Reconstructing `scoped_interpreter` multiple times (e.g. via `SetUp()`)
  // while *also* importing `numpy` wreaks havoc.
  py::scoped_interpreter guard;
  // Define nominal scope, and use a useful name for `ExecuteExtraPythonCode`
  // below.
  py::module m("pydrake.test.pydrake_pybind_test");
  // Test coverage and use this method for `check_copy`.
  ExecuteExtraPythonCode(m);
  test::SynchronizeGlobalsForPython3(m);
  return RUN_ALL_TESTS();
}

}  // namespace
}  // namespace pydrake
}  // namespace drake

int main(int argc, char** argv) {
  return drake::pydrake::DoMain(argc, argv);
}
