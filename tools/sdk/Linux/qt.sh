#!/bin/bash -eux

cd /image

NPROC=$(nproc)

yum install perl-version libxcb libxcb-devel xcb-util xcb-util-devel fontconfig-devel libX11-devel libXrender-devel libXi-devel git

git clone https://code.qt.io/qt/qt5.git

(
  cd qt5
  git checkout 5.6.1
  perl init-repository --module-subset=qtbase,qtimageformats,qtsvg,qtwebsockets,qttranslations,qtrepotools,qtdeclarative
)

mkdir qt5-build
(
  cd qt5-build
  ../qt5/configure -release \
                   -opensource \
                   -confirm-license \
                   -qt-zlib \
                   -no-mtdev \
                   -nomake examples \
                   -nomake tests \
                   
)