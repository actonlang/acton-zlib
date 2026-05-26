const std = @import("std");

const cflags = [_][]const u8{
    "-Wall",
    "-Wextra",
    "-Wpedantic",
    "-DHAVE_UNISTD_H=1",
};

const source_files = [_][]const u8{
    "adler32.c",
    "compress.c",
    "crc32.c",
    "deflate.c",
    "gzclose.c",
    "gzlib.c",
    "gzread.c",
    "gzwrite.c",
    "inflate.c",
    "infback.c",
    "inftrees.c",
    "inffast.c",
    "trees.c",
    "uncompr.c",
    "zutil.c",
};

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});
    const upstream = b.dependency("zlib_upstream", .{});

    const lib = b.addLibrary(.{
        .name = "z",
        .linkage = .static,
        .root_module = b.createModule(.{
            .target = target,
            .optimize = optimize,
            .link_libc = true,
        }),
    });

    const config_header = b.addConfigHeader(
        .{
            .style = .{ .cmake = upstream.path("zconf.h.cmakein") },
            .include_path = "zconf.h",
        },
        .{
            .HAVE_SYS_TYPES_H = true,
            .HAVE_STDINT_H = true,
            .HAVE_STDDEF_H = true,
            .HAVE_UNISTD_H = true,
            .Z_HAVE_UNISTD_H = true,
        },
    );
    lib.root_module.addConfigHeader(config_header);

    lib.root_module.addCSourceFiles(.{
        .root = upstream.path("."),
        .files = &source_files,
        .flags = &cflags,
    });
    lib.root_module.addIncludePath(upstream.path("."));

    lib.installHeader(upstream.path("zlib.h"), "zlib.h");
    lib.installHeader(config_header.getOutputFile(), "zconf.h");

    b.installArtifact(lib);
}
