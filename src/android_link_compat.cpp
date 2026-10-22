// Compatibility for third-party static libraries linked against Android API 29.
// Some dependencies reference glibc-style symbols unavailable in Android's bionic.
#ifdef __ANDROID__
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <sys/types.h>
#include <unistd.h>

extern "C" {
// Abseil's generic stack trace implementation may reference backtrace().
// Android API 29 does not export the glibc backtrace symbol. Returning zero
// frames disables optional stack traces without breaking normal operation.
int backtrace(void** /*buffer*/, int /*size*/) { return 0; }

// Zix uses copy_file_range() for copying files. Implement it with pread/pwrite
// on Android, where the glibc symbol is not exported at our minimum API.
ssize_t copy_file_range(int fd_in, off64_t* off_in, int fd_out,
                        off64_t* off_out, size_t len, unsigned int flags) {
    if (flags != 0) { errno = EINVAL; return -1; }
    char buffer[65536];
    size_t total = 0;
    while (total < len) {
        const size_t chunk = (len - total < sizeof(buffer)) ? len - total : sizeof(buffer);
        ssize_t n = off_in ? pread64(fd_in, buffer, chunk, *off_in) : read(fd_in, buffer, chunk);
        if (n < 0) return total ? static_cast<ssize_t>(total) : -1;
        if (n == 0) break;
        size_t written = 0;
        while (written < static_cast<size_t>(n)) {
            ssize_t w = off_out ? pwrite64(fd_out, buffer + written, n - written, *off_out)
                                : write(fd_out, buffer + written, n - written);
            if (w <= 0) { if (w == 0) errno = EIO; return total ? static_cast<ssize_t>(total) : -1; }
            written += static_cast<size_t>(w);
            if (off_out) *off_out += w;
        }
        total += written;
        if (off_in) *off_in += n;
    }
    return static_cast<ssize_t>(total);
}
}
#endif
