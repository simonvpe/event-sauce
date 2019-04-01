add_executable(execute-test execute-test.cpp)
target_link_libraries(execute-test event-sauce)
set_target_properties(execute-test PROPERTIES CXX_STANDARD 17 CXX_EXTENSIONS OFF)  
add_test(NAME event-sauce/execute-test COMMAND execute-test)
