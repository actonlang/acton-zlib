#include <limits.h>
#include <stdbool.h>
#include <zlib.h>

static struct B_ValueError zlibQ_libQ_output_too_long_error =
    STATIC_EXCEPTION(B_ValueError, "Zlib output is too long");
static struct B_MemoryError zlibQ_libQ_allocation_failed_error =
    STATIC_EXCEPTION(B_MemoryError, "Unable to allocate zlib output");

void zlibQ_libQ___ext_init__() {}

static B_bytes zlib_transform(B_bytes data, bool compressing) {
    if (data->nbytes == 0)
        return data;

    const char *operation = compressing ? "compress" : "decompress";
    z_stream stream = {0};
    int ret = compressing ? deflateInit(&stream, Z_DEFAULT_COMPRESSION) : inflateInit(&stream);
    if (ret != Z_OK)
        RAISE(B_ValueError, $FORMAT("Unable to %s data, init error: %d", operation, ret));

    stream.next_in = data->str;
    int64_t input_left = data->nbytes;
    int64_t capacity = (input_left < 65536 ? input_left : 65536) + 16;
    Bytef *output = acton_malloc_atomic(capacity);
    int64_t written = 0;
    const int64_t output_limit = MAX_STR_LEN < SIZE_MAX ? MAX_STR_LEN : SIZE_MAX - 1;

    // avail_in and avail_out are uInt even on 64-bit systems. On Windows,
    // total_out is also 32-bit, so count the bytes written on each call.
    while (output != NULL) {
        if (stream.avail_in == 0) {
            stream.avail_in = input_left > UINT_MAX ? UINT_MAX : (uInt)input_left;
            input_left -= stream.avail_in;
        }
        int64_t space = capacity - written;
        uInt chunk = space > UINT_MAX ? UINT_MAX : (uInt)space;
        stream.next_out = output + written;
        stream.avail_out = chunk;

        if (compressing)
            ret = deflate(&stream, input_left == 0 ? Z_FINISH : Z_NO_FLUSH);
        else
            ret = inflate(&stream, Z_NO_FLUSH);
        written += chunk - stream.avail_out;
        if (ret != Z_OK || written > output_limit)
            break;
        if (written == capacity) {
            // Allow one extra byte so zlib can finish a stream whose output
            // fits the limit exactly, or show that its output is too long.
            capacity = capacity > output_limit / 2 ? output_limit + 1 : capacity * 2;
            output = acton_realloc(output, capacity);
        }
    }

    if (compressing)
        deflateEnd(&stream);
    else
        inflateEnd(&stream);

    if (output == NULL)
        RAISE_EXC(&zlibQ_libQ_allocation_failed_error);
    if (written > output_limit)
        RAISE_EXC(&zlibQ_libQ_output_too_long_error);
    if (ret != Z_STREAM_END)
        RAISE(B_ValueError, $FORMAT("Unable to %s data, error: %d", operation, ret));
    return actBytesFromCStringLength((char *)output, written);
}

B_bytes zlibQ_libQ_compress(B_bytes data) {
    return zlib_transform(data, true);
}

B_bytes zlibQ_libQ_decompress(B_bytes data) {
    return zlib_transform(data, false);
}
