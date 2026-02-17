import os

source_path = os.path.join('src', 'gnubg', 'gnubg.c')
dest_path = os.path.join('src', 'gnubgmodule', 'gnubg_lib.c')

try:
    with open(source_path, 'r', encoding='utf-8') as f:
        content = f.read()

    # 1. Rename main() to prevent entry-point conflicts
    new_content = content.replace('int\nmain(int argc, char *argv[])',
                                  'int\ngnubg_lib_unused_main(int argc, char *argv[])')
    if new_content == content:
        new_content = content.replace('int main(int argc, char *argv[])',
                                      'int gnubg_lib_unused_main(int argc, char *argv[])')

    # 2. Inject Python.h and the actual memory definition for 'td'
    # We place this right at the top so it's available to everything.
    injection = """
#include "config.h"
#ifdef USE_PYTHON
#include <Python.h>
#endif
#include "multithread.h"

/* AUTHORITATIVE DEFINITION: Allocate memory for the global thread data */
/* This prevents the 'undefined symbol: td' runtime error. */
ThreadData td; 

"""
    # Replace the first config.h include with our injected block
    if '#include "config.h"' in new_content:
        new_content = new_content.replace('#include "config.h"', injection, 1)
    else:
        new_content = injection + new_content

    with open(dest_path, 'w', encoding='utf-8') as f:
        f.write(new_content)

    print(f"Successfully patched {dest_path}")
    print("- Renamed main()")
    print("- Injected <Python.h> to fix version error")
    print("- Defined 'ThreadData td' to fix undefined symbol error")

except Exception as e:
    print(f"Error: {e}")