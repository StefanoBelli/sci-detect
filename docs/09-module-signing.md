# Module signing

When secure boot is enabled, the kernel **must** check signature before the
module is loaded (even though, this **may** not be the only situation in which the 
kernel enforces module signature check)
, that is, no unsigned code is allowed to run (maintaining
some kind of *"chain of trust"* from the early stages of bootloading).

Therefore, instead of disabling secure boot or deactivating ```module.sig_enforce```, just **sign the module**!

### Keyring sources

During boot and whether secure boot is enabled or not, the kernel builds
its own keyring that uses at runtime to check signatures against. It also
has some kind of revocation list (```.blacklist```).

**How the keyring is built**

 * From the kernel built-in keys, at compile-time (```.builtin_trusted_keys``` keyring)
 * From the **machine owner keys (MOK)** (```.platform``` keyring) if **secure boot is enabled**
 * From UEFI's ```db``` (those go into ```.platform``` keyring) and ```dbx``` (this last one, for revoked keys)

The ITU X.509 standard is widely used.

A very good explaination: https://docs.redhat.com/en/documentation/red_hat_enterprise_linux/8/html/managing_monitoring_and_updating_the_kernel/signing-a-kernel-and-modules-for-secure-boot_managing-monitoring-and-updating-the-kernel

### Signing

Our objective is to:
 * generate an asymmetric keypair (public/private)
 * store the pubkey in the machine owner keys (MOK)
 * reboot the system to successfully enroll the key in MOK db
    * the kernel now sees and trusts our pubkey (loaded in ```.platform```)
 * sign the module using our private key
 * **KEEP THE PRIVATE KEY SAFE**
 * using insmod, kernel signature verification shall now pass.

Generating signing keys: https://www.kernel.org/doc/html/v6.1/admin-guide/module-signing.html#generating-signing-keys

Manually signing modules: https://www.kernel.org/doc/html/v6.1/admin-guide/module-signing.html#manually-signing-modules
