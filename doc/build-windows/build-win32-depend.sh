#!/bin/bash

#build build make tools
pacman -S mingw-w64-i686-gcc --noconfirm
pacman -S pkg-config libtool automake autoconf --noconfirm

#build build dependencies
pacman -S mingw-w64-i686-qt5 --noconfirm
#pacman -S mingw-w64-i686-openssl
pacman -S mingw-w64-i686-db --noconfirm
pacman -S mingw-w64-i686-boost --noconfirm
pacman -S mingw-w64-i686-miniupnpc --noconfirm
pacman -S mingw-w64-i686-protobuf --noconfirm
pacman -S mingw-w64-i686-gmp --noconfirm
#pacman -S mingw-w64-i686-libpng --noconfirm
pacman -S mingw-w64-i686-libwinpthread --noconfirm
pacman -S mingw-w64-i686-libevent --noconfirm

#build qrencode library, it is building manually because not present in pacman packages
wget fukuchi.org/works/qrencode/qrencode-3.4.4.tar.gz
tar xvfz qrencode-3.4.4.tar.gz
cd qrencode-3.4.4
./configure --enable-static --disable-shared --without-tools
make
make install

cd /mingw32/lib
for file in *.dll.a; do
	if [ -f "`basename "$file" .dll.a`.a" ]
	then
		echo "`basename "$file" .dll.a`.a found."
	else
		cp "$file" "`basename "$file" .dll.a`.a"
	fi
done

cd /src
#checkout the right branch for building PoS
#git clone https://gitlab.qtum.org/qtum_project/qtum.git
cd qtum
#git fetch origin master-pos
#git checkout master-pos

#configure quantum
./configure --with-incompatible-bdb CXXFLAGS="-I/mingw32/include/" --libdir="/mingw32/lib" --with-boost-libdir="/mingw32/lib" --with-qt-libdir="/mingw32/lib" --with-qt-incdir="/mingw32/include" --disable-tests --prefix="/mingw32/qtum"

#make and install quantum into the folder /mingw32/qtum
make
make install
