mkdir build
rm -rf build/AFLplusplus-4.21c/
cp -rf AFLplusplus-4.21c/ build/

cp Binvariants_aflpp.h build/AFLplusplus-4.21c/include/
cp Binvariants_aflpp.c build/AFLplusplus-4.21c/src/

cp Binvariants_common.h build/AFLplusplus-4.21c/include/
cp Binvariants_common.c build/AFLplusplus-4.21c/src/

cd build/AFLplusplus-4.21c/
make all -j$(nproc)


cd ../../
rm -rf build/qemuafl/
cp -rf qemuafl/ build/qemuafl/

cp Binvariants_qemu.h build/qemuafl/include/
cp Binvariants_qemu.c build/qemuafl/linux-user/

cp Binvariants_common.h build/qemuafl/include/
cp Binvariants_common.c build/qemuafl/linux-user/

CPU_TARGET="`uname -m`"

QEMU_CONF_FLAGS=" \
  --enable-plugins \
  --audio-drv-list= \
  --disable-bochs \
  --disable-brlapi \
  --disable-bsd-user \
  --disable-bzip2 \
  --disable-cap-ng \
  --disable-cloop \
  --disable-curl \
  --disable-curses \
  --disable-dmg \
  --disable-fdt \
  --disable-gcrypt \
  --disable-glusterfs \
  --disable-gnutls \
  --disable-gtk \
  --disable-guest-agent \
  --disable-iconv \
  --disable-libiscsi \
  --disable-libnfs \
  --disable-libssh \
  --disable-libusb \
  --disable-linux-aio \
  --disable-live-block-migration \
  --disable-lzo \
  --disable-nettle \
  --disable-numa \
  --disable-opengl \
  --disable-parallels \
  --disable-qcow1 \
  --disable-qed \
  --disable-rbd \
  --disable-rdma \
  --disable-replication \
  --disable-sdl \
  --disable-seccomp \
  --disable-smartcard \
  --disable-snappy \
  --disable-system \
  --disable-spice \
  --disable-tools \
  --disable-tpm \
  --disable-usb-redir \
  --disable-vde \
  --disable-vdi \
  --disable-vhost-crypto \
  --disable-vhost-kernel \
  --disable-vhost-net \
  --disable-vhost-user \
  --disable-vhost-vdpa \
  --disable-virglrenderer \
  --disable-virtfs \
  --disable-vnc \
  --disable-vnc-jpeg \
  --disable-vnc-sasl \
  --disable-vte \
  --disable-vvfat \
  --disable-xen \
  --disable-xen-pci-passthrough \
  --target-list="${CPU_TARGET}-linux-user" \
  --without-default-devices \
  --extra-cflags=-Wno-int-conversion \
  --disable-werror \
  --disable-debug-info \
  --disable-debug-mutex \
  --disable-debug-tcg \
  --disable-qom-cast-debug \
  --disable-stack-protector \
  --disable-docs \
  "

cd build/qemuafl/
./configure $QEMU_CONF_FLAGS || exit 1

make -j$(nproc) V=1 || { echo "[*] Build failed. Exiting ..."; exit 1; }

echo "[*] Build succeeded. Copying qemu binary ..."

cp build/qemu-$CPU_TARGET ../AFLplusplus-4.21c/afl-qemu-trace || { echo "[*] Error when copying build/qemu-$CPU_TARGET. Exiting ..."; exit 1; }