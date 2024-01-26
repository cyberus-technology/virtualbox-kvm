#include <stdint.h>
#include <stdio.h>

#include <blapi.h>


#if defined (__x86_64__) || \
    defined (__amd64__) || \
    defined (__ia64__) || \
    defined (__powerpc64__) || \
    defined (__s390x__) || \
    (defined (__sparc__) && defined(__arch64__)) || \
    defined(__aarch64__)

#define EXPECTED_LIB_FLATTENSIZE 248

#elif defined (__i386__) || \
    defined (__powerpc__) || \
    defined (__s390__) || \
    defined (__sparc__) || \
    defined (__arm__)

#define EXPECTED_LIB_FLATTENSIZE 160

#else

#error Undefined architecture type

#endif

int main(void)
{
    SHA1Context *context;
    uint32_t libFlattenSize;

    context = SHA1_NewContext();
    if (!context) {
        printf("Could not create SHA1 context.\n");
        return EXIT_FAILURE;
    }
    SHA1_Begin(context);

    libFlattenSize = SHA1_FlattenSize(context);
    if (libFlattenSize != EXPECTED_LIB_FLATTENSIZE) {
        printf("SHA1 flatten size is %d, expected %d\n",
               libFlattenSize,
               EXPECTED_LIB_FLATTENSIZE);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
