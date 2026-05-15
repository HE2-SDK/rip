set(RIP_RESOURCES
    animation_state_machine
    # gedit
    # map
    # material
    # rfl
    # vat
    # fxcol
    # svcol
    # swif
    # sobj
    # nxs
    # path
    # pcmodel
    # mlevel
    # density_setting
    # aism
)

foreach(RIP_RESOURCE IN LISTS RIP_RESOURCES)
    configure_file(convert-impl.in.cpp convert-${RIP_RESOURCE}.cpp)
endforeach()

list(TRANSFORM RIP_RESOURCES PREPEND ${CMAKE_CURRENT_BINARY_DIR}/convert-)
list(TRANSFORM RIP_RESOURCES APPEND .cpp)

target_sources(rip
    PRIVATE
        "main.cpp"
        "config.cpp"
        "convert.cpp"
        ${RIP_RESOURCES}
    PRIVATE FILE_SET HEADERS FILES
        "io/InputFile.h"
        "io/BinaryInputFile.h"
        "io/MirageInputFile.h"
        "io/SWIFInputFile.h"
        "io/JsonInputFile.h"
        "io/load_input.h"
        "io/write_output.h"
        "io/mem_stream.h"
        "io/load_hedgeset_template.h"
        "config.h"
        "convert.h"
        "util.h"
        "resource-table.h"
)
