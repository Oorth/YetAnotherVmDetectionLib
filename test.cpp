#include "YetAnotherVmDetectionLib.hpp"

int main()
{

    //if(GetHypervisorVendor()) std::cout << "Hypervisor found -> " << VM::brand << std::endl;

    //if(CLOCK()) std::cout << "Hypervisor found CLOCK\n";

    //if(VMWARE_BACKDOOR()) std::cout << "Hypervisor found VMWARE_BACKDOOR\n";

    //if(CheckSMBIOS()) std::cout << "Hypervisor found CheckSMBIOS\n";

    //if(BOOT_LOGO()) std::cout << "Hypervisor found BOOT_LOGO\n";

    if(DISK_SERIAL()) std::cout << "Hypervisor found DISK_SERIAL\n";

    
    
    else std::cout << "Not a VM  :) DISK_SERIAL\n";

    return 0;
}