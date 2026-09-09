#!/usr/bin/env bash
set -e
cd "$(dirname "$0")"
ISO="build/charos.iso"
VM_NAME="charOS-Live"
MEM=2048
CPUS=2
DISK="build/charos.vdi"

if [ ! -f "$ISO" ]; then
  echo "ISO bulunamadı: $ISO"
  echo "make all ile ISO oluşturun"
  exit 1
fi

# VM oluştur
if VBoxManage list vms | grep -q "\"$VM_NAME\""; then
  echo "VM zaten var: $VM_NAME"
else
  VBoxManage createvm --name "$VM_NAME" --ostype Linux26_64 --register
  VBoxManage modifyvm "$VM_NAME" --memory $MEM --cpus $CPUS --vram 64 --accelerate3d off
  VBoxManage modifyvm "$VM_NAME" --firmware UEFI
  VBoxManage modifyvm "$VM_NAME" --boot1 dvd --boot2 disk --boot3 none --boot4 none
  VBoxManage storagectl "$VM_NAME" --name "SATA" --add sata --controller IntelAhci
fi

# Disk oluştur
if [ ! -f "$DISK" ]; then
  VBoxManage createmedium disk --filename "$DISK" --size 20480 --format VDI
  VBoxManage storageattach "$VM_NAME" --storagectl "SATA" --port 0 --device 0 --type hdd --medium "$DISK"
fi

# ISO bağla
VBoxManage storageattach "$VM_NAME" --storagectl "SATA" --port 1 --device 0 --type dvddrive --medium "$ISO"

echo "VM hazır: $VM_NAME"
echo "VBoxManage startvm \"$VM_NAME\" --type headless"
