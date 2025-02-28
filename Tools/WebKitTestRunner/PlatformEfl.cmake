add_custom_target(forwarding-headersEflForWebKitTestRunner
    COMMAND ${PERL_EXECUTABLE} ${WEBKIT2_DIR}/Scripts/generate-forwarding-headers.pl ${WEBKIT_TESTRUNNER_DIR} ${DERIVED_SOURCES_WEBKIT2_DIR}/include efl
    COMMAND ${PERL_EXECUTABLE} ${WEBKIT2_DIR}/Scripts/generate-forwarding-headers.pl ${WEBKIT_TESTRUNNER_DIR} ${DERIVED_SOURCES_WEBKIT2_DIR}/include soup
)
set(ForwardingHeadersForWebKitTestRunner_NAME forwarding-headersEflForWebKitTestRunner)

if (WTF_USE_TYGL)
    list(APPEND WebKitTestRunner_SOURCES
        ${WEBKIT_TESTRUNNER_DIR}/tygl/TestInvocationTyGL.cpp
    )
    list(APPEND WebKitTestRunner_INCLUDE_DIRECTORIES
        ${WEBKIT2_DIR}/Shared/API/c/tygl
    )
else()
    list(APPEND WebKitTestRunner_SOURCES
        ${WEBKIT_TESTRUNNER_DIR}/cairo/TestInvocationCairo.cpp
    )
    list(APPEND WebKitTestRunner_INCLUDE_DIRECTORIES
        ${CAIRO_INCLUDE_DIRS}
    )
    list(APPEND WebKitTestRunner_LIBRARIES
        ${CAIRO_LIBRARIES}
    )
endif()

list(APPEND WebKitTestRunner_SOURCES
    ${WEBKIT_TESTRUNNER_DIR}/efl/EventSenderProxyEfl.cpp
    ${WEBKIT_TESTRUNNER_DIR}/efl/PlatformWebViewEfl.cpp
    ${WEBKIT_TESTRUNNER_DIR}/efl/TestControllerEfl.cpp
    ${WEBKIT_TESTRUNNER_DIR}/efl/main.cpp
)

list(APPEND WebKitTestRunner_INCLUDE_DIRECTORIES
    ${DERIVED_SOURCES_WEBCORE_DIR}
    ${DERIVED_SOURCES_WEBCORE_DIR}/include
    ${DERIVED_SOURCES_WEBKIT2_DIR}/include

    ${WEBKIT2_DIR}/UIProcess/API/C/efl
    ${WEBKIT2_DIR}/UIProcess/API/efl
    "${WTF_DIR}/wtf/gobject"
    ${WEBCORE_DIR}/platform/network/soup
    ${ECORE_EVAS_INCLUDE_DIRS}
    ${ECORE_FILE_INCLUDE_DIRS}
    ${ECORE_INCLUDE_DIRS}
    ${EINA_INCLUDE_DIRS}
    ${EO_INCLUDE_DIRS}
    ${EVAS_INCLUDE_DIRS}
    ${GLIB_INCLUDE_DIRS}
)

list(APPEND WebKitTestRunner_LIBRARIES
    ${ECORE_LIBRARIES}
    ${ECORE_EVAS_LIBRARIES}
    ${EINA_LIBRARIES}
    ${EO_LIBRARIES}
    ${EVAS_LIBRARIES}
    WTF
)

if (ENABLE_GLES2)
    list(APPEND WebKitTestRunner_LIBRARIES
        ${OPENGLES2_LIBRARY}
    )
else ()
    list(APPEND WebKitTestRunner_LIBRARIES
        ${OPENGL_LIBRARIES}
    )
endif ()

list(APPEND WebKitTestRunnerInjectedBundle_LIBRARIES
    ${ATK_LIBRARIES}
    ${ECORE_LIBRARIES}
    ${ECORE_FILE_LIBRARIES}
    ${FONTCONFIG_LIBRARIES}
    ${GLIB_GOBJECT_LIBRARIES}
)

if (ENABLE_ECORE_X)
    list(APPEND WebKitTestRunner_INCLUDE_DIRECTORIES
        ${ECORE_X_INCLUDE_DIRS}
    )

    list(APPEND WebKitTestRunner_LIBRARIES
        ${ECORE_X_LIBRARIES}
        ${X11_Xext_LIB}
    )
endif ()

list(APPEND WebKitTestRunnerInjectedBundle_SOURCES
    ${WEBKIT_TESTRUNNER_INJECTEDBUNDLE_DIR}/atk/AccessibilityControllerAtk.cpp
    ${WEBKIT_TESTRUNNER_INJECTEDBUNDLE_DIR}/atk/AccessibilityNotificationHandlerAtk.cpp
    ${WEBKIT_TESTRUNNER_INJECTEDBUNDLE_DIR}/atk/AccessibilityUIElementAtk.cpp

    ${WEBKIT_TESTRUNNER_INJECTEDBUNDLE_DIR}/efl/ActivateFontsEfl.cpp
    ${WEBKIT_TESTRUNNER_INJECTEDBUNDLE_DIR}/efl/FontManagement.cpp
    ${WEBKIT_TESTRUNNER_INJECTEDBUNDLE_DIR}/efl/InjectedBundleEfl.cpp
    ${WEBKIT_TESTRUNNER_INJECTEDBUNDLE_DIR}/efl/TestRunnerEfl.cpp
)

# FIXME: EFL port needs to have own test font and font configure instead of gtk test font in future
# FIXME: DOWNLOADED_FONTS_DIR should not hardcode the directory structure.
add_definitions(-DFONTS_CONF_DIR="${TOOLS_DIR}/WebKitTestRunner/gtk/fonts"
                -DDOWNLOADED_FONTS_DIR="${CMAKE_SOURCE_DIR}/WebKitBuild/DependenciesEFL/Source/webkitgtk-test-fonts")

if (ENABLE_ACCESSIBILITY)
    list(APPEND WebKitTestRunner_INCLUDE_DIRECTORIES
        ${ATK_INCLUDE_DIRS}
    )
    list(APPEND WebKitTestRunner_LIBRARIES
        ${ATK_LIBRARIES}
    )
endif ()
