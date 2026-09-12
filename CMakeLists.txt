set(srcs "table256.cpp" "RSEncode16.cpp")
if(CONFIG_RS_PIE)
    list(APPEND srcs "RSEncode16_pie.S")
endif()

idf_component_register(SRCS ${srcs}
                    INCLUDE_DIRS "include")
