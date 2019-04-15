# svt-vp9 ffmpeg plugin installation

1. Build and install SVT-VP9
- git clone https://github.com/OpenVisualCloud/SVT-VP9
- cd SVT-VP9
- cd Build && cmake .. && make -j `nproc` && sudo make install

2. Apply SVT-VP9 plugin and enable libsvtvp9 to FFmpeg
- git clone https://github.com/FFmpeg/FFmpeg ffmpeg
- cd ffmpeg
- git checkout release/4.1
- export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:/usr/local/lib
- export PKG_CONFIG_PATH=${PKG_CONFIG_PATH}:/usr/local/lib/pkgconfig
- SVT-VP9 alone:
   - git apply SVT-VP9/ffmpeg_plugin/0001-Add-ability-for-ffmpeg-to-run-svt-vp9.patch
   - ./configure --enable-libsvtvp9
- Based on SVT-HEVC & SVT-AV1:
   - git apply SVT-HEVC/ffmpeg_plugin/0001-lavc-svt_hevc-add-libsvt-hevc-encoder-wrapper.patch
   - git apply SVT-AV1/ffmpeg_plugin/0001-Add-ability-for-ffmpeg-to-run-svt-av1-with-svt-hevc.patch
   - git apply SVT-VP9/ffmpeg_plugin/0001-Add-ability-for-ffmpeg-to-run-svt-vp9-with-svt-hevc-av1.patch
   - ./configure --enable-libsvthevc --enable-libsvtav1 --enable-libsvtvp9
- make -j `nproc`

3. Verify
- ./ffmpeg  -i input.mp4 -c:v libsvt_vp9 -rc 1 -b:v 10M -preset 1  -y test.ivf
- ./ffmpeg  -i input.mp4 -vframes 1000 -c:v libsvt_vp9 -y test.mp4

