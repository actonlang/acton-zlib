#include <zlib.h>
#include <stdlib.h>
#include <string.h>

void zlibQ_libQ___ext_init__() {}

B_bytes zlibQ_libQ_compress(B_bytes data) {
    if (data->nbytes == 0) {
        return data;
    }

    // Prepare the zlib stream
    int ret;
    z_stream stream;
    memset(&stream, 0, sizeof(stream));
    ret = deflateInit(&stream, Z_DEFAULT_COMPRESSION);
    if (ret != Z_OK) {
        $RAISE((B_BaseException)$NEW(B_ValueError, $FORMAT("Unable to compress data, init error: %d", ret)));
    }

    // Set the input data
    stream.avail_in = data->nbytes;
    stream.next_in = (Bytef*)data->str;

    // Allocate the output buffer using Acton's malloc
    size_t output_size = deflateBound(&stream, data->nbytes);
    Bytef* output_buffer = (Bytef*)acton_malloc_atomic(output_size);
    stream.avail_out = output_size;
    stream.next_out = output_buffer;

    // Perform the deflate operation
    ret = deflate(&stream, Z_FINISH);
    size_t output_len = stream.total_out;
    deflateEnd(&stream);
    if (ret != Z_STREAM_END) {
        $RAISE((B_BaseException)$NEW(B_ValueError, $FORMAT("Unable to compress data, error: %d", ret)));
    }

    // The deflate stream is binary and contains NUL bytes
    return actBytesFromCStringLengthNoCopy((char *)output_buffer, output_len);
}

B_bytes zlibQ_libQ_decompress(B_bytes data) {
    if (data->nbytes == 0) {
        return data;
    }

    // Prepare the zlib stream
    int ret;
    z_stream stream;
    memset(&stream, 0, sizeof(stream));

    ret = inflateInit(&stream);
    if (ret != Z_OK) {
        $RAISE((B_BaseException)$NEW(B_ValueError, $FORMAT("Unable to decompress data, init error: %d", ret)));
    }

    // Set the input data
    stream.avail_in = data->nbytes;
    stream.next_in = (Bytef*)data->str;

    // Allocate the output buffer using Acton's malloc
    size_t output_size = 2 * data->nbytes; // Initial output buffer size
    Bytef* output_buffer = (Bytef*)acton_malloc_atomic(output_size);
    stream.avail_out = output_size;
    stream.next_out = output_buffer;

    // Inflate until the end of the stream, doubling the output buffer each
    // time it fills up. Z_BUF_ERROR with room left in the output buffer means
    // the input ended before the stream did.
    do {
        if (stream.avail_out == 0) {
            output_size *= 2;
            output_buffer = (Bytef*)acton_realloc(output_buffer, output_size);
            stream.next_out = output_buffer + stream.total_out;
            stream.avail_out = output_size - stream.total_out;
        }
        ret = inflate(&stream, Z_NO_FLUSH);
    } while (ret == Z_OK);
    size_t output_len = stream.total_out;
    inflateEnd(&stream);
    if (ret != Z_STREAM_END) {
        $RAISE((B_BaseException)$NEW(B_ValueError, $FORMAT("Unable to decompress data, error: %d", ret)));
    }

    return actBytesFromCStringLengthNoCopy((char *)output_buffer, output_len);
}
