cmake_minimum_required(VERSION 3.0)
set(PIM_VERSION "5.5.80")

if (POLICY CMP0053)
    cmake_policy(SET CMP0053 NEW)
endif()

project(Messagelib VERSION ${PIM_VERSION})

option(MIMETREEPARSER_ONLY_BUILD "Build only mimetreeparser" FALSE)
option(KDEPIM_ENTERPRISE_BUILD "Enable features specific to the enterprise branch, which are normally disabled. Also, it disables many components not needed for Kontact such as the Kolab client." FALSE)


set(KF5_VERSION "5.35.0")


set(MESSAGELIB_LIB_VERSION ${PIM_VERSION})
set(AKONADIMIME_LIB_VERSION "5.5.80")

set(QT_REQUIRED_VERSION "5.7.0")

set(AKONADI_VERSION "5.5.80")
set(GRANTLEETHEME_LIB_VERSION "5.5.80")
set(GRAVATAR_LIB_VERSION "5.5.80")
set(IDENTITYMANAGEMENT_LIB_VERSION "5.5.80")
set(KCONTACTS_LIB_VERSION "5.5.80")
set(KDEPIM_APPS_LIB_VERSION "5.5.80")
set(KLDAP_LIB_VERSION "5.5.80")
set(KMAILTRANSPORT_LIB_VERSION "5.5.80")
set(KMBOX_LIB_VERSION "5.5.80")
set(KMIME_LIB_VERSION "5.5.80")
set(KPIMTEXTEDIT_LIB_VERSION "5.5.80")
set(LIBKDEPIM_LIB_VERSION "5.5.80")
set(LIBKLEO_LIB_VERSION "5.5.80")
set(PIMCOMMON_LIB_VERSION "5.5.80")
set(GPGME_LIB_VERSION "1.8.0")
set(AKONADI_CONTACT_VERSION "5.5.80")

if (${MIMETREEPARSER_ONLY_BUILD})
   set(KF5_VERSION "5.26.0")
   set(KMIME_LIB_VERSION "5.1.40")
endif()

find_package(ECM ${KF5_VERSION} CONFIG REQUIRED)
set(CMAKE_MODULE_PATH ${Messagelib_SOURCE_DIR}/cmake/modules ${ECM_MODULE_PATH})
set(LIBRARY_NAMELINK)
include(GenerateExportHeader)
include(ECMSetupVersion)
include(ECMGenerateHeaders)
include(ECMGeneratePriFile)
include(CMakePackageConfigHelpers)
include(FeatureSummary)
include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDEFrameworkCompilerSettings NO_POLICY_SCOPE)
include(ECMQtDeclareLoggingCategory)
include(ECMAddTests)
include(ECMCoverageOption)


find_package(Qt5 ${QT_REQUIRED_VERSION} CONFIG REQUIRED Gui Test)

find_package(KF5Codecs ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5I18n ${KF5_VERSION} CONFIG REQUIRED)
find_package(KF5Mime ${KMIME_LIB_VERSION} CONFIG REQUIRED)
find_package(QGpgme ${GPGME_LIB_VERSION} CONFIG REQUIRED)

if (NOT ${MIMETREEPARSER_ONLY_BUILD})
   find_package(Qt5 ${QT_REQUIRED_VERSION} CONFIG REQUIRED Widgets Network PrintSupport WebEngine WebEngineWidgets)
   find_package(KF5Archive ${KF5_VERSION} CONFIG REQUIRED)
   find_package(KF5Completion ${KF5_VERSION} CONFIG REQUIRED)
   find_package(KF5Config ${KF5_VERSION} CONFIG REQUIRED)
   find_package(KF5ConfigWidgets ${KF5_VERSION} CONFIG REQUIRED)
   find_package(KF5IconThemes ${KF5_VERSION} CONFIG REQUIRED)
   find_package(KF5ItemViews ${KF5_VERSION} CONFIG REQUIRED)
   find_package(KF5JobWidgets ${KF5_VERSION} CONFIG REQUIRED)
   find_package(KF5KIO ${KF5_VERSION} CONFIG REQUIRED)
   find_package(KF5Service ${KF5_VERSION} CONFIG REQUIRED)
   find_package(KF5Sonnet ${KF5_VERSION} CONFIG REQUIRED)
   find_package(KF5TextWidgets ${KF5_VERSION} CONFIG REQUIRED)
   find_package(KF5WidgetsAddons ${KF5_VERSION} CONFIG REQUIRED)
   find_package(KF5WindowSystem ${KF5_VERSION} CONFIG REQUIRED)
   find_package(KF5XmlGui ${KF5_VERSION} CONFIG REQUIRED)
   find_package(KF5SyntaxHighlighting ${KF5_VERSION} CONFIG REQUIRED)

   find_package(Grantlee5 "5.1" CONFIG REQUIRED)

   find_package(KF5Akonadi ${AKONADI_VERSION} CONFIG REQUIRED)
   find_package(KF5AkonadiMime ${AKONADIMIME_LIB_VERSION} CONFIG REQUIRED)
   find_package(KF5Contacts ${KCONTACTS_LIB_VERSION} CONFIG REQUIRED)
   find_package(KF5AkonadiContact ${AKONADI_CONTACT_VERSION} CONFIG REQUIRED)
   find_package(KF5FollowupReminder ${KDEPIM_APPS_LIB_VERSION} CONFIG REQUIRED)
   find_package(KF5GrantleeTheme ${GRANTLEETHEME_LIB_VERSION} CONFIG REQUIRED)
   find_package(KF5Gravatar ${GRAVATAR_LIB_VERSION} CONFIG REQUIRED)
   find_package(KF5IdentityManagement ${IDENTITYMANAGEMENT_LIB_VERSION} CONFIG REQUIRED)
   find_package(KF5KaddressbookGrantlee ${KDEPIM_APPS_LIB_VERSION} CONFIG REQUIRED)
   find_package(KF5Ldap ${KLDAP_LIB_VERSION} CONFIG REQUIRED)
   find_package(KF5LibkdepimAkonadi ${LIBKDEPIM_LIB_VERSION} CONFIG REQUIRED)
   find_package(KF5Libkleo ${LIBKLEO_LIB_VERSION} CONFIG REQUIRED)
   find_package(KF5MailTransportAkonadi ${KMAILTRANSPORT_LIB_VERSION} CONFIG REQUIRED)
   find_package(KF5Mbox ${KMBOX_LIB_VERSION} CONFIG REQUIRED)
   find_package(KF5PimCommonAkonadi ${PIMCOMMON_LIB_VERSION} CONFIG REQUIRED)
   find_package(KF5PimTextEdit ${KPIMTEXTEDIT_LIB_VERSION} CONFIG REQUIRED)
   find_package(KF5SendLater ${KDEPIM_APPS_LIB_VERSION} CONFIG REQUIRED)

   find_package(KF5AkonadiSearch "5.5.80" CONFIG REQUIRED)
   set_package_properties(KF5AkonadiSearch PROPERTIES DESCRIPTION "The Akonadi Search libraries" URL "http://www.kde.org" TYPE REQUIRED PURPOSE "Provides search capabilities in KMail and Akonadi")

endif()

# Support for the GIT revision number in messagecomposer-version.h
if(EXISTS "${Messagelib_SOURCE_DIR}/.git")
  find_package(Git)
  if(GIT_FOUND)
    execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
                    WORKING_DIRECTORY ${Messagelib_SOURCE_DIR}
                    OUTPUT_VARIABLE messagelib_git_revision)
    string(REGEX REPLACE "\n" "" messagelib_git_revision "${messagelib_git_revision}")
    set(messagelib_git_revision "git-${messagelib_git_revision}")

    execute_process(COMMAND ${GIT_EXECUTABLE} log -1 --oneline --format=%ci
                    WORKING_DIRECTORY ${Messagelib_SOURCE_DIR}
                    OUTPUT_VARIABLE messagelib_git_last_change)
    string(REGEX REPLACE " [-0-9:+ ]*\n" "" messagelib_git_last_change "${messagelib_git_last_change}")
  endif()
endif ()

add_definitions(-DQT_NO_URL_CAST_FROM_STRING)

if(BUILD_TESTING)
   add_definitions(-DBUILD_TESTING)
endif(BUILD_TESTING)

add_definitions(-DQT_NO_NARROWING_CONVERSIONS_IN_CONNECT)
add_subdirectory(mimetreeparser)
if (NOT ${MIMETREEPARSER_ONLY_BUILD})
   add_subdirectory(messageviewer)
   add_subdirectory(templateparser)
   add_subdirectory(messagecomposer)
   add_subdirectory(messagecore)
   add_subdirectory(messagelist)
   add_subdirectory(webengineviewer)
endif()

install( FILES messagelib.renamecategories messagelib.categories DESTINATION ${KDE_INSTALL_CONFDIR} )
feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)

if (NOT ${MIMETREEPARSER_ONLY_BUILD})
    if (${Qt5WebEngine_VERSION} GREATER "5.7.99")
        MESSAGE(STATUS "QtWebEngine greater or equal 5.8.0 provides printing support and dnd fix. It's a good idea to use it")
    else()
        if (${Qt5WebEngine_VERSION} GREATER "5.6.99")
            MESSAGE(STATUS "QtWebEngine 5.7.0 doesn't provide printing support. It has a lot of regression. Better to upgrade QtWebEngine to 5.8.0")
        endif()
    endif()
endif()
