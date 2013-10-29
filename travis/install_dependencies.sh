echo "Installing compilation dependencies."

# Install GCC-ARM Compiler.

echo "Adding apt repositories."
sudo add-apt-repository -y ppa:terry.guo/gcc-arm-embedded > /dev/null
sudo add-apt-repository -y ppa:ubuntu-wine/ppa > /dev/null

echo "Installing gcc_arm software"
sudo apt-get update > /dev/null
sudo apt-get install -y gcc-arm-none-eabi > /dev/null

echo "Setting up Wine."
sudo apt-get install -y wine1.5 > /dev/null


# Download ARMCC (Testing Purposes only at the moment)

echo "Installing ARMCC"
wget https://dl.dropboxusercontent.com/u/15449666/ARMCC.tar.gz > /dev/null
tar xvfz ARMCC.tar.gz > /dev/null

# Setup ARMCC environment variables

echo "Setting up Environment Variables"
printf "#%s/bin/bash\nwine armcc.exe" ! > ARMCC/bin/armcc
chmod a+x ARMCC/bin/armcc

printf "#%s/bin/bash\nwine armar.exe" ! > ARMCC/bin/armar
chmod a+x ARMCC/bin/armar

printf "#%s/bin/bash\nwine armasm.exe" ! > ARMCC/bin/armasm
chmod a+x ARMCC/bin/armasm

printf "#%s/bin/bash\nwine armlink.exe" ! > ARMCC/bin/armlink
chmod a+x ARMCC/bin/armlink

printf "#%s/bin/bash\nwine fromelf.exe" ! > ARMCC/bin/fromelf
chmod a+x ARMCC/bin/fromelf

export PATH=$PATH:$TRAVIS_BUILD_DIR"/ARMCC/bin"
