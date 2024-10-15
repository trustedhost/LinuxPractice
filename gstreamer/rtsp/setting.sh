#!/bin/sh
# install gstreamer package
sudo apt install libglib2.0-dev bison flex libtool autoconf automake autopoint gtk-doc-tools libx264-dev nasm yasm cmake libmicrohttpd-dev libjansson-dev libnice-dev librtmp-dev meson libavfilter-dev
#libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev liborc-0.4-dev
#sudo apt purge libgstreamer1.0-0 libgstreamer-plugins-base1.0-dev
#sudo apt-get upgrade gstreamer1.0*
sudo apt remove liborc-0.4-0
sudo ldconfig
# update mason
cd
git clone https://github.com/mesonbuild/meson.git
cd meson/
sudo mv /usr/bin/meson /usr/bin/meson-0.56.2
sudo ln -s ~/meson/meson.py /usr/bin/meson
cd
meson --version
# GStreamer compile & install
git clone git://anongit.freedesktop.org/gstreamer/gstreamer
cd gstreamer
git clone https://gitlab.freedesktop.org/gstreamer/gst-build.git
meson build && ninja -C build && sudo ninja -C build install
cd ..
sudo mv /usr/local/lib/aarch64-linux-gnu/libgstgl-1.0.so /usr/local/lib/aarch64-linux-gnu/libgstgl-1.0.so-old
# GStreamer plugins-base compile & install
git clone git://anongit.freedesktop.org/gstreamer/gst-plugins-base
cd gst-plugins-base
meson build && ninja -C build && sudo ninja -C build install
cd ..
# GStreamer plugins-good compile & install
git clone git://anongit.freedesktop.org/gstreamer/gst-plugins-good
cd gst-plugins-good
meson build && ninja -C build && sudo ninja -C build install
cd ..
# GStreamer plugins-bad compile & install
git clone git://anongit.freedesktop.org/gstreamer/gst-plugins-bad
cd gst-plugins-bad
meson build && ninja -C build && sudo ninja -C build install
cd ..
# GStreamer plugins-ugly compile & install
git clone git://anongit.freedesktop.org/gstreamer/gst-plugins-ugly
cd gst-plugins-ugly
meson build && ninja -C build && sudo ninja -C build install
cd ..
# GStreamer FFmpeg compile & install
git clone git://anongit.freedesktop.org/gstreamer/gst-ffmpeg
cd gst-ffmpeg
meson build && ninja -C build && sudo ninja -C build install
cd ..