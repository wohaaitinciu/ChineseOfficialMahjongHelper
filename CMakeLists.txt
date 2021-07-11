#/****************************************************************************
# Copyright (c) 2013-2014 cocos2d-x.org
# Copyright (c) 2015-2017 Chukong Technologies Inc.
#
# http://www.cocos2d-x.org
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:

# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
# ****************************************************************************/

cmake_minimum_required(VERSION 3.6)

set(APP_NAME ChineseOfficialMahjongHelper)

project(${APP_NAME})

if(XCODE)
    if(NOT DEFINED CMAKE_XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET)
        SET (CMAKE_XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET 8.0)
    endif()
endif()

if(NOT DEFINED BUILD_ENGINE_DONE) # to test ChineseOfficialMahjongHelper into root project
    set(COCOS2DX_ROOT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cocos2d)
    set(CMAKE_MODULE_PATH ${COCOS2DX_ROOT_PATH}/cmake/Modules/)

    include(CocosBuildSet)
    add_subdirectory(${COCOS2DX_ROOT_PATH}/cocos ${ENGINE_BINARY_PATH}/cocos/core)
endif()

# record sources, headers, resources...
set(GAME_SOURCE)
set(GAME_HEADER)

set(GAME_RES_FOLDER
    "${CMAKE_CURRENT_SOURCE_DIR}/Resources"
    )
if(APPLE OR WINDOWS)
    cocos_mark_multi_resources(common_res_files RES_TO "Resources" FOLDERS ${GAME_RES_FOLDER})
endif()

# add cross-platforms source files and header files 
list(APPEND GAME_SOURCE
     Classes/AppDelegate.cpp
     Classes/HelloWorldScene.cpp
     Classes/cocos-wheels/CWCommon.cpp
     Classes/cocos-wheels/CWTableView.cpp
     Classes/CompetitionSystem/CompetitionMainScene.cpp
     Classes/CompetitionSystem/LatestCompetitionScene.cpp
     Classes/FanCalculator/FanCalculatorScene.cpp
     Classes/FanTable/FanTableScene.cpp
     Classes/mahjong-algorithm/fan_calculator.cpp
     Classes/mahjong-algorithm/shanten.cpp
     Classes/mahjong-algorithm/stringify.cpp
     Classes/MahjongTheory/MahjongTheoryScene.cpp
     Classes/MainMenu/LeftSideMenu.cpp
     Classes/Other/OtherScene.cpp
     Classes/RecordSystem/Record.cpp
     Classes/RecordSystem/RecordHistoryScene.cpp
     Classes/RecordSystem/RecordScene.cpp
     Classes/RecordSystem/ScoreSheetScene.cpp
     Classes/Training/TrainingScene.cpp
     Classes/utils/common.cpp
     Classes/utils/socket.cpp
     Classes/widget/AlertDialog.cpp
     Classes/widget/CheckBoxScale9.cpp
     Classes/widget/CommonWebViewScene.cpp
     Classes/widget/DatePicker.cpp
     Classes/widget/ExtraInfoWidget.cpp
     Classes/widget/HandTilesWidget.cpp
     Classes/widget/PopupMenu.cpp
     Classes/widget/TilePickWidget.cpp
     Classes/widget/Toast.cpp
     )
list(APPEND GAME_HEADER
     Classes/AppDelegate.h
     Classes/HelloWorldScene.h
     Classes/cocos-wheels/CWCommon.h
     Classes/cocos-wheels/CWTableView.h
     Classes/CompetitionSystem/CompetitionMainScene.h
     Classes/CompetitionSystem/LatestCompetitionScene.h
     Classes/FanCalculator/FanCalculatorScene.h
     Classes/FanTable/FanTableScene.h
     Classes/mahjong-algorithm/fan_calculator.h
     Classes/mahjong-algorithm/shanten.h
     Classes/mahjong-algorithm/stringify.h
     Classes/mahjong-algorithm/tile.h
     Classes/MahjongTheory/MahjongTheoryScene.h
     Classes/MainMenu/LeftSideMenu.h
     Classes/Other/OtherScene.h
     Classes/RecordSystem/Record.h
     Classes/RecordSystem/RecordHistoryScene.h
     Classes/RecordSystem/RecordScene.h
     Classes/RecordSystem/ScoreSheetScene.h
     Classes/Training/TrainingScene.h
     Classes/utils/common.h
     Classes/utils/compiler.h
     Classes/utils/socket.h
     Classes/widget/AlertDialog.h
     Classes/widget/calendar-inline.h
     Classes/widget/CheckBoxScale9.h
     Classes/widget/CommonWebViewScene.h
     Classes/widget/DatePicker.h
     Classes/widget/ExtraInfoWidget.h
     Classes/widget/HandTilesWidget.h
     Classes/widget/LoadingView.h
     Classes/widget/PopupMenu.h
     Classes/widget/TilePickWidget.h
     Classes/widget/Toast.h
     )

if(ANDROID)
    # change APP_NAME to the share library name for Android, it's value depend on AndroidManifest.xml
    set(APP_NAME MyGame)
    list(APPEND GAME_SOURCE
         proj.android/app/jni/hellocpp/main.cpp
         Classes/cocos-wheels/CWCommon-android.cpp
         )
elseif(LINUX)
    list(APPEND GAME_SOURCE
         proj.linux/main.cpp
         )
elseif(WINDOWS)
    list(APPEND GAME_HEADER
         proj.win32/main.h
         proj.win32/resource.h
         )
    list(APPEND GAME_SOURCE
         Classes/cocos-wheels/CWCommon-win32.cpp
         proj.win32/main.cpp
         proj.win32/game.rc
         ${common_res_files}
         )
elseif(APPLE)
    if(IOS)
        list(APPEND GAME_HEADER
             proj.ios_mac/ios/AppController.h
             proj.ios_mac/ios/RootViewController.h
             )
        set(APP_UI_RES
            proj.ios_mac/ios/LaunchScreen.storyboard
            proj.ios_mac/ios/LaunchScreenBackground.png
            proj.ios_mac/ios/Images.xcassets
            )
        list(APPEND GAME_SOURCE
             Classes/cocos-wheels/CWCommon-ios.mm
             proj.ios_mac/ios/main.m
             proj.ios_mac/ios/AppController.mm
             proj.ios_mac/ios/RootViewController.mm
             proj.ios_mac/ios/Prefix.pch
             ${APP_UI_RES}
             )
    elseif(MACOSX)
        set(APP_UI_RES
            proj.ios_mac/mac/Icon.icns
            proj.ios_mac/mac/Info.plist
            )
        list(APPEND GAME_SOURCE
             Classes/cocos-wheels/CWCommon-mac.mm
             proj.ios_mac/mac/main.cpp
             proj.ios_mac/mac/Prefix.pch
             ${APP_UI_RES}
             )
    endif()
    list(APPEND GAME_SOURCE ${common_res_files})
endif()

# mark app complie info and libs info
set(all_code_files
    ${GAME_HEADER}
    ${GAME_SOURCE}
    )
if(NOT ANDROID)
    add_executable(${APP_NAME} ${all_code_files})
else()
    add_library(${APP_NAME} SHARED ${all_code_files})
    add_subdirectory(${COCOS2DX_ROOT_PATH}/cocos/platform/android ${ENGINE_BINARY_PATH}/cocos/platform)
    target_link_libraries(${APP_NAME} -Wl,--whole-archive cpp_android_spec -Wl,--no-whole-archive)
endif()

target_link_libraries(${APP_NAME} cocos2d)
target_include_directories(${APP_NAME}
        PRIVATE Classes
        PRIVATE ${COCOS2DX_ROOT_PATH}/cocos/audio/include/
)

# mark app resources
setup_cocos_app_config(${APP_NAME})
if(APPLE)
    set_target_properties(${APP_NAME} PROPERTIES RESOURCE "${APP_UI_RES}")

    if(MACOSX)
        set_xcode_property(${APP_NAME} INFOPLIST_FILE "${CMAKE_CURRENT_SOURCE_DIR}/proj.ios_mac/mac/Info.plist")
    elseif(IOS)
        set_xcode_property(${APP_NAME} INFOPLIST_FILE "${CMAKE_CURRENT_SOURCE_DIR}/proj.ios_mac/ios/Info.plist")
        set_xcode_property(${APP_NAME} ASSETCATALOG_COMPILER_APPICON_NAME "AppIcon")
    endif()

    # For code-signing, set the DEVELOPMENT_TEAM:
    #set_xcode_property(${APP_NAME} DEVELOPMENT_TEAM "GRLXXXX2K9")
elseif(WINDOWS)
    cocos_copy_target_dll(${APP_NAME})
endif()

if(LINUX OR WINDOWS)
    cocos_get_resource_path(APP_RES_DIR ${APP_NAME})
    cocos_copy_target_res(${APP_NAME} LINK_TO ${APP_RES_DIR} FOLDERS ${GAME_RES_FOLDER})
endif()
