#pragma once

void bmb_monitor_task(void * unused);
bool bmb_monitor_ready_for_reboot(void);
float bmb_monitor_get_lowest_cell_v(void);
float bmb_monitor_get_highest_cell_v(void);

// do what's right | made with <3 at Cooper Union
