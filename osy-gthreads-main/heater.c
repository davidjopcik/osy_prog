#include <stdio.h>
#include "gthr.h"

int heater_id;
int cooler_id;

int temperature = 21;

void task_sensor(void)
{
    while (1)
    {
        temperature += (rand() % 3) - 1;
        printf("[SENSOR] Temperature: %d°C\n", temperature);

        if (temperature < 18)
        {
            printf("[SENSOR] Temperature low → waking HEATER\n");
            gt_resume(heater_id);
            temperature += 2;
        }
        else if (temperature > 23)
        {
            printf("[SENSOR] Temperature high → waking COOLER\n");
            gt_resume(cooler_id);
            temperature -= 2;
        }

        gt_delay(500); 
    }
}

void task_heater(void)
{
    while (1)
    {
        printf("[HEATER] Going to sleep...\n");
        gt_suspend();

        printf("[HEATER] Heating ON! 🔥\n");
        gt_delay(2000); 

        printf("[HEATER] Heating OFF.\n");
    }
}

void set_temp(void){
    while(1) {
    char c;
    ssize_t i = read(0, &c, 1);
    if(c == '+') {
        temperature += 1;
        printf("Temperature raised to: %d\n", temperature);
    }
    else if(c == '-') {
        temperature -= 1;
        printf("Temperature lowered to: %d\n", temperature);
    }
}
}


void task_cooler(void)
{
    while (1)
    {
        printf("[COOLER] Going to sleep...\n");
        gt_suspend();

        printf("[COOLER] Cooling ON! ❄️\n");
        gt_delay(2000); 

        printf("[COOLER] Cooling OFF.\n");
    }
}

int main(void)
{
    gt_init();

    gt_go("Sensor", task_sensor);
    heater_id = gt_go("Heater", task_heater);
    cooler_id = gt_go("Cooler", task_cooler);

    gt_go("Setter", set_temp);

    gt_start_scheduler();
    return 0;
}