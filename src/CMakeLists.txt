project(delphioracle)

set(EOSIO_WASM_OLD_BEHAVIOR "Off")
find_package(eosio.cdt)

add_contract( delphioracle delphioracle delphioracle.cpp )
target_include_directories( delphioracle PUBLIC ${CMAKE_SOURCE_DIR}/../include/delphioracle )
target_ricardian_directory( delphioracle ${CMAKE_SOURCE_DIR}/../ricardian )