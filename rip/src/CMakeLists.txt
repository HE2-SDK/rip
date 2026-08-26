function(add_resource_version RIP_RESOURCE RIP_VERSION)
    configure_file(convert-impl.in.cpp convert-${RIP_RESOURCE}-${RIP_VERSION}.cpp)
    target_sources(rip PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/convert-${RIP_RESOURCE}-${RIP_VERSION}.cpp)
endfunction()

add_resource_version("animation_state_machine" "1.03-rangers")
add_resource_version("animation_state_machine" "1.03-miller")
add_resource_version("gedit" "2")
add_resource_version("gedit" "3")
add_resource_version("map" "1")
add_resource_version("material" "3-container-v1")
add_resource_version("material" "3")
add_resource_version("rfl" "1")
add_resource_version("rfl" "2-1.00")
add_resource_version("vat" "1-rangers")
add_resource_version("vat" "1-miller")
add_resource_version("fxcol" "1")
add_resource_version("svcol" "1")
add_resource_version("swif" "1")
add_resource_version("swif" "2")
add_resource_version("swif" "3")
add_resource_version("swif" "3-be")
add_resource_version("swif" "4")
add_resource_version("swif" "5")
add_resource_version("swif" "6")
add_resource_version("sobj" "1-colors")
add_resource_version("sobj" "1-scu")
add_resource_version("nxs" "1")
add_resource_version("path" "1")
add_resource_version("path" "2.00")
add_resource_version("pcmodel" "2")
add_resource_version("mlevel" "0")
add_resource_version("density_setting" "11")
add_resource_version("aism" "0")
add_resource_version("cemt" "16")
add_resource_version("cemt" "18")
add_resource_version("effdb" "100")

target_sources(rip
    PRIVATE
        "main.cpp"
        "config.cpp"
        "convert.cpp"
    PRIVATE FILE_SET HEADERS FILES
        "config.h"
        "convert.h"
        "util.h"
        "resource-table.h"
)
