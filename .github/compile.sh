#!/bin/bash

BINDIR="../binaries"
SRCDIR="../source"

mkdir -p ${BINDIR}/attenuator
mkdir -p ${BINDIR}/pack
mkdir -p ${BINDIR}/wand/extras

# Proton Pack
echo "Building Proton Pack Binary..."
arduino-cli compile --output-dir ${BINDIR} -b arduino:avr:mega -e ${SRCDIR}/ProtonPack/ProtonPack.ino

rm -f ${BINDIR}/*.bin
rm -f ${BINDIR}/*.eep
rm -f ${BINDIR}/*.elf
rm -f ${BINDIR}/*bootloader.hex

mv ${BINDIR}/ProtonPack.ino.hex ${BINDIR}/pack/ProtonPack.hex
echo "Done."

# Neutrona Wand
echo "Building Neutrona Wand Binary..."
arduino-cli compile --output-dir ${BINDIR} -b arduino:avr:mega -e ${SRCDIR}/NeutronaWand/NeutronaWand.ino

rm -f ${BINDIR}/*.bin
rm -f ${BINDIR}/*.eep
rm -f ${BINDIR}/*.elf
rm -f ${BINDIR}/*bootloader.hex

mv ${BINDIR}/NeutronaWand.ino.hex ${BINDIR}/wand/NeutronaWand.hex
echo "Done."

# Attenuator (Arduino)
echo "Building Attenuator Binary (Arduino)..."
arduino-cli compile --output-dir ${BINDIR} -b arduino:avr:nano -e ${SRCDIR}/Attenuator/Attenuator.ino

rm -f ${BINDIR}/*.bin
rm -f ${BINDIR}/*.eep
rm -f ${BINDIR}/*.elf
rm -f ${BINDIR}/*bootloader.hex

mv ${BINDIR}/Attenuator.ino.hex ${BINDIR}/attenuator/Attenuator-Nano.hex
echo "Done."

# Attenuator (ESP32)
echo "Building Attenuator Binary (ESP32)..."
arduino-cli compile --output-dir ${BINDIR} -b esp32:esp32:uPesy_wroom -e ${SRCDIR}/Attenuator/Attenuator.ino

# Keep any .bin files
rm -f ${BINDIR}/*.eep
rm -f ${BINDIR}/*.elf
rm -f ${BINDIR}/*.map
rm -f ${BINDIR}/*bootloader.hex

mv ${BINDIR}/Attenuator.ino.bin ${BINDIR}/attenuator/Attenuator-ESP32.hex
echo "Done."

# Neutrona Wand (Bench Test)
echo "Building Neutrona Wand (Bench Test) Binary..."
arduino-cli compile --output-dir ${BINDIR} -b arduino:avr:mega -e ${SRCDIR}/NeutronaWand/NeutronaWand.ino

# Change flag(s) for compilation
sed -i -e 's/b_gpstar_benchtest = false/b_gpstar_benchtest = true/g' ${SRCDIR}/NeutronaWand/Configuration.h

rm -f ${BINDIR}/*.bin
rm -f ${BINDIR}/*.eep
rm -f ${BINDIR}/*.elf
rm -f ${BINDIR}/*bootloader.hex

mv ${BINDIR}/NeutronaWand.ino.hex ${BINDIR}/wand/extras/NeutronaWand-BenchTest.hex

# Restore flag(s) from compilation
sed -i -e 's/b_gpstar_benchtest = true/b_gpstar_benchtest = false/g' ${SRCDIR}/NeutronaWand/Configuration.h

echo "Done."