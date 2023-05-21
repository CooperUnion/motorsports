#include "ember_bltools.h"
#include "ember_taskglue.h"
#include "ember_tasking.h"
#include "module_list.inc"

void app_main()
{
    /* set boot partition back to bootloader */
    ember_bltools_set_boot_partition_to_factory();

    /* begin running tasks */
    ember_tasking_begin();
}

// temp: remove when all ECUs have module_rf defined
__attribute__((weak)) ember_rate_funcs_S module_rf = { NULL };
