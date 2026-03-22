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
        // Librerías del sistema de Windows que SDL2 necesita internamente
        exe.linkSystemLibrary("gdi32");      // GDI — ventanas, píxeles, fuentes
        exe.linkSystemLibrary("winmm");      // Multimedia timer
        exe.linkSystemLibrary("ole32");      // COM — CoInitialize, CoCreateInstance
        exe.linkSystemLibrary("oleaut32");   // OLE Automation — SysFreeString
        exe.linkSystemLibrary("imm32");      // Input Method Manager — IME
        exe.linkSystemLibrary("setupapi");   // HID/joystick device enumeration
        exe.linkSystemLibrary("version");    // GetFileVersionInfo — IME detection
        exe.linkSystemLibrary("cfgmgr32");   // CM_Locate_DevNode — gamepad support
    } else {
        exe.linkSystemLibrary("SDL2");
        exe.linkSystemLibrary("GL");
    }

    b.installArtifact(exe);
}
