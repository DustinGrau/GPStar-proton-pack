#!/bin/bash

# Perform a compile of all binaries using their respective architectures.

BINDIR="../binaries"
SRCDIR="../source"

echo ""

# COMPILE PACK/WAND COMPONENTS

source ./compile_reset.sh
source ./compile_pack.sh
source ./compile_wand.sh

# COMPILE ATTENUATOR AND VARIANTS

source ./compile_attenuator_esp.sh
source ./compile_attenuator_esp_extras.sh

# COMPILE STANDALONE/ADD-ON DEVICES

source ./compile_blaster.sh
source ./compile_gizmo.sh
source ./compile_stream.sh

# COMPILE GHOST TRAP COMPONENTS

source ./compile_trap_base.sh
source ./compile_trap_cartridge.sh
