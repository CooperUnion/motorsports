#include "module_list.inc"

#include <stdio.h>

#include <ember_bltools.h>
#include <ember_taskglue.h>
#include <ember_tasking.h>

void app_main()
{
    printf("### FSAE FIRMWARE BOOT ###");
    /* set boot partition back to bootloader */
    ember_bltools_set_boot_partition_to_factory();

    /* begin running tasks */
    ember_tasking_begin();
}

// do what's right | made with <3 at Cooper Union
