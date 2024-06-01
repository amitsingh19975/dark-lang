# find_package(PkgConfig REQUIRED)
# pkg_check_modules(gmp REQUIRED IMPORTED_TARGET gmp)
# PkgConfig::gmp

find_package(PkgConfig REQUIRED)
pkg_check_modules(gmpxx REQUIRED IMPORTED_TARGET gmpxx)