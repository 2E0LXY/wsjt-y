# Building from Source

---

## Linux (Debian / Ubuntu 22.04+)

### Install build dependencies

```bash
sudo apt-get update
sudo apt-get install -y \
  build-essential cmake ninja-build gfortran \
  qtbase5-dev qt5-qmake qttools5-dev qttools5-dev-tools \
  qtmultimedia5-dev libqt5multimedia5-plugins \
  libqt5serialport5-dev libqt5websockets5-dev \
  libqt5sql5-sqlite libqt5svg5-dev \
  libfftw3-dev libboost-log-dev libboost-thread-dev \
  libhamlib-dev libhamlib-utils \
  libusb-1.0-0-dev libudev-dev portaudio19-dev \
  dpkg-dev
```

### Clone and build

```bash
git clone https://github.com/2E0LXY/wsjt-zii
cd wsjt-zii
mkdir build && cd build

cmake -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX=/usr \
  -DWSJT_SKIP_MANPAGES=ON \
  -DWSJT_GENERATE_DOCS=OFF \
  ..

ninja -j$(nproc)
```

### Create .deb package

```bash
cpack -G DEB
sudo dpkg -i wsjtz_*.deb
```

### CMake options

| Option | Default | Description |
|--------|---------|-------------|
| `WSJT_WITH_OMNIRIG` | `ON` | OmniRig COM server support (Windows only) |
| `WSJT_GENERATE_DOCS` | `ON` | Requires asciidoctor |
| `WSJT_SKIP_MANPAGES` | `OFF` | Skip man page generation |

---

## Windows (MSYS2 / MinGW64)

This is the approach used by the CI. For a manual build, follow the same steps.

### 1. Install MSYS2

Download and install MSYS2 from https://www.msys2.org/. Open the **MSYS2 MinGW x64** shell.

### 2. Update and install packages

```bash
pacman -Syu                         # update base packages
pacman -S \
  mingw-w64-x86_64-gcc \
  mingw-w64-x86_64-gcc-fortran \
  mingw-w64-x86_64-cmake \
  mingw-w64-x86_64-ninja \
  mingw-w64-x86_64-qt5-base \
  mingw-w64-x86_64-qt5-multimedia \
  mingw-w64-x86_64-qt5-serialport \
  mingw-w64-x86_64-qt5-websockets \
  mingw-w64-x86_64-qt5-svg \
  mingw-w64-x86_64-qt5-tools \
  mingw-w64-x86_64-fftw \
  mingw-w64-x86_64-boost \
  mingw-w64-x86_64-libusb \
  mingw-w64-x86_64-portaudio \
  mingw-w64-x86_64-openssl \
  mingw-w64-x86_64-readline \
  mingw-w64-x86_64-nsis \
  autoconf automake libtool make
```

### 3. Pin GCC to 14.2.0-3

GCC 15+ has a `.tls_common` regression that breaks Fortran thread-local storage. Pin the compiler binaries:

```bash
BASE=https://repo.msys2.org/mingw/mingw64
pacman -U --noconfirm --nodeps \
  "${BASE}/mingw-w64-x86_64-gcc-14.2.0-3-any.pkg.tar.zst" \
  "${BASE}/mingw-w64-x86_64-gcc-fortran-14.2.0-3-any.pkg.tar.zst"
```

See [Windows Build Pitfalls](Windows-Build-Pitfalls) for a full explanation.

### 4. Build Hamlib from source

Hamlib is not available as an MSYS2 package and must be compiled:

```bash
curl -L https://github.com/Hamlib/Hamlib/releases/download/4.5.5/hamlib-4.5.5.tar.gz | tar xz
cd hamlib-4.5.5
./configure --prefix=/mingw64 --disable-static --without-cxx-binding --disable-winradio
make -j$(nproc)
make install
cd ..
```

### 5. Configure and build

```bash
git clone https://github.com/2E0LXY/wsjt-zii
cd wsjt-zii
mkdir build && cd build

cmake -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX=/mingw64 \
  -DCMAKE_PREFIX_PATH=/mingw64 \
  -DWSJT_SKIP_MANPAGES=ON \
  -DWSJT_GENERATE_DOCS=OFF \
  -DWSJT_WITH_OMNIRIG=OFF \
  ..

ninja -j$(nproc)
```

### 6. Create NSIS installer

```bash
cpack -G NSIS
ls *.exe
```

---

## GitHub Actions CI

The repository includes two workflow files in `.github/workflows/`:

| File | Trigger | Output |
|------|---------|--------|
| `linux-deb.yml` | Push to tag `v*` or manual dispatch | `wsjtz_*.deb` |
| `windows-installer.yml` | Push to tag `v*` or manual dispatch | `wsjtz-*-win64.exe` |

To trigger a manual build: **GitHub → Actions → [workflow] → Run workflow**.

### Releasing a new version

1. Update `Versions.cmake` with the new version
2. Commit and push to `master`
3. Tag: `git tag -a vX.Y.Z -m "Release X.Y.Z" && git push origin vX.Y.Z`
4. Both CI workflows trigger automatically and attach assets to the GitHub release
