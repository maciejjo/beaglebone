#include <stdio.h>
#include <stdlib.h>
#include <prussdrv.h>
#include <pruss_intc_mapping.h>

int main()
{
	void *pru0_memory;
	unsigned int *pru0_memory_uint;
	int distance_samples;
	int n;

	tpruss_intc_initdata pruss_intc_initdata = PRUSS_INTC_INITDATA;

	prussdrv_init();
	prussdrv_open(PRU_EVTOUT_0);

	prussdrv_pruintc_init(&pruss_intc_initdata);
	prussdrv_map_prumem(PRUSS0_PRU0_DATARAM, &pru0_memory);
	pru0_memory_uint = (unsigned int*)pru0_memory;

	prussdrv_exec_program(0, "./hcsr04.bin");

	n = prussdrv_pru_wait_event(PRU_EVTOUT_0);
	distance_samples = *pru0_memory_uint;

	printf("Measured distance: %f cm\n", (float)distance_samples / (100.0 * 50.));

	prussdrv_pru_disable(0);
	prussdrv_exit();
	return 0;
}
