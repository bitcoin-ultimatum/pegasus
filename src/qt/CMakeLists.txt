CMAKE_MINIMUM_REQUIRED(VERSION 3.10)

set(PACKAGE_NAME "btcu qt")

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/scripts)
list(APPEND CMAKE_PREFIX_PATH ./src/eth_client/utils/ethash)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC_SEARCH_PATHS ${CMAKE_CURRENT_SOURCE_DIR}/forms ${CMAKE_CURRENT_SOURCE_DIR}/btcu/forms ${CMAKE_CURRENT_SOURCE_DIR}/btcu/settings/forms)

# Qt MOC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux" AND BUILD_STATIC)
	add_linker_flags(-Wl,-Bdynamic -lGL)
	add_linker_flags(-Wl,-Bdynamic -lX11)
	add_linker_flags(-Wl,--copy-dt-needed-entries)

    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
	add_compile_options("-fPIE")
endif ()
if(BUILD_STATIC)
		find_package(Qt5QmlImportScanner REQUIRED)
endif()
include_directories ( ${Qrcode_INCLUDE_DIRS} )

# you can remove check for the Darwin if you'd build
# a static QT version by yourself
if (BUILD_STATIC AND NOT ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	set(Qt5_USE_STATIC_LIBS ON)
	set(Qt5_USE_STATIC_RUNTIME ON)
endif ()
if(BUILD_STATIC AND NOT ${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	cmake_policy(SET CMP0022 NEW)

	find_library(GRAPHITE2_LIBRARY NAMES graphite2 libgraphite2)
	message(STATUS "GRAPHITE2_LIBRARY: ${GRAPHITE2_LIBRARY}")

	set(TMP_CMAKE_CXX_STANDARD_LIBRARIES ${CMAKE_CXX_STANDARD_LIBRARIES})

	if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
		# we don't want tp link qt5 statically since it is quite unstable
		set(CMAKE_FIND_LIBRARY_SUFFIXES ".dylib" ".so" CACHE STRING "CMake search suffix" FORCE)
		find_package( X11 REQUIRED )
		message(STATUS "X11_LIBRARIES: ${X11_LIBRARIES}")
		message(STATUS "X11_Xdmcp_LIB: ${X11_Xdmcp_LIB}")

		# This is required to fix libm.a incorrect import in QT
		list(APPEND CMAKE_CXX_STANDARD_LIBRARIES "m")
	endif()
endif()

unset(Qt5Enables)
foreach(comp Widgets Core Gui Network LinguistTools DBus Svg Charts Test)
	if(${CMAKE_SYSTEM_NAME} MATCHES "Linux" AND BUILD_STATIC)
		find_package(
						Qt5${comp}
					PATHS
						${qt5_ROOT_DIR}
						${qt5_ROOT_DIR}/lib
						${qt5_ROOT_DIR}/lib/cmake
					PATH_SUFFIXES
						Qt5${comp}
					NO_SYSTEM_ENVIRONMENT_PATH
					NO_DEFAULT_PATH
					NO_CMAKE_PATH
					NO_CMAKE_ENVIRONMENT_PATH
					NO_CMAKE_SYSTEM_PATH
					NO_CMAKE_FIND_ROOT_PATH
				)
		if(NOT Qt5${comp}_FOUND)
			find_package(Qt5${comp} NO_SYSTEM_ENVIRONMENT_PATH)
		endif()
	else()
    	find_package(Qt5${comp} NO_SYSTEM_ENVIRONMENT_PATH)
	endif()
    set(Qt5Enables "${Qt5Enables};Qt5${comp}_FOUND")
    if(Qt5${comp}_FOUND)
		if(BUILD_STATIC AND NOT ${comp} MATCHES "LinguistTools" AND NOT ${CMAKE_SYSTEM_NAME} MATCHES "Windows")
			target_link_libraries(Qt5::${comp} INTERFACE ${GRAPHITE2_LIBRARY})
			if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
				target_link_libraries(Qt5::${comp} INTERFACE ${X11_LIBRARIES})
				target_link_libraries(Qt5::${comp} INTERFACE ${X11_Xau_LIB})
				target_link_libraries(Qt5::${comp} INTERFACE ${X11_Xdmcp_LIB})
			endif()
		endif()
        message(STATUS "FOUND QT5${comp}")
    	message(STATUS "INCLUDE: ${Qt5${comp}_INCLUDE_DIRS}")
    	message(STATUS ${Qt5${comp}_LIBRARIES})
        include_directories(${include_directories} ${Qt5${comp}_INCLUDE_DIRS} ${QT_USE_FILE})
        add_definitions(${Qt5${comp}_DEFINITIONS})
        list(APPEND Qt5_LIBRARIES ${Qt5${comp}_LIBRARIES})
    else()
        MESSAGE(WARNING "Cant find Qt5${comp}")
    endif()
endforeach(comp)

if(BUILD_STATIC AND NOT ${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	set(CMAKE_CXX_STANDARD_LIBRARIES ${TMP_CMAKE_CXX_STANDARD_LIBRARIES})
	set(CMAKE_FIND_LIBRARY_SUFFIXES ".a" ".lib" ".dylib" ".so" CACHE STRING "CMake search suffix" FORCE)
endif()

if (Qt5DBus_FOUND)
    add_compile_options("-DUSE_DBUS")
endif()

if (Qt5Charts_FOUND)
    add_compile_options("-DUSE_QTCHARTS")
endif()

find_program(MOC_BIN NAMES moc moc-qt5 qt5-moc
        PATHS  ${MOC_BIN_DIR} /usr/lib/qt5/bin /usr/lib/x86_64-linux-gnu/qt5/bin ${qt5_ROOT_DIR}/bin
        NO_DEFAULT_PATH
        NO_SYSTEM_ENVIRONMENT_PATH)
if (MOC_BIN)
    MESSAGE(STATUS "FOUND MOC ${MOC_BIN}")
else()
    MESSAGE(STATUS "NOT FOUND MOC ${MOC_BIN}")
endif()

add_compile_options("-DQT_NO_KEYWORDS")

# Find out more about Qt. This is similar to
# http://code.qt.io/cgit/qt/qtwebkit.git/tree/Source/cmake/OptionsQt.cmake
get_target_property(QT_CORE_TYPE Qt5::Core TYPE)
if(QT_CORE_TYPE MATCHES STATIC)
	set(QT_STATIC_BUILD ON)
    message(STATUS "QT5: Static build ENABLED")
endif()


file(GLOB LOCAL_QT_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
source_group("QtHeaders" FILES ${LOCAL_QT_HEADERS})

find_package(Protobuf REQUIRED)
if(Protobuf_FOUND)
    message(STATUS "INCLUDE: ${Protobuf_INCLUDE_DIR}")
    message(STATUS "Found Protobuf compiler: ${Protobuf_PROTOC_EXECUTABLE}")
    message(STATUS "Found Protobuf library: ${Protobuf_LIBRARIES}")
endif()
include_directories(${PROTOBUF_INCLUDE_DIRS})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

PROTOBUF_GENERATE_CPP(PROTO_SRCS PROTO_HDRS ${CMAKE_CURRENT_SOURCE_DIR}/paymentrequest.proto)

SET(QT_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/bantablemodel.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/blockexplorer.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/bitcoinaddressvalidator.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/bitcoinamountfield.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/bitcoinunits.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/clientmodel.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/csvmodelwriter.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/governancepage.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/guiutil.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/intro.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/networkstyle.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/notificator.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/optionsdialog.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/optionsmodel.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/peertablemodel.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/platformstyle.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/qvalidatedlineedit.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/qvaluecombobox.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/rpcconsole.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/splashscreen.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/trafficgraphwidget.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/utilitydialog.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/addressbookpage.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/addresstablemodel.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/askpassphrasedialog.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/coincontroldialog.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/zbtcucontroldialog.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/coincontroltreewidget.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/proposalframe.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/editaddressdialog.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/openuridialog.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/paymentrequestplus.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/paymentserver.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/recentrequeststablemodel.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/transactiondesc.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/transactionfilterproxy.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/transactionrecord.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/transactiontablemodel.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/walletmodel.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/walletmodeltransaction.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/zbtcucontroldialog.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/btcu/btcugui.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/btcu/loadingdialog.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/btcu/pwidget.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/btcu/guitransactionsutils.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/btcu/snackbar.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/btcu/navmenuwidget.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/btcu/lockunlock.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/btcu/receivedialog.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/btcu/furabstractlistitemdelegate.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/btcu/txviewholder.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/btcu/qtutils.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/btcu/expandablebutton.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/btcu/walletpassworddialog.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/btcu/topbar.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/btcu/txrow.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/btcu/addressfilterproxymodel.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/btcu/dashboardwidget.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/btcu/coincontrolbtcuwidget.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/btcu/addresslabelrow.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/btcu/contactdropdownrow.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/btcu/contactsdropdown.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/btcu/optionbutton.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/btcu/mninfodialog.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/btcu/coldstakingmodel.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/btcu/leasingmodel.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/btcu/mnmodel.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/btcu/masternodewizarddialog.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/btcu/mnrow.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/btcu/masternodeswidget.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/btcu/validatorwidget.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/btcu/registervalidator.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/btcu/validatorrow.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/btcu/createcontract.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/btcu/sendchangeaddressdialog.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/btcu/sendconfirmdialog.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/btcu/sendcustomfeedialog.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/btcu/sendchangeaddressdialog.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/btcu/sendmultirow.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/btcu/send.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/btcu/addnewaddressdialog.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/btcu/addnewcontactdialog.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/btcu/requestdialog.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/btcu/receivewidget.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/btcu/myaddressrow.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/btcu/csrow.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/btcu/lrow.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/btcu/addressholder.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/btcu/tooltipmenu.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/btcu/addresseswidget.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/btcu/defaultdialog.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/btcu/denomgenerationdialog.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/btcu/privacywidget.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/btcu/coldstakingwidget.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/btcu/leasingwidget.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/btcu/settings/settingsbackupwallet.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/btcu/settings/settingsbittoolwidget.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/btcu/settings/settingsconsolewidget.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/btcu/settings/settingsdisplayoptionswidget.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/btcu/settings/settingsfaqwidget.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/btcu/settings/settingsinformationwidget.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/btcu/settings/settingsmainoptionswidget.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/btcu/settings/settingsmultisenddialog.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/btcu/settings/settingsmultisendwidget.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/btcu/settings/settingssignmessagewidgets.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/btcu/settings/settingswalletoptionswidget.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/btcu/settings/settingswalletrepairwidget.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/btcu/settings/settingswidget.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/btcu/welcomecontentwidget.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/btcu/splash.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/btcu/multileasingdialog.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/btcu/createmasternodewidget.cpp 
        ${CMAKE_CURRENT_SOURCE_DIR}/btcu/createvalidatorwidget.cpp 
        ${CMAKE_CURRENT_SOURCE_DIR}/btcu/leasingstatisticswidget.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/btcu/cbtokenrow.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/btcu/cbtokenmodel.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/btcu/cbtocendropdown.cpp
        )

set(SERVER_SOURCES
        ${CMAKE_SOURCE_DIR}/src/eth_client/utils/ethash/lib/ethash/keccak.c
        ${CMAKE_SOURCE_DIR}/src/eth_client/utils/ethash/lib/ethash/ethash.cpp
        ${CMAKE_SOURCE_DIR}/src/eth_client/utils/ethash/lib/ethash/keccakf1600.c
        ${CMAKE_SOURCE_DIR}/src/eth_client/utils/ethash/lib/ethash/primes.c

        ${CMAKE_SOURCE_DIR}/src/eth_client/utils/ethash/lib/ethash/ethash.cpp
        ${CMAKE_SOURCE_DIR}/src/eth_client/utils/ethash/lib/ethash/keccak.c
        ${CMAKE_SOURCE_DIR}/src/eth_client/utils/ethash/lib/ethash/keccakf800.c
        ${CMAKE_SOURCE_DIR}/src/eth_client/utils/ethash/lib/ethash/keccakf1600.c
        ${CMAKE_SOURCE_DIR}/src/eth_client/utils/ethash/lib/ethash/managed.cpp
        ${CMAKE_SOURCE_DIR}/src/eth_client/utils/ethash/lib/ethash/primes.c
        ${CMAKE_SOURCE_DIR}/src/eth_client/utils/ethash/lib/ethash/progpow.cpp
        )
add_library(SERVER_B STATIC ${BitcoinHeaders} ${SERVER_SOURCES})

target_include_directories(SERVER_B
    PUBLIC
        ${ZeroMQ_INCLUDE_DIRS}
        ${LibEvent_INCLUDE_DIR}
        ${BerkeleyDB_INCLUDE_DIRS}
        ${OPENSSL_INCLUDE_DIR}
        ${Boost_INCLUDE_DIRS}
        ${Boost_INCLUDE_DIRS}/filesystem
        ${CMAKE_SOURCE_DIR}/src/eth_client
        ${CMAKE_SOURCE_DIR}/src/eth_client/utils/ethash/include
        ${CMAKE_SOURCE_DIR}/src/evmone/evmc/include
        ${CMAKE_SOURCE_DIR}/src/qtum
        )

set(BITCOIN_CRYPTO_SOURCES
        ${CMAKE_SOURCE_DIR}/src/crypto/keccak.c
        )
add_library(BITCOIN_CRYPTO_B ${BITCOIN_CRYPTO_SOURCES})

target_link_libraries(BITCOIN_CRYPTO_B
            univalue
            ${Boost_LIBRARIES}
            ${OPENSSL_LIBRARIES}
            ${LibEvent_LIBRARIES}
            Threads::Threads
            )
target_include_directories(BITCOIN_CRYPTO_B PUBLIC
        ${CMAKE_SOURCE_DIR}/src
        ${OPENSSL_INCLUDE_DIR}
        ${Boost_INCLUDE_DIRS}
        )

set(COMMON_SOURCES
        ${CMAKE_SOURCE_DIR}/src/eth_client/libethashseal/GenesisInfo.cpp
        )

add_library(COMMON_B STATIC ${BitcoinHeaders} ${COMMON_SOURCES})

target_include_directories(COMMON_B
    PUBLIC
        ${OPENSSL_INCLUDE_DIR}
        ${BerkeleyDB_INCLUDE_DIRS}
        ${CMAKE_SOURCE_DIR}/src/eth_client
        ${CMAKE_SOURCE_DIR}/src/eth_client/utils/ethash/include
        ${CMAKE_SOURCE_DIR}/src/evmone/evmc/include
        ${CMAKE_SOURCE_DIR}/src/qtum
        )
target_link_libraries(COMMON_B
            BITCOIN_CRYPTO_B
            ${Boost_LIBRARIES}
            ${OPENSSL_LIBRARIES}
            ${LibEvent_LIBRARIES}
            SERVER_B
            )

# Workaround until the old rpcconsole UI window is fully removed
set_property(SOURCE ${CMAKE_CURRENT_SOURCE_DIR}/btcu/settings/settingsconsolewidget.cpp PROPERTY SKIP_AUTOMOC ON)
execute_process(
        COMMAND ${MOC_BIN} -o moc_settingsconsolewidget.cpp settingsconsolewidget.h
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/btcu/settings
)

execute_process(
        COMMAND ${MOC_BIN} -o moc_pfborderimage.cpp pfborderimage.h
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/btcu
)
list(APPEND QT_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/btcu/moc_pfborderimage.cpp)

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    list(APPEND QT_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/macdockiconhandler.mm)
    list(APPEND QT_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/macnotificationhandler.mm)
    list(APPEND QT_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/macos_appnap.mm)
endif()

add_library(qt_stuff STATIC ${BitcoinHeaders} ${QtHeaders} ${QT_SOURCES} ${PROTO_SRCS} ${PROTO_HDRS} ${SERVER_SOURCES} ${COMMON_SOURCES} ${BITCOIN_CRYPTO_SOURCES})
target_include_directories(qt_stuff
		PUBLIC 
			${CMAKE_SOURCE_DIR}/src
			${CMAKE_SOURCE_DIR}/src/leveldb/include
			${CMAKE_SOURCE_DIR}/src/univalue/include
			${CMAKE_SOURCE_DIR}/src/secp256k1/include
            ${CMAKE_SOURCE_DIR}/src/eth_client
            ${CMAKE_SOURCE_DIR}/src/eth_client/utils/ethash/include
            ${CMAKE_SOURCE_DIR}/src/evmone/evmc/include
            ${CMAKE_SOURCE_DIR}/src/qtum
			${CMAKE_CURRENT_SOURCE_DIR}
			${CMAKE_CURRENT_SOURCE_DIR}/btcu
			${CMAKE_CURRENT_SOURCE_DIR}/btcu/settings
			${OPENSSL_INCLUDE_DIR}
			${BerkeleyDB_INCLUDE_DIRS}
			${CMAKE_SOURCE_DIR}/src/eth_clientm/
			${Boost_INCLUDE_DIRS}
			${Boost_INCLUDE_DIRS}/filesystem
        )
set_property(TARGET qt_stuff PROPERTY CXX_STANDARD 17)

if(ZeroMQ_FOUND)
    if(${CMAKE_SYSTEM_NAME} MATCHES "Windows" OR BUILD_STATIC)
		target_link_libraries(qt_stuff
			ZeroMQ::zmq
			)
	else()
		target_link_libraries(qt_stuff
			${ZeroMQ_LIBRARY}
			)
	endif()
    target_include_directories(qt_stuff PUBLIC ${ZeroMQ_INCLUDE_DIRS})
endif()
if(BUILD_STATIC)
	target_link_libraries(qt_stuff ZLIB::ZLIB)
	target_link_libraries(qt_stuff RocksDB::rocksdb)
    target_include_directories(qt_stuff PUBLIC ${RocksDB_INCLUDE_DIRS})
endif()

if(SIGN_DMG)
	include(CodeSignIOS)

	# Do this on every run, to keep list of possible identities up-to-date.
	if (NOT DEFINED SIGN_CERT_NAME)
		code_sign_get_identity_macos(M_SIGN_CERT_NAME)
	else()
		set(M_SIGN_CERT_NAME "${SIGN_CERT_NAME}")
	endif ()
	set(SIGN_CERT_NAME "${M_SIGN_CERT_NAME}" CACHE STRING "code signing identity (e.g., \"iPhone Developer\") (required for code signing)")
	
	code_sign_get_cmd_args_macos(CODESIGNARGS)
	message(STATUS "Variable SIGN_CERT_NAME now is ${SIGN_CERT_NAME}")
endif()

file(GLOB QT_TRANSLATIONS_FILES ${CMAKE_CURRENT_SOURCE_DIR}/locale/*.ts)

get_filename_component(QM_FILES_BINARY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/locale" ABSOLUTE)
set_source_files_properties(${QT_TRANSLATIONS_FILES} PROPERTIES OUTPUT_LOCATION "${QM_FILES_BINARY_DIR}")

QT5_ADD_TRANSLATION(QM ${QT_TRANSLATIONS_FILES})
add_custom_target(translations_target ALL DEPENDS ${QM})

QT5_ADD_RESOURCES(QRC_RESOURCE ${PROJECT_SOURCE_DIR}/src/qt/btcu.qrc)
QT5_ADD_RESOURCES(QRC_LOCALE_RESOURCE ${PROJECT_SOURCE_DIR}/src/qt/btcu_locale.qrc)

add_executable(btcu-qt btcu.cpp ${QM} ${QRC_RESOURCE} ${QRC_LOCALE_RESOURCE} ${PROJECT_SOURCE_DIR}/src/qt/btcu.qrc  ../interfaces/node.h)
if(BUILD_STATIC)
	set_target_properties(btcu-qt PROPERTIES LINK_SEARCH_START_STATIC 1)
	set_target_properties(btcu-qt PROPERTIES LINK_SEARCH_END_STATIC 1)
endif()
if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	include(WindowsVersionInfo)
	generate_windows_version_info(btcu-qt
		DESCRIPTION "GUI node for BTCU"
		ICONS
			"res/icons/bitcoin.ico"
	)
endif()
add_dependencies(btcu-qt translations_target leveldb leveldb_sse42 memenv)

target_include_directories(btcu-qt PUBLIC ${ENDIAN_INCLUDES} ${CMAKE_SOURCE_DIR}/src/eth_client/)

target_link_libraries(btcu-qt
        qt_stuff
        univalue
        SERVER_A WALLET_A CLI_A UTIL_A COMMON_A ZEROCOIN_A BITCOIN_CRYPTO_A COMMON_B BITCOIN_CRYPTO_B SERVER_B
        leveldb leveldb_sse42 memenv secp256k1
        ${BerkeleyDB_Cxx_LIBRARY}
        ${OPENSSL_LIBRARIES}
		${Boost_LIBRARIES}
		${PROTOBUF_LIBRARIES}
		${LibEvent_LIBRARIES}
        ${Qrcode_LIBRARIES}
        Threads::Threads
    )

if(BUILD_STATIC)
    qt5_import_qml_plugins(btcu-qt)
    qt5_import_plugins(btcu-qt INCLUDE Qt5::QSvgPlugin Qt5::QSvgIconPlugin Qt5::QSvgIconPlugin Qt5::QGifPlugin Qt5::QJpegPlugin)
endif()

get_filename_component(_qt_bin_dir "${qt5_ROOT_DIR}/bin" DIRECTORY)
find_program(MACDEPLOYQT_EXECUTABLE macdeployqt HINTS "${_qt_bin_dir}")

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin" AND BUILTIN_QTDEPLOY)
	if(SIGN_DMG)
		add_custom_command(TARGET btcu-qt POST_BUILD
			COMMAND "${MACDEPLOYQT_EXECUTABLE}"
				"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/btcu-qt.app"
				-always-overwrite -codesign=${SIGN_CERT_NAME}
			COMMENT "Deploying Qt..."
			WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
			VERBATIM
		)
	else()
		add_custom_command(TARGET btcu-qt POST_BUILD
			COMMAND "${MACDEPLOYQT_EXECUTABLE}"
				"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/btcu-qt.app"
				-always-overwrite
			COMMENT "Deploying Qt..."
			WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
			VERBATIM
		)
	endif()
endif()

# Linux support
if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
	set(CMAKE_AUTOMOC_MOC_OPTIONS "-DQ_OS_LINUX")
	list(APPEND QT_PLUGIN_COMPONENTS QXcbIntegrationPlugin)
	list(APPEND QT_PLUGIN_PLATFORM_DEFINITIONS -DQT_QPA_PLATFORM_XCB=1)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    set(CMAKE_AUTOMOC_MOC_OPTIONS "-DQ_OS_WIN")

	list(APPEND QT_PLUGIN_COMPONENTS QWindowsIntegrationPlugin)
	list(APPEND QT_PLUGIN_PLATFORM_DEFINITIONS -DQT_QPA_PLATFORM_WINDOWS=1)

	target_sources(btcu-qt PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/winshutdownmonitor.cpp)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(CMAKE_AUTOMOC_MOC_OPTIONS "-DQ_OS_MAC")

	list(APPEND QT_PLUGIN_COMPONENTS QCocoaIntegrationPlugin)
	list(APPEND QT_PLUGIN_PLATFORM_DEFINITIONS -DQT_QPA_PLATFORM_COCOA=1)

	target_link_libraries(btcu-qt
		"-framework Foundation"
		"-framework ApplicationServices"
		"-framework AppKit"
	)
else()
	# Add the minimal integration plugin, and other plugins according to the target
	# platform.
	set(QT_PLUGIN_COMPONENTS QMinimalIntegrationPlugin)
	set(QT_PLUGIN_PLATFORM_DEFINITIONS -DQT_QPA_PLATFORM_MINIMAL=1)
endif()

if(GMP_FOUND)
    target_link_libraries(btcu-qt ${GMP_LIBRARIES})
    target_include_directories(btcu-qt PUBLIC ${GMP_INCLUDE_DIR})
endif()
if(ZeroMQ_FOUND)
    target_link_libraries(btcu-qt ZMQ_A)
    if(${CMAKE_SYSTEM_NAME} MATCHES "Windows" OR BUILD_STATIC)
		target_link_libraries(btcu-qt
			ZeroMQ::zmq
			)
	else()
		target_link_libraries(btcu-qt
			${ZeroMQ_LIBRARY}
			)
	endif()
    target_include_directories(btcu-qt PUBLIC ${ZeroMQ_INCLUDE_DIRS})
endif()
if (Qrcode_FOUND)
    target_link_libraries(btcu-qt ${Qrcode_LIBRARIES})
    target_include_directories(btcu-qt PUBLIC ${Qrcode_INCLUDE_DIRS})
endif()
if(MINIUPNP_FOUND)
	if(${CMAKE_SYSTEM_NAME} MATCHES "Windows" OR BUILD_STATIC)
		target_compile_definitions(btcu-qt
			PUBLIC -DSTATICLIB
			PUBLIC -DMINIUPNP_STATICLIB
		)
	endif()
    target_link_libraries(btcu-qt ${MINIUPNP_LIBRARY})
    target_include_directories(btcu-qt PUBLIC ${MINIUPNP_INCLUDE_DIR})
endif()

target_link_libraries(btcu-qt Qt5::Gui Qt5::Widgets Qt5::Network Qt5::Svg ${Qt5_LIBRARIES})
if (Qt5DBus_FOUND)
    target_link_libraries(btcu-qt Qt5::DBus)
endif()
if (Qt5Charts_FOUND)
    target_link_libraries(btcu-qt Qt5::Charts)
endif()
if(BUILD_STATIC)
	target_link_libraries(btcu-qt ZLIB::ZLIB)
	target_link_libraries(btcu-qt RocksDB::rocksdb)
    target_include_directories(btcu-qt PUBLIC ${RocksDB_INCLUDE_DIRS})
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	set(btcu_VERSION "${CLIENT_VERSION_MAJOR}.${CLIENT_VERSION_MINOR}.${CLIENT_VERSION_REVISION}")

	set(OSX_INSTALLER_ICONS "res/icons/bitcoin.icns")
	get_filename_component(OSX_INSTALLER_ICONS_NAME
		"${OSX_INSTALLER_ICONS}"
		NAME
	)

	set(INFO_PLIST_STRINGS_FILE "Base.lproj/InfoPlist.strings")
	set(INFO_PLIST_STRINGS_PATH "${CMAKE_CURRENT_BINARY_DIR}/${INFO_PLIST_STRINGS_FILE}")
	file(WRITE
		"${INFO_PLIST_STRINGS_PATH}"
		"{	CFBundleDisplayName = \"${PACKAGE_NAME}\"; CFBundleName = \"${PACKAGE_NAME}\"; }"
	)

	set(EMPTY_LPROJ_FILE "${CMAKE_CURRENT_BINARY_DIR}/empty.lproj")
	file(TOUCH "${EMPTY_LPROJ_FILE}")

	target_sources(btcu-qt PRIVATE
		"${OSX_INSTALLER_ICONS}"
		"${INFO_PLIST_STRINGS_PATH}"
		"${EMPTY_LPROJ_FILE}"
	)

	string(JOIN ";" BITCOINQT_BUNDLE_RESOURCES
		"${OSX_INSTALLER_ICONS}"
		"${EMPTY_LPROJ_FILE}"
	)

	set(CLIENT_VERSION_MAJOR ${btcu_VERSION_MAJOR})
	set(CLIENT_VERSION_MINOR ${btcu_VERSION_MINOR})
	set(CLIENT_VERSION_REVISION ${btcu_VERSION_PATCH})
	set(CLIENT_VERSION_BUILD ${btcu_VERSION_TWEAK})


	set(APP_VERSION "${CLIENT_VERSION_MAJOR}.${CLIENT_VERSION_MINOR}.${CLIENT_VERSION_REVISION}")

	set(COPYRIGHT_YEAR 2021)

	set(BITCOIN_QT_OSX_BUNDLE_NAME "btcu-qt")

	set(MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/share/qt/Info.plist")
	set(MACOSX_BUNDLE_BUNDLE_NAME "${BITCOIN_QT_OSX_BUNDLE_NAME}")
	set(MACOSX_BUNDLE_BUNDLE_VERSION "${APP_VERSION}")
	set(MACOSX_BUNDLE_GUI_IDENTIFIER "org.BTCU.${BITCOIN_QT_OSX_BUNDLE_NAME}")
	set(MACOSX_BUNDLE_ICON_FILE "${OSX_INSTALLER_ICONS_NAME}")
	set(MACOSX_BUNDLE_INFO_STRING "${APP_VERSION}.${CLIENT_VERSION_BUILD}, Copyright © ${COPYRIGHT_YEAR} BTCU")
	set(MACOSX_BUNDLE_LONG_VERSION_STRING "${APP_VERSION}")
	set(MACOSX_BUNDLE_SHORT_VERSION_STRING "${APP_VERSION}")

	configure_file(
		"${CMAKE_SOURCE_DIR}/share/qt/Info.plist.cmake.in"
		"${CMAKE_SOURCE_DIR}/share/qt/Info.plist"
	)
	set_target_properties(btcu-qt PROPERTIES
		MACOSX_BUNDLE ON
		OUTPUT_NAME "${BITCOIN_QT_OSX_BUNDLE_NAME}"
		MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/share/qt/Info.plist"
		MACOSX_BUNDLE_BUNDLE_NAME "${BITCOIN_QT_OSX_BUNDLE_NAME}"
		MACOSX_BUNDLE_BUNDLE_VERSION "${APP_VERSION}"
		MACOSX_BUNDLE_GUI_IDENTIFIER "org.BTCU.${BITCOIN_QT_OSX_BUNDLE_NAME}"
		MACOSX_BUNDLE_ICON_FILE "${OSX_INSTALLER_ICONS_NAME}"
		MACOSX_BUNDLE_INFO_STRING "${APP_VERSION}.${CLIENT_VERSION_BUILD}, Copyright © ${COPYRIGHT_YEAR} BTCU"
		MACOSX_BUNDLE_LONG_VERSION_STRING "${APP_VERSION}"
		MACOSX_BUNDLE_SHORT_VERSION_STRING "${APP_VERSION}"
		RESOURCE "${BITCOINQT_BUNDLE_RESOURCES}"
		INSTALL_RPATH "@executable_path/../Frameworks"
	)
	# The InfoPlist.strings files should be located in a resource subdirectory.
	# This is not supported by the RESOURCE property and require the use of the
	# MACOSX_PACKAGE_LOCATION property instead. The RESOURCE documentation has
	# an example demonstrating this behavior (see the appres.txt file):
	# https://cmake.org/cmake/help/latest/prop_tgt/RESOURCE.html
	set_source_files_properties(
		"${INFO_PLIST_STRINGS_PATH}"
		PROPERTIES
			MACOSX_PACKAGE_LOCATION "Resources/Base.lproj"
	)

	# Create a stripped version of the application bundle to be used in the DMG.
	# Since the LOCATION property and the BundleUtilities package are deprecated
	# by cmake, only generator expressions can be used to determine the path to
	# the bundle and its executable. However the generator expressions are
	# solved at build time, making them unusable to do path computation at
	# configuration time.
	# The paths here are then hard-coded, which is safe since the structure of
	# an application bundle is well-known and specified by Apple. Note that this
	# will only work for building MacOS application bundle as the IOS structure
	# is slightly different.
	set(STRIPPED_BUNDLE "${CMAKE_CURRENT_BINARY_DIR}/stripped/${BITCOIN_QT_OSX_BUNDLE_NAME}.app")
	add_custom_command(
		OUTPUT
			"${STRIPPED_BUNDLE}"
		COMMAND
			${CMAKE_COMMAND} -E copy_directory "$<TARGET_BUNDLE_DIR:btcu-qt>" "${STRIPPED_BUNDLE}"
		COMMAND
			${CMAKE_STRIP} -u -r "${STRIPPED_BUNDLE}/Contents/MacOS/${BITCOIN_QT_OSX_BUNDLE_NAME}"
		DEPENDS
			btcu-qt
	)

	include(DoOrFail)
	find_program_or_fail(CMAKE_INSTALL_NAME_TOOL "install_name_tool")
	find_program_or_fail(CMAKE_OTOOL "otool")

	set(QT_BASE_TRANSLATIONS
		"bg" "ca" "cs" "da" "de" "es" "fi" "fr_FR"
		"it" "ja" "ko_KR" "lt_LT" "pl" "pt" "ru" "sk" "sv" "uk" "zh_CN"
		"zh_TW"
	)
	string(JOIN "," QT_LOCALES ${QT_BASE_TRANSLATIONS})

	get_target_property(QMAKE_EXECUTABLE Qt5::qmake IMPORTED_LOCATION)
	execute_process(
		COMMAND
			"${QMAKE_EXECUTABLE}"
			-query QT_INSTALL_TRANSLATIONS
		OUTPUT_VARIABLE
			QT_TRANSLATION_DIR
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)

	function(get_qt_translation_dir QT_TRANSLATION_DIR)
		foreach(_locale ${ARGN})
			find_path(_qt_translation_dir
				"${_locale}.qm"
				HINTS
					"${QT_TRANSLATION_DIR}"
				PATH_SUFFIXES
					"translations"
			)

			# Ensure that all the translation files are found, and are located
			# in the same directory.
			if(NOT _qt_translation_dir OR (_qt_translation_dir_previous AND (NOT _qt_translation_dir_previous STREQUAL _qt_translation_dir)))
				return()
			endif()

			set(_qt_translation_dir_previous _qt_translation_dir)
		endforeach()

		set(QT_TRANSLATION_DIR ${_qt_translation_dir} PARENT_SCOPE)
	endfunction()

	get_qt_translation_dir(QT_TRANSLATION_DIR ${QT_BASE_TRANSLATIONS})
	if(NOT QT_TRANSLATION_DIR)
		message(FATAL_ERROR "Qt translation files are not found")
	endif()

	set(MACDEPLOY_DIR "${CMAKE_SOURCE_DIR}/contrib/macdeploy")
	set(MACDEPLOYQTPLUS "${MACDEPLOY_DIR}/macdeployqtplus.py")
	set(DMG_DIST "${CMAKE_BINARY_DIR}/dist")
	add_custom_command(
		OUTPUT
			"${DMG_DIST}"
		COMMAND
			"INSTALLNAMETOOL=${CMAKE_INSTALL_NAME_TOOL}"
			"OTOOL=${CMAKE_OTOOL}"
			"STRIP=${CMAKE_STRIP}"
			"${Python_EXECUTABLE}"
			"${MACDEPLOYQTPLUS}"
			"${STRIPPED_BUNDLE}"
			-translations-dir "${CMAKE_CURRENT_SOURCE_DIR}/locale"
			-add-qt-tr "${QT_LOCALES}"
			-sys-lib-dir "/usr/local/lib"
			-built-lib-dir "${CMAKE_INSTALL_PREFIX}/lib"
		WORKING_DIRECTORY
			"${CMAKE_BINARY_DIR}"
		DEPENDS
			"${STRIPPED_BUNDLE}"
	)

	# Building the DMG background image requires several steps:
	#   1/ The SVG file must be edited to display the package name
	#   2/ The SVG file should be transformed into a couple PNG files, on for
	#      low resolution screens and one for high resolution screens.
	#   3/ The PNG files must be transformed into a multi-resolution TIFF file.
	#      The names are not set arbitrarily, they follow Apple's guidelines for
	#      resolution independent bitmap images (see `man tiffutil`).
	set(BACKGROUND_SVG "background.svg")
	configure_file(
		"${CMAKE_SOURCE_DIR}/contrib/macdeploy/background.svg.cmake.in"
		"${BACKGROUND_SVG}"
	)

	include(ImageHelper)
	set(BACKGROUND_PNG_LOWRES "${CMAKE_CURRENT_BINARY_DIR}/background_temp.png")
	set(BACKGROUND_PNG_HIRES "${CMAKE_CURRENT_BINARY_DIR}/background_temp@2x.png")
	set(BACKGROUND_TIFF_LOWRES "${CMAKE_CURRENT_BINARY_DIR}/background_temp.tiff")
	set(BACKGROUND_TIFF_HIRES "${CMAKE_CURRENT_BINARY_DIR}/background_temp@2x.tiff")
	set(BACKGROUND_TIFF_NAME "background.tiff")
	set(BACKGROUND_TIFF_MULTIRES "${CMAKE_BINARY_DIR}/${BACKGROUND_TIFF_NAME}")
	convert_svg_to_png("${BACKGROUND_SVG}" "${BACKGROUND_PNG_LOWRES}" 36)
	convert_svg_to_png("${BACKGROUND_SVG}" "${BACKGROUND_PNG_HIRES}" 72)
	convert_png_to_tiff("${BACKGROUND_PNG_LOWRES}" "${BACKGROUND_TIFF_LOWRES}")
	convert_png_to_tiff("${BACKGROUND_PNG_HIRES}" "${BACKGROUND_TIFF_HIRES}")
	cat_multi_resolution_tiff("${BACKGROUND_TIFF_MULTIRES}" "${BACKGROUND_TIFF_LOWRES}" "${BACKGROUND_TIFF_HIRES}")

	set(BACKGROUND_DIST_DIR "${DMG_DIST}/.background")
	set(BACKGROUND_DIST_TIFF "${BACKGROUND_DIST_DIR}/${BACKGROUND_TIFF_NAME}")
	add_custom_command(
		OUTPUT
			"${BACKGROUND_DIST_TIFF}"
		COMMAND
			${CMAKE_COMMAND} -E make_directory "${BACKGROUND_DIST_DIR}"
		COMMAND
			${CMAKE_COMMAND} -E copy "${BACKGROUND_TIFF_MULTIRES}" "${BACKGROUND_DIST_TIFF}"
		DEPENDS
			"${BACKGROUND_TIFF_MULTIRES}"
			"${DMG_DIST}"
	)

	if (CMAKE_GENERATOR MATCHES "Xcode")
		set_target_properties(btcu-qt PROPERTIES
			XCODE_ATTRIBUTE_INSTALL_PATH "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}"
			XCODE_ATTRIBUTE_SKIP_INSTALL "No"
		)
	else()
		string(REPLACE " " "-" OSX_VOLNAME "${PACKAGE_NAME}")
		file(WRITE "${CMAKE_BINARY_DIR}/osx_volname" "${OSX_VOLNAME}")

		set(DMG_DSSTORE "${DMG_DIST}/.DS_Store")
		set(GEN_DSSTORE "${MACDEPLOY_DIR}/custom_dsstore.py")
		add_custom_command(
			OUTPUT
				"${DMG_DSSTORE}"
			COMMAND
				"${Python_EXECUTABLE}"
				"${GEN_DSSTORE}"
				"${DMG_DSSTORE}"
				"${OSX_VOLNAME}"
			DEPENDS
				"${GEN_DSSTORE}"
				"${DMG_DIST}"
		)

		set(OSX_APPLICATION_DIR "Applications")
		set(OSX_APPLICATION_SYMLINK "${DMG_DIST}/${OSX_APPLICATION_DIR}")
		add_custom_command(
			OUTPUT
				"${OSX_APPLICATION_SYMLINK}"
			COMMAND
				${CMAKE_COMMAND} -E create_symlink "/${OSX_APPLICATION_DIR}" "${OSX_APPLICATION_SYMLINK}"
			DEPENDS
				"${DMG_DIST}"
		)

		if (NOT CMAKE_GENERATOR MATCHES "Xcode")
			add_custom_target(osx-deploydir
				DEPENDS
					"${OSX_APPLICATION_SYMLINK}"
					"${DMG_DSSTORE}"
					"${BACKGROUND_DIST_TIFF}"
			)
		endif()

		set(LOCAL_LIB_PATH "${CMAKE_SOURCE_DIR}/lib")

		if(BUILTIN_QTDEPLOY)
			add_custom_target(osx-dmg
				COMMAND
					"${Python_EXECUTABLE}"
					"${MACDEPLOYQTPLUS}"
					"${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/btcu-qt.app"
					-dmg
					-fancy "${MACDEPLOY_DIR}/fancy.plist"
					-volname "${OSX_VOLNAME}"
					-sys-lib-dir "/usr/local/lib"
					-built-lib-dir "${LOCAL_LIB_PATH}"
					-no-processing
					-no-strip
				WORKING_DIRECTORY
					"${CMAKE_BINARY_DIR}"
				DEPENDS
					btcu-qt
					"${BACKGROUND_TIFF_MULTIRES}"
			)
		else()
			if(SIGN_DMG)
				add_custom_target(osx-dmg
					COMMAND
						"${Python_EXECUTABLE}"
						"${MACDEPLOYQTPLUS}"
						"${STRIPPED_BUNDLE}"
						-translations-dir "${CMAKE_CURRENT_SOURCE_DIR}/locale"
						-add-qt-tr "${QT_LOCALES}"
						-dmg
						-sign
						-codesignargs "${CODESIGNARGS}"
						-fancy "${MACDEPLOY_DIR}/fancy.plist"
						-volname "${OSX_VOLNAME}"
						-sys-lib-dir "/usr/local/lib"
						-built-lib-dir "${LOCAL_LIB_PATH}"
					WORKING_DIRECTORY
						"${CMAKE_BINARY_DIR}"
					DEPENDS
						"${STRIPPED_BUNDLE}"
						"${BACKGROUND_TIFF_MULTIRES}"
				)
			else()
				add_custom_target(osx-dmg
					COMMAND
						"${Python_EXECUTABLE}"
						"${MACDEPLOYQTPLUS}"
						"${STRIPPED_BUNDLE}"
						-translations-dir "${CMAKE_CURRENT_SOURCE_DIR}/locale"
						-add-qt-tr "${QT_LOCALES}"
						-dmg
						-fancy "${MACDEPLOY_DIR}/fancy.plist"
						-volname "${OSX_VOLNAME}"
						-sys-lib-dir "/usr/local/lib"
						-built-lib-dir "${LOCAL_LIB_PATH}"
					WORKING_DIRECTORY
						"${CMAKE_BINARY_DIR}"
					DEPENDS
						"${STRIPPED_BUNDLE}"
						"${BACKGROUND_TIFF_MULTIRES}"
				)
			endif()
		endif()
	endif()
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux" AND BUILD_STATIC)
        # TODO: add custom deb command
        # add_custom_target(btcu-qt_deb_package
        #     COMMAND "${CMAKE_CPACK_COMMAND}"
        #     "--config" "${CMAKE_SOURCE_DIR}/cmake/scripts/CPackConfig.cmake")
	install(TARGETS btcu-qt
		LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/btcu
		RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	)
	# this will be replaced with svg in next versions
	install (FILES ${CMAKE_SOURCE_DIR}/share/deb/btcu-qt.png DESTINATION share/icons/hicolor/16x16/apps)
	install (FILES ${CMAKE_SOURCE_DIR}/share/deb/btcu-qt.png DESTINATION share/icons/hicolor/24x24/apps)
	install (FILES ${CMAKE_SOURCE_DIR}/share/deb/btcu-qt.png DESTINATION share/icons/hicolor/32x32/apps)
	install (FILES ${CMAKE_SOURCE_DIR}/share/deb/btcu-qt.png DESTINATION share/icons/hicolor/48x48/apps)
	install (FILES ${CMAKE_SOURCE_DIR}/share/deb/btcu-qt.png DESTINATION share/icons/hicolor/64x64/apps)
	install (FILES ${CMAKE_SOURCE_DIR}/share/deb/btcu-qt.png DESTINATION share/icons/hicolor/128x128/apps)
	install (FILES ${CMAKE_SOURCE_DIR}/share/deb/btcu-qt.desktop DESTINATION share/applications)
endif()

if(ENABLE_TESTS)
	set(QT_TEST_SOURCES
			${CMAKE_CURRENT_SOURCE_DIR}/test/test_main.cpp
			${CMAKE_CURRENT_SOURCE_DIR}/test/uritests.cpp

			# wallet
			${CMAKE_CURRENT_SOURCE_DIR}/test/paymentservertests.cpp
		)

	add_executable(test_btcu-qt ${QT_TEST_SOURCES})
	add_dependencies(test_btcu-qt translations_target leveldb leveldb_sse42 memenv)
	target_include_directories(test_btcu-qt PUBLIC
			${ENDIAN_INCLUDES}
			${CMAKE_SOURCE_DIR}/src/eth_client/
			${CMAKE_CURRENT_SOURCE_DIR}/test
			${CMAKE_SOURCE_DIR}/src
			${CMAKE_SOURCE_DIR}/src/leveldb/include
			${CMAKE_SOURCE_DIR}/src/univalue/include
			${CMAKE_SOURCE_DIR}/src/secp256k1/include
			${CMAKE_CURRENT_SOURCE_DIR}
			${CMAKE_CURRENT_SOURCE_DIR}/btcu
			${CMAKE_CURRENT_SOURCE_DIR}/btcu/settings
			${OPENSSL_INCLUDE_DIR}
			${BerkeleyDB_INCLUDE_DIRS}
			${CMAKE_SOURCE_DIR}/src/eth_client/
		)

	target_link_libraries(test_btcu-qt PUBLIC
			qt_stuff
			univalue
			SERVER_A WALLET_A CLI_A UTIL_A COMMON_A ZEROCOIN_A BITCOIN_CRYPTO_A COMMON_B BITCOIN_CRYPTO_B SERVER_B
			leveldb leveldb_sse42 memenv secp256k1
			${BerkeleyDB_Cxx_LIBRARY}
			${OPENSSL_LIBRARIES}
			${Boost_LIBRARIES} 
			${PROTOBUF_LIBRARIES} 
			${LibEvent_LIBRARIES}
			${Qrcode_LIBRARIES}
			Qt5::Gui Qt5::Core Qt5::Widgets Qt5::Network Qt5::Svg Qt5::Test ${QT_LIBRARIES}
		)
	if(BUILD_STATIC)
		qt5_import_qml_plugins(test_btcu-qt)
		qt5_import_plugins(btcu-qt INCLUDE Qt5::QSvgPlugin Qt5::QSvgIconPlugin Qt5::QSvgIconPlugin Qt5::QGifPlugin Qt5::QJpegPlugin)
	endif()

	if(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Windows")
		target_link_libraries(test_btcu-qt PUBLIC pthread)
	endif()

	if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
		target_link_libraries(test_btcu-qt PUBLIC
			"-framework Foundation"
			"-framework ApplicationServices"
			"-framework AppKit"
		)
	endif()

	if(GMP_FOUND)
		target_link_libraries(test_btcu-qt PUBLIC ${GMP_LIBRARIES})
		target_include_directories(test_btcu-qt PUBLIC ${GMP_INCLUDE_DIR})
	endif()
	if (Qt5DBus_FOUND)
		target_link_libraries(test_btcu-qt PUBLIC Qt5::DBus ${QT_LIBRARIES})
	endif()
	if (Qt5Charts_FOUND)
		target_link_libraries(test_btcu-qt PUBLIC Qt5::Charts ${QT_LIBRARIES})
	endif()
	if(ZeroMQ_FOUND)
		target_link_libraries(test_btcu-qt PUBLIC ZMQ_A)
		if(${CMAKE_SYSTEM_NAME} MATCHES "Windows" OR BUILD_STATIC)
			target_link_libraries(test_btcu-qt PUBLIC
				ZeroMQ::zmq
				)
		else()
			target_link_libraries(test_btcu-qt PUBLIC
				${ZeroMQ_LIBRARY}
				)
		endif()
		target_include_directories(test_btcu-qt PUBLIC ${ZeroMQ_INCLUDE_DIRS})
	endif()
	if(MINIUPNP_FOUND)
		if(${CMAKE_SYSTEM_NAME} MATCHES "Windows" OR BUILD_STATIC)
			target_compile_definitions(test_btcu-qt
				PUBLIC -DSTATICLIB
				PUBLIC -DMINIUPNP_STATICLIB
			)
		endif()
		target_link_libraries(test_btcu-qt PUBLIC ${MINIUPNP_LIBRARY})
		target_include_directories(test_btcu-qt PUBLIC ${MINIUPNP_INCLUDE_DIR})
	endif()
endif()

