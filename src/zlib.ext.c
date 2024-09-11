#include <zlib.h>
#include <stdlib.h>
#include <string.h>

void zlibQ___ext_init__() {}

B_bytes zlibQ_compress(B_bytes data) {
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
    if (ret != Z_STREAM_END) {
        $RAISE((B_BaseException)$NEW(B_ValueError, $FORMAT("Unable to compress data, error: %d", ret)));
    }

    // Clean up
    deflateEnd(&stream);

    return actBytesFromCStringNoCopy(output_buffer);
}

B_bytes zlibQ_decompress(B_bytes data) {
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
    memset(output_buffer, 0, output_size);
    stream.avail_out = output_size;
    stream.next_out = output_buffer;

    // Perform the inflate operation, increasing the output buffer size if needed
    do {
        ret = inflate(&stream, Z_NO_FLUSH);
        if (ret == Z_BUF_ERROR) {
            // Increase the output buffer size and continue decompressing
            size_t new_output_size = output_size * 2;
            output_buffer = (Bytef*)acton_realloc(output_buffer, new_output_size);
            stream.avail_out = new_output_size - stream.total_out;
            stream.next_out = output_buffer + stream.total_out;
        } else if (ret != Z_OK) {
            $RAISE((B_BaseException)$NEW(B_ValueError, $FORMAT("Unable to decompress data, error: %d", ret)));
        }
    } while (ret == Z_BUF_ERROR);

    // Clean up
    inflateEnd(&stream);

    return actBytesFromCStringNoCopy(output_buffer);
}
