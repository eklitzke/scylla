#!/bin/sh -e

if [ ! -e dist/ami/build_ami.sh ] || [ ! -e ../scylla-jmx/dist/redhat/build_rpm.sh ] || [ ! -e ../cassandra/dist/redhat/build_rpm.sh ]; then
    echo "run build_ami.sh in top of scylla dir"
    echo "please make sure scylla-jmx is checked out under the same directory as scylla"
    echo "please make sure cassandra with scylla tools branch checked out under the same directory as scylla"
    exit 1
fi

cd dist/ami

if [ ! -f variables.json ]; then
    echo "create variables.json before start building AMI"
    exit 1
fi

if [ ! -f files/scylla-server.rpm ] || [ ! -f files/scylla-server-debuginfo.rpm ]; then
    cd ../../
    dist/redhat/build_rpm.sh
    RPM=`ls build/rpms/scylla-server-*.x86_64.rpm|grep -v debuginfo`
    cp $RPM dist/ami/files/scylla-server.rpm
    cp build/rpms/scylla-server-debuginfo-*.x86_64.rpm dist/ami/files/scylla-server-debuginfo.rpm
    cd -
fi

if [ ! -f files/scylla-jmx.rpm ]; then
    CWD=`pwd`
    cd ../../../scylla-jmx
    dist/redhat/build_rpm.sh
    RPM=`ls build/rpms/scylla-jmx-*.noarch.rpm`
    cp $RPM $CWD/files/scylla-jmx.rpm
    cd -
fi

if [ ! -f files/scylla-tools.rpm ]; then
    CWD=`pwd`
    cd ../../../cassandra
    dist/redhat/build_rpm.sh
    RPM=`ls build/rpms/scylla-tools-*.noarch.rpm`
    cp $RPM $CWD/files/scylla-tools.rpm
    cd -
fi

if [ ! -d packer ]; then
    wget https://dl.bintray.com/mitchellh/packer/packer_0.8.6_linux_amd64.zip
    mkdir packer
    cd packer
    unzip -x ../packer_0.8.6_linux_amd64.zip
    cd -
fi

packer/packer build -var-file=variables.json scylla.json
