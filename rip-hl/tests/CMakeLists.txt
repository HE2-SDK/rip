target_sources(rip-hl-tests
    PRIVATE
        binary/containers/binary-file/common.cpp
 "../src/rip/serialization/json/binary-file.h" "../src/rip/serialization/binary/binary-file.h" "../src/rip/serialization/binary/mirage.h" "../src/rip/serialization/binary/forwards.h" "../src/rip/serialization/binary/raw.h")
