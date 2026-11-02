cmake_minimum_required(VERSION 3.15)

project(telematics_gateway)

find_package(eCAL REQUIRED)
find_package(Threads REQUIRED)

# --------------------------------------------------------
# core logic (no eCAL dependency, unit tested)
# --------------------------------------------------------
add_library(telematics_core STATIC
  src/vehicle_state.cpp
  src/can/can_decoder.cpp
)
target_include_directories(telematics_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(telematics_core PUBLIC cxx_std_17)
target_link_libraries(telematics_core PUBLIC Threads::Threads)
set_property(TARGET telematics_core PROPERTY FOLDER contrib/telematics_gateway)

# --------------------------------------------------------
# eCAL processes
# They link against the TCU board support package (HSM, relays), which is
# only available in the target toolchain.
# --------------------------------------------------------
find_package(tcu_bsp QUIET)
if(tcu_bsp_FOUND)
  add_executable(tcu_can_decoder apps/can_decoder/main.cpp)
  target_link_libraries(tcu_can_decoder PRIVATE telematics_core eCAL::core)
  set_property(TARGET tcu_can_decoder PROPERTY FOLDER contrib/telematics_gateway)
endif()

# --------------------------------------------------------
# tests
# --------------------------------------------------------
if(ECAL_BUILD_TESTS)
  find_package(GTest REQUIRED)
  ecal_add_gtest(test_telematics_gateway
    tests/vehicle_state_test.cpp
    tests/can_decoder_test.cpp
  )
  target_link_libraries(test_telematics_gateway PRIVATE telematics_core)
  ecal_install_gtest(test_telematics_gateway)
  set_property(TARGET test_telematics_gateway PROPERTY FOLDER tests/cpp/contrib/telematics_gateway)
endif()
