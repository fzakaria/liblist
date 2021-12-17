# ldlist

Print out the paths of the resolved binary as dictated **by the linker**.

This differentiates itself from other tools that replicate the linker process by walking _RUNPATH_ or _RPATH_ and may do it wrong. Using **LD_PRELOAD** hijack the binary after the linking process and discover where they were loaded from.

```bash
LD_PRELOAD=$PWD/libpreload.so ruby
/nix/store/ia70ss13m22znbl8khrf2hq72qmh5drr-ruby-2.7.5/lib/libruby-2.7.5.so.2.7
/nix/store/2zz2l3j07h9bkvdmxhxinj2rcjy3cdh7-zlib-1.2.11/lib/libz.so.1
/nix/store/563528481rvhc5kxwipjmg6rqrl95mdx-glibc-2.33-56/lib/libpthread.so.0
/nix/store/563528481rvhc5kxwipjmg6rqrl95mdx-glibc-2.33-56/lib/librt.so.1
/nix/store/563528481rvhc5kxwipjmg6rqrl95mdx-glibc-2.33-56/lib/libdl.so.2
/nix/store/563528481rvhc5kxwipjmg6rqrl95mdx-glibc-2.33-56/lib/libcrypt.so.1
/nix/store/563528481rvhc5kxwipjmg6rqrl95mdx-glibc-2.33-56/lib/libm.so.6
/nix/store/563528481rvhc5kxwipjmg6rqrl95mdx-glibc-2.33-56/lib/libc.so.6
```

or

```bash
❯ LD_PRELOAD=$PWD/libpreload.so python
/lib/x86_64-linux-gnu/libdl.so.2
/lib/x86_64-linux-gnu/libutil.so.1
/lib/x86_64-linux-gnu/libz.so.1
/lib/x86_64-linux-gnu/libm.so.6
/lib/x86_64-linux-gnu/libpthread.so.0
/lib/x86_64-linux-gnu/libc.so.6
```

## TODO

If you build this binary in a Nix context then it fails to locate the correct GLIBC. If you rebuild it either in the nix-shell context it then works for within nix.

```bash
❯ LD_PRELOAD=$PWD/libpreload.so ruby
ruby: /lib/x86_64-linux-gnu/libc.so.6: version `GLIBC_2.33' not found (required by /nix/store/2nkjrh3za68vrw6kf8lxn6nq1dval05v-gcc-10.3.0-lib/lib/libstdc++.so.6)
```