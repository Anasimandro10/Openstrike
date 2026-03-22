const std = @import("std");

pub fn build(b: *std.Build) void {
    const target   = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const exe = b.addExecutable(.{
        .name = "openstrike",
        .root_module = b.createModule(.{
            .root_source_file = b.path("src/main.zig"),
            .target           = target,
            .optimize         = optimize,
        }),
    });

    exe.linkLibC();

    const isWindows = target.result.os.tag == .windows;

    if (isWindows) {
        exe.addIncludePath(b.path("deps/windows/SDL2/x86_64-w64-mingw32/include"));
        exe.addLibraryPath(b.path("deps/windows/SDL2/x86_64-w64-mingw32/lib"));
        exe.linkSystemLibrary("SDL2");
        exe.linkSystemLibrary("opengl32");
    } else {
        exe.linkSystemLibrary("SDL2");
        exe.linkSystemLibrary("GL");
    }

    b.installArtifact(exe);
}
