// Android compatibility for prebuilt libraries expecting glibc interfaces.
// TODO: replace with Android-specific upstream dependency fixes.
#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <sys/types.h>
#include <unistd.h>

extern "C" int backtrace(void** frames, int size) {
    (void)frames;
    (void)size;
    return 0; // Android does not expose glibc's backtrace().
}

extern "C" ssize_t copy_file_range(int fd_in, off64_t* off_in,
                                    int fd_out, off64_t* off_out,
                                    size_t len, unsigned int flags) {
    if (flags != 0) { errno = EINVAL; return -1; }
    char buffer[65536];
    size_t total = 0;
    while (total < len) {
        size_t chunk = len - total < sizeof(buffer) ? len - total : sizeof(buffer);
        ssize_t n = off_in ? pread64(fd_in, buffer, chunk, *off_in) : read(fd_in, buffer, chunk);
        if (n == 0) break;
        if (n < 0) return total ? static_cast<ssize_t>(total) : -1;
        ssize_t done = 0;
        while (done < n) {
            ssize_t written = off_out
                ? pwrite64(fd_out, buffer + done, n - done, *off_out)
                : write(fd_out, buffer + done, n - done);
            if (written <= 0) {
                if (written == 0) errno = EIO;
                return total ? static_cast<ssize_t>(total) : -1;
            }
            done += written;
            total += written;
            if (off_out) *off_out += written;
        }
        if (off_in) *off_in += n;
    }
    return static_cast<ssize_t>(total);
}
