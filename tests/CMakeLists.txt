find_package(Qt6 ${REQUIRED_QT_VERSION} CONFIG REQUIRED Test)

include(ECMMarkAsTest)

macro(skanlite_executable_tests)
  foreach(_testname ${ARGN})
    add_executable(${_testname} ${_testname}.cpp)
    target_link_libraries(${_testname} Qt6::Test KF5skanlite)
    ecm_mark_as_test(${_testname})
  endforeach(_testname)
endmacro()

#skanlite_executable_tests(
#  anothertest
#)
