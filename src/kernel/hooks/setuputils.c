#include <hooks/setuputils.h>
#include <logging.h>

/* types */
typedef void (*unregister_probes_cb)(void*, int);

typedef int (*register_probes_cb)(void*, int);

struct __register_probes_args {
	union {
		unregister_probes_cb unreg_cb;
		register_probes_cb reg_cb;
	};

	const char* probes_type;
	void* arr;
	int nr;
};

/*
 * this cannot fail in anyway,
 * @unregargs: unregistration arguments to check and pass to the unreg callback
 */
static void __unregister_probes(struct __register_probes_args *unregargs)
{
	if(unregargs->arr && unregargs->nr > 0) {
		scid_infof("unregistering %d %s", unregargs->nr, unregargs->probes_type);
		unregargs->unreg_cb(unregargs->arr, unregargs->nr);
	} else if(!unregargs->arr)
		scid_warnf("%s's array is NULL", unregargs->probes_type);
	else
		scid_infof("there are no %s to unregister :/ (they're 0)", unregargs->probes_type);
}

/*
 * this may fail if registration fails
 * @regargs: registration arguments to check and pass to the reg callback
 *
 * Returns: 0 if everything ok, not 0 otherwise
 */
static int __register_probes(struct __register_probes_args *regargs)
{
	if(regargs->arr && regargs->nr > 0) {
		scid_infof("registering %d %s", regargs->nr, regargs->probes_type);

		int rv = regargs->reg_cb(regargs->arr, regargs->nr);
		if(rv) {
			scid_errf("unable to register %d %s", regargs->nr, regargs->probes_type);
			return rv;
		}
	} else if(!regargs->arr)
		scid_warnf("%s's array is NULL", regargs->probes_type);
	else
		scid_infof("there are no %s to register :/ (they're 0)", regargs->probes_type);

	return 0;
}

#define DEFINE_REGISTER_KRETPROBES_ARGS(_name_, _krps_, _nr_krps_) \
	struct __register_probes_args _name_ = { \
		.reg_cb = (register_probes_cb) register_kretprobes, \
		.probes_type = "kretprobes", \
		.arr = (_krps_), \
		.nr = (_nr_krps_), \
	}

#define DEFINE_REGISTER_KPROBES_ARGS(_name_, _kps_, _nr_kps_) \
	struct __register_probes_args _name_ = { \
		.reg_cb = (register_probes_cb) register_kprobes, \
		.probes_type = "kprobes", \
		.arr = (_kps_), \
		.nr = (_nr_kps_), \
	}

#define DEFINE_UNREGISTER_KRETPROBES_ARGS(_name_, _krps_, _nr_krps_) \
	struct __register_probes_args _name_ = { \
		.unreg_cb = (unregister_probes_cb) unregister_kretprobes, \
		.probes_type = "kretprobes", \
		.arr = (_krps_), \
		.nr = (_nr_krps_), \
	}

#define DEFINE_UNREGISTER_KPROBES_ARGS(_name_, _kps_, _nr_kps_) \
	struct __register_probes_args _name_ = { \
		.unreg_cb = (unregister_probes_cb) unregister_kprobes, \
		.probes_type = "kprobes", \
		.arr = (_kps_), \
		.nr = (_nr_kps_), \
	}

/*
 * if failure in registering any type of probes, ensures that any previously
 * setup probes is unregistered before returning to caller
 *
 * @args: all probes to setup
 *
 * Returns: 0 if successful, not 0 otherwise
 */
int __base_setup_hooks(struct __base_setup_hooks_args *args) 
{
	int rv = 0;

	DEFINE_REGISTER_KRETPROBES_ARGS(regi_krps, args->krps, args->nr_krps);
	
	rv = __register_probes(&regi_krps);
	if(rv)
		return rv;

	DEFINE_REGISTER_KPROBES_ARGS(regi_kps, args->kps, args->nr_kps);
	
	rv = __register_probes(&regi_kps);
	if(rv) {
		regi_krps.unreg_cb = (unregister_probes_cb) unregister_kretprobes;
		scid_errf("unregistering already-registered %d %s", 
				regi_krps.nr, regi_krps.probes_type);
		__unregister_probes(&regi_krps);
		return rv;
	}

	return rv;
}

/*
 * Unregister probes, this cannot fail
 *
 * @args: all probes to teardown
 */
void __base_teardown_hooks(struct __base_setup_hooks_args *args) 
{
	DEFINE_UNREGISTER_KPROBES_ARGS(unregi_kps, args->kps, args->nr_kps);
	DEFINE_UNREGISTER_KRETPROBES_ARGS(unregi_krps, args->krps, args->nr_krps);

	__unregister_probes(&unregi_kps);
	__unregister_probes(&unregi_krps);
}

#undef DEFINE_REGISTER_KRETPROBES_ARGS
#undef DEFINE_REGISTER_KPROBES_ARGS
#undef DEFINE_UNREGISTER_KRETPROBES_ARGS
#undef DEFINE_UNREGISTER_KPROBES_ARGS
