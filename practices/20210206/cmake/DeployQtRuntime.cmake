include(CMakeParseArguments)

## 部署Qt运行时及插件
function(DeployQtRuntime)
    set(options WebEngine)
    set(oneValueArgs TARGET QmlFilesPath)
    set(multiValueArgs PLUGINS)
    cmake_parse_arguments(Gen "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
    
    find_package(Qt5 COMPONENTS Core CONFIG)

    #加载Qt5::Core对应的库配置文件,能够找到Qt的bin目录
    if(NOT _qt5_install_prefix)
        return()
    endif()

    ## message(STATUS "Qt cmake模块位于:${_qt5_install_prefix}")
    ## _qt5_install_prefix基本上在lib/cmake位置,需要定位到bin路径下面找到部署程序
    find_program(__qt5_deploy windeployqt PATHS "${_qt5_install_prefix}/../../bin")

    ## 获取QTDIR
    set(QTDIR "${_qt5_install_prefix}/../../")

    set(qt_plugins "")
    foreach(__plugin ${Gen_PLUGINS})
        string(APPEND qt_plugins " -${__plugin}") 
    endforeach()
    separate_arguments(qt_plugins_list WINDOWS_COMMAND ${qt_plugins})

    set(QmlFilesPath "$<TARGET_FILE_DIR:${Gen_TARGET}>")
    if(Gen_QmlFilesPath)
        #如果用户设置了qml文件路径则直接使用
        set(QmlFilesPath "${Gen_QmlFilesPath}")
    endif()

    add_custom_command(TARGET ${Gen_TARGET} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E env QTDIR="${QTDIR}" "PATH=${QTDIR}/bin" windeployqt.exe --qmldir ${QmlFilesPath} $<TARGET_FILE:${Gen_TARGET}> ${qt_plugins_list}
        COMMENT "deploy Qt runtime dependencies"  
    )

    ## Qt如果部署包含Qt WebEngine的应用,需要处理无法自动部署的资源
    if(Gen_WebEngine)
        add_custom_command(TARGET ${Gen_TARGET} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory 
                "${_qt5_install_prefix}/../../resources/"  ##拷贝 resources 目录下内容
                "$<TARGET_FILE_DIR:${Gen_TARGET}>"
            COMMENT "deploy Qt WebEngine resources"

            COMMAND ${CMAKE_COMMAND} -E copy        	"${_qt5_install_prefix}/../../bin/$<IF:$<CONFIG:DEBUG>,QtWebEngineProcessd.exe,QtWebEngineProcess.exe>" 
                        "$<TARGET_FILE_DIR:${Gen_TARGET}>/$<IF:$<CONFIG:DEBUG>,QtWebEngineProcessd.exe,QtWebEngineProcess.exe>"            
            COMMENT "deploy Qt WebEngine process"
        )
    endif()    
endfunction()