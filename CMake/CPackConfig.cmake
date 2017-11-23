set(CPACK_PACKAGE_CONTACT "Juan Jose Garcia Cantero <juanjosegarciacan@gmail.com>")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${PROJECT_SOURCE_DIR}/README.md")
set(CPACK_PACKAGE_LICENSE "Proprietary")

set(CPACK_DEBIAN_PACKAGE_DEPENDS "libboost-test-dev")

include(CommonCPack)
