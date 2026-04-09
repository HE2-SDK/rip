target_sources(rip-hl
    PRIVATE
        "rip/binary/containers/swif/SWIF.cpp"
    PUBLIC FILE_SET HEADERS FILES
        "rip/util/byteswap.h"
        "rip/util/memory.h"
        "rip/binary/stream.h"
        "rip/binary/types.h"
        
        "rip/binary/containers/binary-file/common.h"
        "rip/binary/containers/binary-file/v1.h"
        "rip/binary/containers/binary-file/v2.h"
        "rip/binary/containers/swif/SWIF.h"
        "rip/binary/serialization/BlobWorker.h"
        "rip/binary/serialization/JsonSerializer.h"
        "rip/binary/serialization/JsonDeserializer.h"
        "rip/binary/serialization/ReflectionSerializer.h"
        "rip/binary/serialization/ReflectionDeserializer.h"
        "rip/hson/HsonSerializer.h"
        "rip/hson/HsonDeserializer.h"
        "rip/schemas/hedgeset.h"
 "rip/binary/serialization/ReflectCppSerializer.h" "rip/util/object-id-guids.h" "rip/binary/containers/mirage/v1.h" "rip/binary/containers/mirage/v2.h" "rip/hson/JsonReflections.h" "rip/util/math.h" "rip/binary/containers/needle-archive/v1.h" "rip/binary/accessors/binary-stream.h" "rip/binary/accessors/json.h" "rip/binary/containers/ntsi/v1.h" "rip/binary/containers/ntsp/v1.h" "rip/binary/serialization2/binary.h" "rip/binary/serialization2/BlobWorker.h")
