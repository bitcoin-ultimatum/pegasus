#!/bin/bash

echo  ""
echo  "[63%] Downloading latest version of the BTCU... "
#git clone https://github.com/bitcoin-ultimatum/orion --recursive
#cd orion

echo  ""
echo  "[65%] Installing Berkeley DB... "


if [ -f /opt/lib/libdb-18.1.a ]
then
    echo  "[65%] Berkeley DB is already installed."
else
    # Since as of 5th March 2020 the Oracle moved Barkeley DB 
    # to login-protected tarball for 18.1.32 version 
    # we added the dependency as a static file included in the repository.
    # You can check the details in depends/packages/static/berkeley-db-18.1.32/README.MD

    tar zxvf depends/packages/static/berkeley-db-18.1.32/berkeley-db-18.1.32.tar.gz -C ./
    cd  db-18.1.32/build_unix
    ../dist/configure --enable-cxx --disable-shared --disable-replication --with-pic --prefix=/opt && make &&  make install
    cd -
fi

echo  ""
echo  "[68%] Installing Berkeley DB... Done!"

echo  ""
echo  "[68%] Running CMake configuring... "

cmake -G "CodeBlocks - Unix Makefiles" . -DENABLE_GUI=OFF -DQUIET=ON

echo  ""
echo  "[71%] Running CMake configuring... Done!"

echo  ""
echo  "[72%] Building BTCU... "

make btcud btcu-cli

echo  ""
echo  "[90%] Building BTCU... Done!"

echo  ""
echo  "[100%] Build is completed!"

echo  ""
echo  ""
echo  ""

echo  "=========================================================="
echo  "The built binaries was placed in ./bin folder"
echo  "For start daemon please run:"
echo  "./bin/btcud -daemon"
echo  "Outputs a list of command-line options:"
echo  "./bin/btcu-cli --help"
echo  "=========================================================="
