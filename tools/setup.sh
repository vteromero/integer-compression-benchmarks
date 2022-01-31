#!/bin/bash

sudo cpupower frequency-set --governor performance

echo 1 | sudo tee /sys/devices/system/cpu/intel_pstate/no_turbo
