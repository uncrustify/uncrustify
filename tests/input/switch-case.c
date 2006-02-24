
asmlinkage unsigned long
osf_setsysinfo(unsigned long op, void __user *buffer, unsigned long nbytes,
	       int __user *start, void __user *arg)
{
	switch (op) {
	case SSI_IEEE_FP_CONTROL: {
		unsigned long swcr, fpcr;
		unsigned int *state;

		/*
		 * Alpha Architecture Handbook 4.7.7.3:
		 * To be fully IEEE compiant, we must track the current IEEE
		 * exception state in software, because spurrious bits can be
		 * set in the trap shadow of a software-complete insn.
		 */

		if (get_user(swcr, (unsigned long __user *)buffer))
			return -EFAULT;
		state = &current_thread_info()->ieee_state;

		/* Update softare trap enable bits.  */
		*state = (*state & ~IEEE_SW_MASK) | (swcr & IEEE_SW_MASK);

		/* Update the real fpcr.  */
		fpcr = rdfpcr() & FPCR_DYN_MASK;
		fpcr |= ieee_swcr_to_fpcr(swcr);
		wrfpcr(fpcr);

		return 0;
	}

	case SSI_IEEE_RAISE_EXCEPTION: {
		unsigned long exc, swcr, fpcr, fex;
		unsigned int *state;

		if (get_user(exc, (unsigned long __user *)buffer))
			return -EFAULT;
		state = &current_thread_info()->ieee_state;
		exc &= IEEE_STATUS_MASK;

		/* Update softare trap enable bits.  */
 		swcr = (*state & IEEE_SW_MASK) | exc;
		*state |= exc;

		/* Update the real fpcr.  */
		fpcr = rdfpcr();
		fpcr |= ieee_swcr_to_fpcr(swcr);
		wrfpcr(fpcr);

 		/* If any exceptions set by this call, and are unmasked,
		   send a signal.  Old exceptions are not signaled.  */
		fex = (exc >> IEEE_STATUS_TO_EXCSUM_SHIFT) & swcr;
 		if (fex) {
			siginfo_t info;
			int si_code = 0;

			if (fex & IEEE_TRAP_ENABLE_DNO) si_code = FPE_FLTUND;
			if (fex & IEEE_TRAP_ENABLE_INE) si_code = FPE_FLTRES;
			if (fex & IEEE_TRAP_ENABLE_UNF) si_code = FPE_FLTUND;
			if (fex & IEEE_TRAP_ENABLE_OVF) si_code = FPE_FLTOVF;
			if (fex & IEEE_TRAP_ENABLE_DZE) si_code = FPE_FLTDIV;
			if (fex & IEEE_TRAP_ENABLE_INV) si_code = FPE_FLTINV;

			info.si_signo = SIGFPE;
			info.si_errno = 0;
			info.si_code = si_code;
			info.si_addr = NULL;  /* FIXME */
 			send_sig_info(SIGFPE, &info, current);
 		}
		return 0;
	}

	case SSI_IEEE_STATE_AT_SIGNAL:
	case SSI_IEEE_IGNORE_STATE_AT_SIGNAL:
		/*
		 * Not sure anybody will ever use this weird stuff.  These
		 * ops can be used (under OSF/1) to set the fpcr that should
		 * be used when a signal handler starts executing.
		 */
		break;

 	case SSI_NVPAIRS: {
		unsigned long v, w, i;
		unsigned int old, new;
		
 		for (i = 0; i < nbytes; ++i) {

 			if (get_user(v, 2*i + (unsigned int __user *)buffer))
 				return -EFAULT;
 			if (get_user(w, 2*i + 1 + (unsigned int __user *)buffer))
 				return -EFAULT;
 			switch (v) {
 			case SSIN_UACPROC:
			again:
				old = current_thread_info()->flags;
				new = old & ~(UAC_BITMASK << UAC_SHIFT);
				new = new | (w & UAC_BITMASK) << UAC_SHIFT;
				if (cmpxchg(&current_thread_info()->flags,
					    old, new) != old)
					goto again;
 				break;

 			default:
 				return -EOPNOTSUPP;
 			}
 		}
 		return 0;
	}

	default:
		break;
	}

	return -EOPNOTSUPP;
}

