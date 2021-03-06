#!/bin/bash
#################################################################
#								#
# Copyright (c) 2019 YottaDB LLC and/or its subsidiaries.	#
# All rights reserved.						#
#								#
#	This source code contains the intellectual property	#
#	of its copyright holder(s), and is made available	#
#	under a license.  If you do not know the terms of	#
#	the license, please stop and do not read further.	#
#								#
#################################################################

source /opt/yottadb/current/ydb_env_set

start_dir=$(pwd)

# CMake commands are different between CentOS and Ubuntu
# disambiguate them here and make it a varible
if [ -x "$(command -v cmake3)" ]; then
  cmakeCommand="cmake3"
else
  cmakeCommand="cmake"
fi

# Download, Compile, and Install the YottaDB POSIX plugin
cd /root
git clone https://gitlab.com/YottaDB/Util/YDBposix.git
cd YDBposix
mkdir build
cd build
${cmakeCommand} ..
make
make install

# Source the ENV script again to YottaDB environment variables after installing POSIX plugin
source /opt/yottadb/current/ydb_env_unset
source /opt/yottadb/current/ydb_env_set
echo "Done setting up POSIX plugin"
echo "ydb_routines: $ydb_routines"

# Download and Install BATS testing framework
cd $start_dir
mkdir build
cd build
git clone https://github.com/bats-core/bats-core.git
cd bats-core
./install.sh /usr/local
cd ..

# Configure the build system for Octo
${cmakeCommand} -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_INSTALL_PREFIX=${ydb_dist}/plugin ..

# Compile Octo
make 2> make_warnings.txt

# Check for unexpected warnings and error/exit if unexpected errors are found
../tools/ci/sort_warnings.sh
echo -n "Checking for unexpected warning(s)... "
if [ -x "$(command -v yum)" ]; then
  diff sorted_warnings.txt ../tools/ci/expected_warnings-centos.ref &> differences.txt
else
  diff sorted_warnings.txt ../tools/ci/expected_warnings.ref &> differences.txt
fi

if [ $(wc -l differences.txt | awk '{print $1}') -gt 0 ]; then
  echo "New build warnings detected!"
  cat differences.txt
  exit 1
fi
echo "OK."

source activate
pushd src

# Setup for tests
$ydb_dist/mupip set -n=true -reg '*'

# Load the data required for tests
./octo -f ../../tests/fixtures/names.sql
$ydb_dist/mupip load ../../tests/fixtures/names.zwr
popd

# Run the tests
make test || exit 1

# Build binary package
make package
