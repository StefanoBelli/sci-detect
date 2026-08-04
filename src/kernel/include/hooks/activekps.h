#ifndef SCID_HOOKS_PPT_SHOW_NMISSED_H
#define SCID_HOOKS_PPT_SHOW_NMISSED_H

#ifdef KP_SHOW_NMISSED
#	define show_kp_nmissed(kp, name) \
		do { \
			if(unlikely((kp).nmissed > 0)) \
				scid_warnf("for kp: " name " nmissed=%ld", (kp).nmissed); \
		} while(0)
#else
#	define show_kp_nmissed(kp, name)
#endif

#ifndef KPS_MAXACTIVE
#	define KPS_MAXACTIVE 100
#endif

#endif
