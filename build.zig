const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const exe = b.addExecutable(.{
        .name = "openstrike",
        .root_module = b.createModule(.{
            .root_source_file = b.path("src/main.zig"),
            .target = target,
            .optimize = optimize,
        }),
    });

    exe.linkLibC();

    const is_windows = target.result.os.tag == .windows;

    if (is_windows) {
        exe.addIncludePath(b.path("deps/windows/SDL2/x86_64-w64-mingw32/include"));
        exe.addLibraryPath(b.path("deps/windows/SDL2/x86_64-w64-mingw32/lib"));
        exe.linkSystemLibrary("SDL2");
        exe.linkSystemLibrary("opengl32");
        exe.linkSystemLibrary("ws2_32");
        exe.linkSystemLibrary("winmm");
        exe.linkSystemLibrary("gdi32");
        exe.linkSystemLibrary("setupapi");
        exe.linkSystemLibrary("ole32");
        exe.linkSystemLibrary("oleaut32");
        exe.linkSystemLibrary("imm32");
        exe.linkSystemLibrary("version");
    } else {
        exe.linkSystemLibrary("SDL2");
        exe.linkSystemLibrary("GL");
    }

    // stb_image — compilar desde fuente C (vendored en csrc/)
    exe.addCSourceFiles(.{
        .files = &.{"csrc/stb_image.c"},
        .flags = &.{"-std=c99"},
    });
    exe.addIncludePath(b.path("csrc"));

    b.installArtifact(exe);
}
