# SVT-VP9 ffmpeg plugin installation

Assumes you have a bash (or similar) shell along with commands such as

- git
- cmake
- make
- pkg-config
- cc

If you do not have any of these, please consult your friendly web browser to determine what is the best way to get these on your system, else ask IT.

If you are on windows, the recommended way to get these tools and build FFmpeg is through [msys2](https://www.msys2.org/). The way to get these specific tools will not be provided here though.

If you are trying to compile FFmpeg using MSVC, please try to consult FFmpeg's [wiki page](https://trac.ffmpeg.org/wiki/CompilationGuide/MSVC) about it.

## Build and install SVT-VP9

```bash
git clone https://github.com/OpenVisualCloud/SVT-VP9.git
cd SVT-VP9/Build
# Adjust this line to add any other cmake related options
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j $(nproc)
sudo make install
cd ..
```

## Enable SVT-VP9 plugin

```bash
# Adjust /usr/local to your chosen prefix
export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:/usr/local/lib
export PKG_CONFIG_PATH=${PKG_CONFIG_PATH}:/usr/local/lib/pkgconfig
git clone https://github.com/FFmpeg/FFmpeg.git ffmpeg
cd ffmpeg
```

Which patch to apply will depend on which version of FFmpeg you are going to use.

master:

```bash
git apply ../ffmpeg_plugin/master-0001-Add-ability-for-ffmpeg-to-run-svt-vp9.patch
```

n4.3.1 tag:

```bash
git checkout n4.3.1
git apply ../ffmpeg_plugin/n4.3.1-0001-Add-ability-for-ffmpeg-to-run-svt-vp9.patch
```

n4.2.3 tag:

```bash
git checkout n4.2.3
git apply ../ffmpeg_plugin/n4.2.3-0001-Add-ability-for-ffmpeg-to-run-svt-vp9.patch
```

n4.2.2 tag:

```bash
git checkout n4.2.2
git apply ../ffmpeg_plugin/0001-Add-ability-for-ffmpeg-to-run-svt-vp9.patch
```

```bash
./configure --enable-libsvtvp9
make -j $(nproc)
```

## Verify

Run basic tests to check if `libsvt_vp9` works

```bash
./ffmpeg -f lavfi -i testsrc=duration=1:size=1280x720:rate=30 -c:v libsvt_vp9 -rc 1 -b:v 10M -preset 1 -y test.ivf
./ffmpeg -f lavfi -i testsrc=duration=1:size=1280x720:rate=30 -vframes 1000 -c:v libsvt_vp9 -y test.mp4
```

## Note

These patches can be applied in any ordering permutation along with the other SVT encoders, there is no strict ordering
