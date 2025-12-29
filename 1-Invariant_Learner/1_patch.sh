wget https://github.com/AFLplusplus/AFLplusplus/archive/refs/tags/v4.21c.tar.gz

tar -xf v4.21c.tar.gz

patch -p0 < aflpp.patch


git clone https://github.com/AFLplusplus/qemuafl.git
cd qemuafl/
git checkout a6f0632a65e101e680dd72643a6128dd180dff72
cd ../

patch -p0 < qemuafl.patch
