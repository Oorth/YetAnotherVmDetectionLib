#pragma once
#include <Windows.h>
#include <iostream>
#include <intrin.h>
#include <vector>


typedef unsigned long DWORD, * PDWORD, * LPDWORD;


extern "C" void RunVMwareBackdoor();

#ifdef __INTELLISENSE__
    void RunVMwareBackdoor() {}
#endif

/*
    to implement -

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

#pragma pack(push, 1)
struct DESCRIPTOR_TABLE
{
    unsigned short Limit;       // 2 bytes
    unsigned long long Base;    // 8 bytes
};
#pragma pack(pop)

namespace signatures
{
    static const char* HYPERVISORS[] =
    {
        "vbox",         // Covers: VirtualBox, VBoxHardened, VBoxGuest
        "vmware",       // Covers: Workstation, Fusion, ESX, GSX
        "qemu",         // Covers: QEMU, KVM+QEMU
        "kvm",          // Covers: KVM, Amazon EC2 (often), Linux KVM
        "microsoft hv", // Covers: Hyper-V, Azure, Windows Sandbox
        "xen",          // Covers: Xen, AWS (older), Citrix
        "parallels",    // Covers: Parallels Desktop
        "virtu"         // GENERIC CATCH-ALL: Virtual PC, VirtualBox, VirtualHD
    };

    // Specific Sandbox / Analysis Tool Signatures
    static const char* SANDBOXES[] =
    {
        "cuckoo",       // Cuckoo Sandbox
        "sandboxie",    // Sandboxie
        "wine",         // Wine (Emulation)
        "docker",       // Docker containers
        "bochs",        // Bochs Emulator
        "joebox",       // JoeSandbox
        "hybrid",       // Hybrid Analysis
        "anubis"        // Anubis
    };

    // Cloud / Enterprise specific
    static const char* CLOUD[] =
    {
        "amazon",       // AWS Nitro / EC2
        "google",       // Google Cloud (GCE)
        "azure"         // Microsoft Azure
    };
}

// ---------------------Fordward declaration----------------------

bool HasSignature(const std::string& input, const char* signatures[], int size);

// ----------------------Very light stuff--------------------------

bool GetHypervisorVendor()
{

    /*
         Common Signatures to look for:
         "Microsoft Hv"  -> Hyper-V / Virtual PC
         "VMwareVMware"  -> VMware
         "VBoxVBoxVBox"  -> VirtualBox
         "KVMKVMKVM"     -> KVM / QEMU
         "Prl Hyperv"    -> Parallels
    */


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

bool CLOCK()
{
    const int Threshold = 1000;
    uint64_t t1, t2, current_cycles = 0;
    uint64_t min_cycles = ~0ULL;
    
    // We will not use these
    int data[4];
    unsigned int aux = 0;


    // Loop to avoid cpu from switching task
    for(int i = 0; i < 10; ++i)
    {

        t1 = __rdtsc();

            __cpuid(data, 0);

        t2 = __rdtscp(&aux);

        current_cycles = t2 - t1;
        if(current_cycles < min_cycles) min_cycles = current_cycles;

    }

    if(min_cycles > Threshold) return true;
    else return false;
}

bool VMWARE_BACKDOOR()
{

    bool is_vm = false;

    __try
    {
        RunVMwareBackdoor();
        is_vm = true;
    }
    __except(1)
    {
        is_vm = false;
    }

    return is_vm;
}

// --------------------Some dependencies--------------------------

bool CheckSMBIOS()
{
    const DWORD signature = 'RSMB';

    DWORD size = GetSystemFirmwareTable(signature, 0, nullptr, 0);
    if(size == 0) return false;

    std::vector<char> buffer(size);
    if(GetSystemFirmwareTable(signature, 0, buffer.data(), size) != size) return false;

    std::string tableData(buffer.begin(), buffer.end());


    for(char& c : tableData) c = static_cast<char>(::tolower(static_cast<unsigned char>(c)));

    if(HasSignature(tableData, signatures::HYPERVISORS, sizeof(signatures::HYPERVISORS) / sizeof(char*))) return true;
    if(HasSignature(tableData, signatures::SANDBOXES, sizeof(signatures::SANDBOXES) / sizeof(char*))) return true;

    return false;
}

bool BOOT_LOGO()
{
    typedef struct acpi_table_header
    {
        char Signature[4];
        UINT32 Length;
        UINT8 Revision;
        UINT8 Checksum;
        char OemId[6];
        char OemTableId[8];
        UINT32 OemRevision;
        char AslCompilerId[4];
        UINT32 AslCompilerRevision;
    } ACPI_TABLE_HEADER;

    const DWORD BGRT_SIGNATURE = 'TRGB';

    DWORD size = GetSystemFirmwareTable('ACPI', BGRT_SIGNATURE, nullptr, 0);

    if(size == 0)
    {
        // Not strictly "VM Detected", but "No Boot Logo Table Found"
        std::cout << "[-] BGRT Table missing (Legacy Boot or VM)." << std::endl;
        return true;
    }

    std::vector<char> buffer(size);
    GetSystemFirmwareTable('ACPI', BGRT_SIGNATURE, buffer.data(), size);

    ACPI_TABLE_HEADER* header = reinterpret_cast<ACPI_TABLE_HEADER*>(buffer.data());

    std::string oemId(header->OemId, 6);
    std::string oemTableId(header->OemTableId, 8);

    std::cout << "[-] ACPI BGRT OEM ID: " << oemId << std::endl;
    std::cout << "[-] ACPI BGRT Table ID: " << oemTableId << std::endl;

    if(HasSignature(oemId, signatures::HYPERVISORS,sizeof(signatures::HYPERVISORS) / sizeof(char*)))
    {
        std::cout << "[!] VM DETECTED via Boot Logo (OEM ID): " << oemId << std::endl;
        return true;
    }

    if(HasSignature(oemTableId, signatures::HYPERVISORS, sizeof(signatures::HYPERVISORS) / sizeof(char*)))
    {
        std::cout << "[!] VM DETECTED via Boot Logo (Table ID): " << oemTableId << std::endl;
        return true;
    }

    return false;
}

bool DISK_SERIAL()
{

    HANDLE hPhysicalDrive = NULL;

    SECURITY_ATTRIBUTES sa = { 0 };
    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = FALSE;

    hPhysicalDrive = CreateFileA("\\\\.\\PhysicalDrive0", 0, FILE_SHARE_READ, &sa, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL);
    if(hPhysicalDrive == INVALID_HANDLE_VALUE)
    {
        std::cout << "CreateFileA failed error: " << GetLastError() << std::endl;
        return true;
    }

    STORAGE_PROPERTY_QUERY PropQuery = {};
    PropQuery.PropertyId = StorageDeviceProperty;
    PropQuery.QueryType = PropertyStandardQuery;
    PropQuery.AdditionalParameters[0] = NULL;

    char buffer[1024] = { 0 };
    DWORD bytesReturned = 0;
    if(!DeviceIoControl(hPhysicalDrive, IOCTL_STORAGE_QUERY_PROPERTY, &PropQuery, sizeof(PropQuery), buffer, sizeof(buffer), &bytesReturned, NULL))
    {
        CloseHandle(hPhysicalDrive);
        return false;
    }

    STORAGE_DEVICE_DESCRIPTOR* pDesc = (STORAGE_DEVICE_DESCRIPTOR*)buffer;
    
    if(pDesc->ProductIdOffset > 0)
    {
        std::string model = &buffer[pDesc->ProductIdOffset];
        std::cout << "Disk Model: " << model << std::endl;

        if(HasSignature(model, signatures::HYPERVISORS, sizeof(signatures::HYPERVISORS) / sizeof(char*)))
        {
            std::cout << "[!] VM DETECTED via Boot Logo (Table ID): " << model << std::endl;
            return true;
        }
    }

    CloseHandle(hPhysicalDrive);
    return false;
}

// ----------------------------Helpers-----------------------------

bool HasSignature(const std::string& input, const char* signatures[], int size)
{

    std::string lowerInput = input;
    for(size_t i = 0; i < lowerInput.length(); i++) lowerInput[i] = std::tolower((unsigned char)lowerInput[i]);

    for(int i = 0; i < size; i++)
    {
        if(lowerInput.find(signatures[i]) != std::string::npos)
        {
            VM::brand = signatures[i];
            return true;
        }
    }
    return false;
}


//bool CheckDescriptorTables()
//{
//    unsigned short ldt = GetLDT();
//
//    if(ldt != 0)
//    {
//        // VM Detected (LDT is being used)
//        return true;
//    }
//
//    // -------------------------------------------
//        // 2. Check IDT / GDT (Optional Heuristics)
//        // -------------------------------------------
//    DESCRIPTOR_TABLE idt = { 0 };
//    DESCRIPTOR_TABLE gdt = { 0 };
//
//    GetIDT(&idt);
//    GetGDT(&gdt);
//
//    // Detailed output for analysis
//    std::cout << "[-] LDT:  " << std::hex << ldt << std::endl;
//    std::cout << "[-] IDTR Base: " << std::hex << idt.Base << std::endl;
//    std::cout << "[-] GDTR Base: " << std::hex << gdt.Base << std::endl;
//
//    // Just for your info (you can log these)
//    // If Base address is 0 (unlikely) or looks weird, you can flag it.
//    // But LDT != 0 is the strongest simple indicator here.
//
//    return false;
//}