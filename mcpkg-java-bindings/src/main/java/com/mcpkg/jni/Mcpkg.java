package com.mcpkg.jni;

public final class Mcpkg {
  static {
    // Expect libmcpkg_jni.so / mcpkg_jni.dll / libmcpkg_jni.dylib in java.library.path
    // or ship natives per-OS and call System.load(...) from a resource extractor.
    System.loadLibrary("mcpkg_jni");
  }

  private Mcpkg() {}

  // High-level operations wired to your C library:
  public static native int update(String mcVersion, String loader);
  public static native int install(String mcVersion, String loader, String[] packages);
  public static native int remove(String mcVersion, String loader, String[] packages);
  public static native String policy(String mcVersion, String loader, String[] packages);
  public static native int activate(String mcVersion, String loader);
  public static native int upgrade(String mcVersion, String loader);
}
