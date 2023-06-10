import time
import cand

bus = cand.client.Bus()

counter = 0
inverter_disable_count = 0

while True:
    counter = 0 if counter == 15 else counter + 1

    # inverter_enable = 1 if inverter_disable_count > 200 else 0
    # inverter_disable_count += 1
    inverter_enable = 0

    bus.send('M192_Command_Message', {'Torque_Command': 10.0, 'Speed_Command': 100.0, 'Direction_Command': 0, 'Inverter_Enable': inverter_enable,
                                      'Inverter_Discharge': 0, 'Speed_Mode_Enable': 1, 'RollingCounter': counter, 'Torque_Limit_Command': 50.0})
    time.sleep(0.003)
