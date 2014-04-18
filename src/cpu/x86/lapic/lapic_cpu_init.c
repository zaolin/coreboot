#include <cpu/x86/lapic.h>
#include <delay.h>
#include <string.h>
#include <console/console.h>
#include <arch/hlt.h>
#include <device/device.h>
#include <device/path.h>
#include <smp/atomic.h>
#include <smp/spinlock.h>
#include <cpu/cpu.h>


#if CONFIG_SMP == 1
/* This is a lot more paranoid now, since Linux can NOT handle
 * being told there is a CPU when none exists. So any errors 
 * will return 0, meaning no CPU. 
 *
 * We actually handling that case by noting which cpus startup
 * and not telling anyone about the ones that dont.
 */ 
static int lapic_start_cpu(unsigned long apicid)
{
	int timeout;
	unsigned long send_status, accept_status, start_eip;
	int j, num_starts, maxlvt;
	extern char _secondary_start[];
		
	/*
	 * Starting actual IPI sequence...
	 */

	printk_spew("Asserting INIT.\n");

	/*
	 * Turn INIT on target chip
	 */
	lapic_write_around(LAPIC_ICR2, SET_LAPIC_DEST_FIELD(apicid));

	/*
	 * Send IPI
	 */
	
	lapic_write_around(LAPIC_ICR, LAPIC_INT_LEVELTRIG | LAPIC_INT_ASSERT
				| LAPIC_DM_INIT);

	printk_spew("Waiting for send to finish...\n");
	timeout = 0;
	do {
		printk_spew("+");
		udelay(100);
		send_status = lapic_read(LAPIC_ICR) & LAPIC_ICR_BUSY;
	} while (send_status && (timeout++ < 1000));
	if (timeout >= 1000) {
		printk_err("CPU %d: First apic write timed out. Disabling\n",
			 apicid);
		// too bad. 
		printk_err("ESR is 0x%x\n", lapic_read(LAPIC_ESR));
		if (lapic_read(LAPIC_ESR)) {
			printk_err("Try to reset ESR\n");
			lapic_write_around(LAPIC_ESR, 0);
			printk_err("ESR is 0x%x\n", lapic_read(LAPIC_ESR));
		}
		return 0;
	}
	mdelay(10);

	printk_spew("Deasserting INIT.\n");

	/* Target chip */
	lapic_write_around(LAPIC_ICR2, SET_LAPIC_DEST_FIELD(apicid));

	/* Send IPI */
	lapic_write_around(LAPIC_ICR, LAPIC_INT_LEVELTRIG | LAPIC_DM_INIT);
	
	printk_spew("Waiting for send to finish...\n");
	timeout = 0;
	do {
		printk_spew("+");
		udelay(100);
		send_status = lapic_read(LAPIC_ICR) & LAPIC_ICR_BUSY;
	} while (send_status && (timeout++ < 1000));
	if (timeout >= 1000) {
		printk_err("CPU %d: Second apic write timed out. Disabling\n",
			 apicid);
		// too bad. 
		return 0;
	}

	start_eip = (unsigned long)_secondary_start;
	printk_spew("start_eip=0x%08lx\n", start_eip);
       
	num_starts = 2;

	/*
	 * Run STARTUP IPI loop.
	 */
	printk_spew("#startup loops: %d.\n", num_starts);

	maxlvt = 4;

	for (j = 1; j <= num_starts; j++) {
		printk_spew("Sending STARTUP #%d to %u.\n", j, apicid);
		lapic_read_around(LAPIC_SPIV);
		lapic_write(LAPIC_ESR, 0);
		lapic_read(LAPIC_ESR);
		printk_spew("After apic_write.\n");

		/*
		 * STARTUP IPI
		 */

		/* Target chip */
		lapic_write_around(LAPIC_ICR2, SET_LAPIC_DEST_FIELD(apicid));

		/* Boot on the stack */
		/* Kick the second */
		lapic_write_around(LAPIC_ICR, LAPIC_DM_STARTUP
					| (start_eip >> 12));

		/*
		 * Give the other CPU some time to accept the IPI.
		 */
		udelay(300);

		printk_spew("Startup point 1.\n");

		printk_spew("Waiting for send to finish...\n");
		timeout = 0;
		do {
			printk_spew("+");
			udelay(100);
			send_status = lapic_read(LAPIC_ICR) & LAPIC_ICR_BUSY;
		} while (send_status && (timeout++ < 1000));

		/*
		 * Give the other CPU some time to accept the IPI.
		 */
		udelay(200);
		/*
		 * Due to the Pentium erratum 3AP.
		 */
		if (maxlvt > 3) {
			lapic_read_around(LAPIC_SPIV);
			lapic_write(LAPIC_ESR, 0);
		}
		accept_status = (lapic_read(LAPIC_ESR) & 0xEF);
		if (send_status || accept_status)
			break;
	}
	printk_spew("After Startup.\n");
	if (send_status)
		printk_warning("APIC never delivered???\n");
	if (accept_status)
		printk_warning("APIC delivery error (%lx).\n", accept_status);
	if (send_status || accept_status)
		return 0;
	return 1;
}

/* Number of cpus that are currently running in linuxbios */
static atomic_t active_cpus = ATOMIC_INIT(1);

/* start_cpu_lock covers last_cpu_index and secondary_stack.
 * Only starting one cpu at a time let's me remove the logic
 * for select the stack from assembly language.
 *
 * In addition communicating by variables to the cpu I
 * am starting allows me to veryify it has started before
 * start_cpu returns.
 */

static spinlock_t start_cpu_lock = SPIN_LOCK_UNLOCKED;
static unsigned last_cpu_index = 0;
volatile unsigned long secondary_stack;

int start_cpu(device_t cpu)
{
	extern unsigned char _estack[];
	struct cpu_info *info;
	unsigned long stack_end;
	unsigned long apicid;
	unsigned long index;
	unsigned long count;
	int result;

	spin_lock(&start_cpu_lock);

	/* Get the cpu's apicid */
	apicid = cpu->path.u.apic.apic_id;

	/* Get an index for the new processor */
	index = ++last_cpu_index;
	
	/* Find end of the new processors stack */
	stack_end = ((unsigned long)_estack) - (STACK_SIZE*index) - sizeof(struct cpu_info);
	
	/* Record the index and which cpu structure we are using */
	info = (struct cpu_info *)stack_end;
	info->index = index;
	info->cpu   = cpu;

	/* Advertise the new stack to start_cpu */
	secondary_stack = stack_end;

	/* Until the cpu starts up report the cpu is not enabled */
	cpu->enabled = 0;
	cpu->initialized = 0;

	/* Start the cpu */
	result = lapic_start_cpu(apicid);

	if (result) {
		result = 0;
		/* Wait 1s or until the new the new cpu calls in */
		for(count = 0; count < 100000 ; count++) {
			if (secondary_stack == 0) {
				result = 1;
				break;
			}
			udelay(10);
		}
	}
	secondary_stack = 0;
	spin_unlock(&start_cpu_lock);
	return result;
}

/* C entry point of secondary cpus */
void secondary_cpu_init(void)
{
	atomic_inc(&active_cpus);
	cpu_initialize();
	atomic_dec(&active_cpus);
	stop_this_cpu();
}

static void initialize_other_cpus(device_t root)
{
	int old_active_count, active_count;
	device_t cpu;
	/* Loop through the cpus once getting them started */
	for(cpu = root->link[1].children; cpu ; cpu = cpu->sibling) {
		if (cpu->path.type != DEVICE_PATH_APIC) {
			continue;
		}
		if (!cpu->enabled) {
			continue;
		}
		if (cpu->initialized) {
			continue;
		}
		if (!start_cpu(cpu)) {
			/* Record the error in cpu? */
			printk_err("CPU  %u would not start!\n",
				cpu->path.u.apic.apic_id);
		}
	}

	/* Now loop until the other cpus have finished initializing */
	old_active_count = 1;
	active_count = atomic_read(&active_cpus);
	while(active_count > 1) {
		if (active_count != old_active_count) {
			printk_info("Waiting for %d CPUS to stop\n", active_count);
			old_active_count = active_count;
		}
		udelay(10);
		active_count = atomic_read(&active_cpus);
	}
	for(cpu = root->link[1].children; cpu; cpu = cpu->sibling) {
		if (cpu->path.type != DEVICE_PATH_APIC) {
			continue;
		}
		if (!cpu->initialized) {
			printk_err("CPU %u did not initialize!\n", 
				cpu->path.u.apic.apic_id);
#warning "FIXME do I need a mainboard_cpu_fixup function?"
		}
	}
	printk_debug("All AP CPUs stopped\n");
}

#else /* CONFIG_SMP */
#define initialize_other_cpus(root) do {} while(0)
#endif /* CONFIG_SMP */

void initialize_cpus(device_t root)
{
	struct device_path cpu_path;
	struct cpu_info *info;

	/* Find the info struct for this cpu */
	info = cpu_info();

#if NEED_LAPIC == 1
	/* Ensure the local apic is enabled */
	enable_lapic();

	/* Get the device path of the boot cpu */
	cpu_path.type           = DEVICE_PATH_APIC;
	cpu_path.u.apic.apic_id = lapicid();
#else
	/* Get the device path of the boot cpu */
	cpu_path.type           = DEVICE_PATH_BOOT_CPU;
#endif
	
	/* Find the device structure for the boot cpu */
	info->cpu = alloc_find_dev(&root->link[1], &cpu_path);
	
	/* Initialize the bootstrap processor */
	cpu_initialize();

	/* Now initialize the rest of the cpus */
	initialize_other_cpus(root);
}

