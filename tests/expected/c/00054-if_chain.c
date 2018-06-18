void foo(void)
{
	if (cond_a) {
		fcn_a();
		fcn_b();
	} else {
		fcn_c();
	}

	if (cond_b)
		fcn_d();
	else
		fcn_e();

	if (cond_c) {
		fcn_f();
		fcn_g();
	} else {
		fcn_h();
	}

	if (cond_d) {
		fcn_i();
	} else {
		fcn_j();
		fcn_k();
	}

	if (cond_e)
		fcn_l();
	else
		fcn_m();

	if (cond_f) {
		fcn_n();
	} else if (cond_g) {
		fcn_o();
		while (cond_g)
			fcn_p();
	} else if (cond_h) {
		while (cond_i) {
			fcn_q();
			fcn_r();
		}
	} else {
		fcn_s();
	}
}

/* this next bit test whether vbraces can be successfully converted
 * when the closing brace is in an #ifdef.
 * Note that the author should have braced the code to begin with.
 */
void bar(void)
{
	if (jiffies >= hw_priv->Counter[ port ].time) {
		hw_priv->Counter[ port ].fRead = 1;
		if (port == MAIN_PORT)
			hw_priv->Counter[ MAIN_PORT ].time = jiffies + HZ * 6;
		else
			hw_priv->Counter[ port ].time =

#ifdef SOME_DEFINE
				hw_priv->Counter[ port - 1 ].time + HZ * 2;

#else /* ifdef SOME_DEFINE */
				hw_priv->Counter[ MAIN_PORT ].time + HZ * 2;
#endif /* ifdef SOME_DEFINE */
	}
}

void funct(int v1, int v2, int v3)
{
	if (v1) {
		if (v2) f1();
	} else {
		if (v3) f2();
	}
}
