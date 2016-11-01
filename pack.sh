#!/bin/bash
#/home/vagrant/baidu/a20/lichee/buildroot/dl/gst-jpcnn-1.0.tar.gz
make clean
rm -rf /home/vagrant/baidu/a20/lichee/out/linux/common/buildroot/build/gst-jpcnn-1.0/
rm -rf /home/vagrant/baidu/a20/lichee/buildroot/dl/gst-jpcnn-1.0.tar.gz
pushd /home/vagrant/
rm /tmp/gst-jpcnn-1.0.tar.gz
tar -zcf /tmp/gst-jpcnn-1.0.tar.gz gst-jpcnn-1.0
popd
cp /tmp/gst-jpcnn-1.0.tar.gz /home/vagrant/baidu/a20/lichee/buildroot/dl/gst-jpcnn-1.0.tar.gz
