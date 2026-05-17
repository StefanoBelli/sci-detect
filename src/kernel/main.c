#include <linux/kernel.h>
#include <linux/module.h>

MODULE_AUTHOR("Stefano Belli");
MODULE_DESCRIPTION("Stealth code injection detector");
MODULE_LICENSE("GPL");

int module_hello(void);
void module_byebye(void);

int module_hello(void) {
	return 0;
}

void module_byebye(void) {

}

module_init(module_hello);
module_exit(module_byebye);
