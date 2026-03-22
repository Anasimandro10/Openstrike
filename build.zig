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
        exe.linkSystemLibrary("ws2_32");   // ENet — sockets UDP
        exe.linkSystemLibrary("winmm");    // ENet — multimedia timer
        exe.linkSystemLibrary("gdi32");    // SDL2 — GDI (ventana, fuentes)
        exe.linkSystemLibrary("setupapi"); // SDL2 — deteccion de joystick/gamepad
        exe.linkSystemLibrary("ole32");    // SDL2 — COM (audio, input)
        exe.linkSystemLibrary("oleaut32"); // SDL2 — COM automation
        exe.linkSystemLibrary("imm32");    // SDL2 — input de texto (IME)
        exe.linkSystemLibrary("version");  // SDL2 — info de version del SO
    } else {
        exe.linkSystemLibrary("SDL2");
        exe.linkSystemLibrary("GL");
    }

    b.installArtifact(exe);
}
