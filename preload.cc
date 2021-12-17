/*
 * Wrapper for __libc_start_main() that replaces the real main
 * function with our hooked version.
 */
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <elfio/elfio.hpp>
#include <link.h>

using namespace ELFIO;

/**
 * This is needed to provide the function unmanagled.
 * 
 */
extern "C" int __libc_start_main(int (*main)(int, char **, char **), int argc,
			  char **ubp_av, void (*init)(void), void (*fini)(void),
			  void (*rtld_fini)(void), void(*stack_end));

static int (*real_main) (int, char **, char **);
static void *libc_handle;

static void die(const char *failed)
{
	fprintf(stderr, "preload: %s\n", failed);
	exit(1);
}

static std::string get_selfpath() {
    char buff[PATH_MAX];
    ssize_t len = ::readlink("/proc/self/exe", buff, sizeof(buff)-1);
    if (len != -1) {
      buff[len] = '\0';
      return std::string(buff);
    }
    die("could not read /proc/self/exe");
}

/** @brief Fake main(), spliced in before the real call to main() by
 *         __libc_start_main (see below).
 *  We get serialized commands from our invoking process over an fd specified
 *  by an environment variable (kFdEnvVar). The environment variable is a list
 *  of key=value pairs (see move_commands_to_env); we use them to construct a
 *  jail, then enter it.
 */
static int fake_main(int argc, char **argv, char **envp)
{
    std::string path = get_selfpath();
    elfio reader;
     if ( !reader.load( path ) ) {
        die("Should never happen");
    }

    section* dynamic_section = reader.sections[".dynamic"];
    if (!dynamic_section) {
        return 0;
    }

    ELFIO::dynamic_section_accessor dynamic(reader, dynamic_section);
    Elf_Xword dyn_no = dynamic.get_entries_num();
    if ( dyn_no <= 0 ) {
        return 0;
    }
    for ( int i = 0; i < dyn_no; ++i ) {
        ELFIO::Elf_Xword tag   = 0;
        ELFIO::Elf_Xword value = 0;
        std::string str;
        dynamic.get_entry(i, tag, value, str);
        if (tag == DT_NEEDED) {
            
            // Comment in for debug:
            // std::cout << str << std::endl;

            void *handle = dlopen(str.c_str(), RTLD_NOW);
            if (!handle) {
                die("Could not find library");
            }

            const struct link_map * map;
            if (dlinfo(handle, RTLD_DI_LINKMAP, &map) != 0) {
                die("Could not dlinfo");
            }
            printf("%s\n", map->l_name);
        }
    }
	return 0;
}


/** @brief LD_PRELOAD override of __libc_start_main.
 *
 *  It is really best if you do not look too closely at this function.  We need
 *  to ensure that some of our code runs before the target program (see the
 *  minijail0.1 file in this directory for high-level details about this), and
 *  the only available place to hook is this function, which is normally
 *  responsible for calling main(). Our LD_PRELOAD will overwrite the real
 *  __libc_start_main with this one, so we have to look up the real one from
 *  libc and invoke it with a pointer to the fake main() we'd like to run before
 *  the real main(). We can't just run our setup code *here* because
 *  __libc_start_main is responsible for setting up the C runtime environment,
 *  so we can't rely on things like malloc() being available yet.
 */

int __libc_start_main(int (*main)(int, char **, char **), int argc,
			  char **ubp_av, void (*init)(void), void (*fini)(void),
			  void (*rtld_fini)(void), void(*stack_end))
{
    void *sym;
	/*
	 * This hack is unfortunately required by C99 - casting directly from
	 * void* to function pointers is left undefined. See POSIX.1-2003, the
	 * Rationale for the specification of dlsym(), and dlsym(3). This
	 * deliberately violates strict-aliasing rules, but gcc can't tell.
	 */
	union {
		int (*fn)(int (*main)(int, char **, char **), int argc,
			  char **ubp_av, void (*init)(void), void (*fini)(void),
			  void (*rtld_fini)(void), void(*stack_end));
		void *symval;
	} real_libc_start_main;

	/*
	 * We hold this handle for the duration of the real __libc_start_main()
	 * and drop it just before calling the real main().
	 */
	libc_handle = dlopen("libc.so.6", RTLD_NOW);

	if (!libc_handle) {
		fprintf(stderr, "can't dlopen() libc\n");
		/*
		 * We dare not use abort() here because it will run atexit()
		 * handlers and try to flush stdio.
		 */
		_exit(2);
	}
	sym = dlsym(libc_handle, "__libc_start_main");
	if (!sym) {
		fprintf(stderr, "can't find the real __libc_start_main()\n");
		_exit(3);
	}
	real_libc_start_main.symval = sym;
	real_main = main;

	/*
	 * Note that we swap fake_main in for main - fake_main knows that it
	 * should call real_main after it's done.
	 */
	return real_libc_start_main.fn(fake_main, argc, ubp_av, init, fini,
				       rtld_fini, stack_end);
}