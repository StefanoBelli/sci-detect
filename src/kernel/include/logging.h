#ifndef SCID_LOGGING_H
#define SCID_LOGGING_H

#include <linux/printk.h>

#include <modname.h>

#define __expand__(y) #y
#define __to_s(x) __expand__(x)

#define __scid_log_format__(prfn, premsg, fmt, ...) \
	prfn("%s: " \
			premsg \
			" in %s," \
			" at " __FILE__ \
			"." __to_s(__LINE__) \
			": " fmt "\n", \
			MODNAME, \
			__func__, \
			__VA_ARGS__)

#define scid_infof(fmt, ...) \
	pr_info("%s: info: " fmt "\n", MODNAME, __VA_ARGS__) 

#define scid_warnf(fmt, ...) \
	__scid_log_format__(pr_warn, "warn", fmt, __VA_ARGS__)

#define scid_errf(fmt, ...) \
	__scid_log_format__(pr_err, "err", fmt, __VA_ARGS__)

#define __scid_log__(prfn, premsg, msg) \
	prfn("%s: " \
			premsg \
			" in %s," \
			" at " __FILE__ \
			"." __to_s(__LINE__) \
			": " msg "\n", \
			MODNAME, \
			__func__)

#define scid_info(msg) \
	pr_info("%s: info: " msg "\n", MODNAME) 

#define scid_warn(msg) \
	__scid_log__(pr_warn, "warn", msg)

#define scid_err(msg) \
	__scid_log__(pr_err, "err", msg)

#endif
