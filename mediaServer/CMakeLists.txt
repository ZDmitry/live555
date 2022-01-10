add_executable(live555MediaServer
    DynamicRTSPServer.cpp
    DynamicRTSPServer.hh
    CustomMediaServer.cpp
    CustomMediaServer.hh
    CustomMediaClient.cpp
    CustomMediaClient.hh
    live555MediaServer.cpp
    MJpegMediaSource.cpp
    MJpegMediaSource.hh
    MJpegMediaSubsession.cpp
    MJpegMediaSubsession.hh
    JpegFrameParser.cpp
    JpegFrameParser.hh
    version.hh
)
target_link_libraries(live555MediaServer PRIVATE live555::live555)
set_target_properties(live555MediaServer PROPERTIES FOLDER "live555/Servers")
