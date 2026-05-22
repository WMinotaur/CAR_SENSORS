target_sources(app PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/sensor_thread.c
        ${CMAKE_CURRENT_SOURCE_DIR}/can_thread.c
        ${CMAKE_CURRENT_SOURCE_DIR}/distanceSensor.c
)