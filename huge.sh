#!/bin/bash

read -p "How many GB of HugePages to allocate? " gb

mb=$((gb * 1024))

page_size_mb=2

pages=$((mb / page_size_mb))

echo "Allocating $pages HugePages..."

echo $pages | sudo tee /proc/sys/vm/nr_hugepages

if ! mount | grep -q '/dev/hugepages'; then
  echo "Mounting hugetlbfs..."
  sudo mkdir -p /dev/hugepages
  sudo mount -t hugetlbfs none /dev/hugepages
fi

kib=$((gb * 1024 * 1024))
echo "Use the following values in your KVM XML:"
echo "  <memory unit=\"KiB\">$kib</memory>"
echo "  <currentMemory unit=\"KiB\">$kib</currentMemory>"
