#pragma once
#include <iostream>
#include <intrin.h>

/*
    to implement -
    CLOCK
    VMWARE_BACKDOOR
    SIDT / SGDT / SLD

    ACPI_SIGNATURE / BOOT_LOGO
    DISK_SERIAL
    DISPLAY / EDID
    MAC
    MUTEX / OBJECTS
    DLL / WINE

    VIRTUAL_REGISTRY / DEVICE_STRING
    DRIVERS
    CUCKOO_PIPE

*/

struct VM
{
    inline static std::string brand;

};


/* 
     Common Signatures to look for:
     "Microsoft Hv"  -> Hyper-V / Virtual PC
     "VMwareVMware"  -> VMware
     "VBoxVBoxVBox"  -> VirtualBox
     "KVMKVMKVM"     -> KVM / QEMU
     "Prl Hyperv"    -> Parallels
*/
bool GetHypervisorVendor()
{

    int cpuInfo[4] = { 0 };

    __cpuid(cpuInfo, 1);
    bool hypervisorBitSet = (cpuInfo[2] & (1 << 31)) != 0;

    if(!hypervisorBitSet) return false;

    // Query Hypervisor Vendor ID (Leaf 0x40000000)
    // The string is returned in EBX, ECX, EDX
    __cpuid(cpuInfo, 0x40000000);

    char vendor[13];
    memset(vendor, 0, sizeof(vendor));

    *reinterpret_cast<int*>(vendor) = cpuInfo[1]; // EBX
    *reinterpret_cast<int*>(vendor + 4) = cpuInfo[2]; // ECX
    *reinterpret_cast<int*>(vendor + 8) = cpuInfo[3]; // EDX

    VM::brand = vendor;

    return true;
}
