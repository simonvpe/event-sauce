add_executable(simple-event-dispatching simple-event-dispatching.cpp)
target_link_libraries(simple-event-dispatching event-sauce)
set_target_properties(simple-event-dispatching PROPERTIES CXX_STANDARD 17 CXX_EXTENSIONS OFF)  
add_test(NAME event-sauce/simple-event-dispatching COMMAND simple-event-dispatching)
