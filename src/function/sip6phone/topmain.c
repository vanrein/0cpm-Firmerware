

/* The main operational loop for the CPU.  Try to run code, and when
 * nothing remains to be done, stop to wait for interrupts.
 */
void top_main (void) {
	// TODO: init ();
	netdb_initialise ();
	// TODO: cpu_add_closure (X);
	// TODO: cpu_add_closure (Y);
	// TODO: cpu_add_closure (Z);
	bottom_critical_region_end ();
	top_network_online ();	// TODO: Should be called from bottom!
	while (true) {
		jobhopper ();
		bottom_sleep_prepare ();
		if (cur_prio == CPU_PRIO_ZERO) {
			bottom_sleep_commit (SLEEP_SNOOZE);
		}
	}
}

