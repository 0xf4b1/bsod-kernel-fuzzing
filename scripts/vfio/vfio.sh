#!/bin/bash

vfio_bind() {
	modprobe vfio-pci

    for address in $1; do
        echo "bind ${address}"
        vendor=$(cat /sys/bus/pci/devices/$address/vendor)
        device=$(cat /sys/bus/pci/devices/$address/device)
        if [ -e /sys/bus/pci/devices/$address/driver ]; then
                echo $address > /sys/bus/pci/devices/$address/driver/unbind
        fi
        echo $vendor $device > /sys/bus/pci/drivers/vfio-pci/new_id
    done
}

vfio_unbind() {
    for address in $1; do
        vendor=$(cat /sys/bus/pci/devices/$address/vendor)
        device=$(cat /sys/bus/pci/devices/$address/device)
        echo "Removing ${address} from vfio-pci id list"
            echo "${vendor} ${device}" > /sys/bus/pci/drivers/vfio-pci/remove_id
        sleep 0.1
        echo "Remove PCI device"
        echo 1 > /sys/bus/pci/devices/${address}/remove
        while [[ -e "/sys/bus/pci/devices/${address}" ]]; do
            sleep 0.1
        done
        echo "Rescanning..."
        echo 1 > /sys/bus/pci/rescan
        while [[ ! -e "/sys/bus/pci/devices/${address}" ]]; do
            sleep 0.1
        done
    done

    rmmod vfio-pci
}