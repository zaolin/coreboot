void i82801er_hard_reset(void);

/* FIXME: There's another hard_reset() in romstage.c. Why? */
void hard_reset(void)
{
	i82801er_hard_reset();
}
