cmake_minimum_required(VERSION 4.2)
project(FPLV)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)
include_directories(FPLV)
include_directories(FPLV/Debug)
include_directories("FPLV/Euroscope 3.2")
include_directories(inc)
include_directories(inc/3.2)
include_directories(inc/rapidjson)
include_directories(inc/rapidjson/error)
include_directories(inc/rapidjson/internal)
include_directories(inc/rapidjson/msinttypes)
include_directories(lib)
include_directories(res)

add_executable(FPLV
        FPLV/Debug/FPLV.pch
        "FPLV/Euroscope 3.2/FPLV.pch"
        inc/3.2/EuroScopePlugIn.h
        inc/rapidjson/error/en.h
        inc/rapidjson/error/error.h
        inc/rapidjson/internal/biginteger.h
        inc/rapidjson/internal/diyfp.h
        inc/rapidjson/internal/dtoa.h
        inc/rapidjson/internal/ieee754.h
        inc/rapidjson/internal/itoa.h
        inc/rapidjson/internal/meta.h
        inc/rapidjson/internal/pow10.h
        inc/rapidjson/internal/regex.h
        inc/rapidjson/internal/stack.h
        inc/rapidjson/internal/strfunc.h
        inc/rapidjson/internal/strtod.h
        inc/rapidjson/internal/swap.h
        inc/rapidjson/msinttypes/inttypes.h
        inc/rapidjson/msinttypes/stdint.h
        inc/rapidjson/allocators.h
        inc/rapidjson/document.h
        inc/rapidjson/encodedstream.h
        inc/rapidjson/encodings.h
        inc/rapidjson/filereadstream.h
        inc/rapidjson/filewritestream.h
        inc/rapidjson/fwd.h
        inc/rapidjson/istreamwrapper.h
        inc/rapidjson/memorybuffer.h
        inc/rapidjson/memorystream.h
        inc/rapidjson/ostreamwrapper.h
        inc/rapidjson/pointer.h
        inc/rapidjson/prettywriter.h
        inc/rapidjson/rapidjson.h
        inc/rapidjson/reader.h
        inc/rapidjson/schema.h
        inc/rapidjson/stream.h
        inc/rapidjson/stringbuffer.h
        inc/rapidjson/writer.h
        inc/discord_register.h
        inc/discord_rpc.h
        lib/3.2/discord-rpc-rel.lib
        lib/3.2/discord-rpc.lib
        lib/3.2/Euroscope.lib
        res/FPLV.rc2
        config.h
        ConfigData.cpp
        ConfigData.h
        ConfigManager.cpp
        ConfigManager.h
        DefaultFileContent.h
        FPLV.cpp
        FPLV.h
        FPLVExt.cpp
        FPLVExt.h
        Resource.h
        stdafx.cpp
        stdafx.h
        targetver.h)
