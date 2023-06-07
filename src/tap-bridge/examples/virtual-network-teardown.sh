#!/bin/bash
lxc-stop -n left
lxc-stop -n right
lxc-destroy -n left
lxc-destroy -n right
ip link set dev br-left down
ip link set dev br-right down
ip link set dev tap-left nomaster
ip link set dev tap-right nomaster
ip link del br-left
ip link del br-right
ip link set dev tap-left down
ip link set dev tap-right down
ip tuntap del mode tap tap-left
ip tuntap del mode tap tap-right
