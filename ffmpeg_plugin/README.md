# svt-vp9 ffmpeg plugin installation

1. Build and install SVT-VP9
```
git clone https://github.com/OpenVisualCloud/SVT-VP9
cd SVT-VP9
cd Build && cmake .. && make -j `nproc` && sudo make install
```

2. Apply SVT-VP9 plugin and enable libsvtvp9 to FFmpeg
```
git clone https://github.com/FFmpeg/FFmpeg ffmpeg
cd ffmpeg
git checkout release/4.1
git apply ../SVT-VP9/ffmpeg_plugin/0001-Add-ability-for-ffmpeg-to-run-svt-vp9.patch
export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:/usr/local/lib
export PKG_CONFIG_PATH=${PKG_CONFIG_PATH}:/usr/local/lib/pkgconfig
./configure --enable-libsvtvp9
make -j `nproc`
```

3. Verify
ffmpeg is now built with svt-vp9, sample command line: 
```./ffmpeg  -i input.mp4 -c:v libsvt_vp9 -rc 1 -b:v 10M -preset 1  -y test.ivf```
```./ffmpeg  -i input.mp4 -vframes 1000 -c:v libsvt_vp9 -y test.mp4```

