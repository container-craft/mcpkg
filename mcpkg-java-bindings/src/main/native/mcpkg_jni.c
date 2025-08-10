#include <jni.h>
#include <stdlib.h>
#include <string.h>

#include <mcpkg.h>
#include <mcpkg_cache.h>
#include <api/mcpkg_api_client.h>
#include <api/mcpkg_entry.h>
#include <api/mcpkg_deps_entry.h>
#include <api/modrith_client.h>
#include <utils/array_helper.h>
#include <utils/mcpkg_fs.h>
#include <mcpkg_get.h>
#include <mcpkg_config.h>
#include <mcpkg_activate.h>

static str_array* jstring_array_to_str_array(JNIEnv *env, jobjectArray jarr) {
    if (!jarr) return NULL;
    jsize n = (*env)->GetArrayLength(env, jarr);
    str_array *arr = str_array_new();
    if (!arr) return NULL;
    for (jsize i = 0; i < n; ++i) {
        jstring jstr = (jstring)(*env)->GetObjectArrayElement(env, jarr, i);
        if (!jstr) continue;
        const char *cstr = (*env)->GetStringUTFChars(env, jstr, 0);
        if (cstr) (void)str_array_add(arr, cstr);
        (*env)->ReleaseStringUTFChars(env, jstr, cstr);
        (*env)->DeleteLocalRef(env, jstr);
    }
    return arr;
}

JNIEXPORT jint JNICALL
Java_com_mcpkg_jni_Mcpkg_update(JNIEnv *env, jclass cls, jstring jver, jstring jloader) {
    (void)env; (void)cls;
    const char *ver = (*env)->GetStringUTFChars(env, jver, 0);
    const char *ldr = (*env)->GetStringUTFChars(env, jloader, 0);

    // Use your existing CLI-equivalent function
    ModrithApiClient *client = modrith_client_new(ver, mcpkg_modloader_from_str(ldr));
    int rc = (client ? modrith_client_update(client) : MCPKG_ERROR_OOM);
    modrith_client_free(client);

    (*env)->ReleaseStringUTFChars(env, jver, ver);
    (*env)->ReleaseStringUTFChars(env, jloader, ldr);
    return rc;
}

JNIEXPORT jint JNICALL
Java_com_mcpkg_jni_Mcpkg_install(JNIEnv *env, jclass cls, jstring jver, jstring jloader, jobjectArray jpkgs) {
    (void)cls;
    const char *ver = (*env)->GetStringUTFChars(env, jver, 0);
    const char *ldr = (*env)->GetStringUTFChars(env, jloader, 0);
    str_array *arr = jstring_array_to_str_array(env, jpkgs);

    int rc = mcpkg_get_install(ver, ldr, arr);

    str_array_free(arr);
    (*env)->ReleaseStringUTFChars(env, jver, ver);
    (*env)->ReleaseStringUTFChars(env, jloader, ldr);
    return rc;
}

JNIEXPORT jint JNICALL
Java_com_mcpkg_jni_Mcpkg_remove(JNIEnv *env, jclass cls, jstring jver, jstring jloader, jobjectArray jpkgs) {
    (void)cls;
    const char *ver = (*env)->GetStringUTFChars(env, jver, 0);
    const char *ldr = (*env)->GetStringUTFChars(env, jloader, 0);
    str_array *arr = jstring_array_to_str_array(env, jpkgs);

    int rc = mcpkg_get_remove(ver, ldr, arr);

    str_array_free(arr);
    (*env)->ReleaseStringUTFChars(env, jver, ver);
    (*env)->ReleaseStringUTFChars(env, jloader, ldr);
    return rc;
}

JNIEXPORT jstring JNICALL
Java_com_mcpkg_jni_Mcpkg_policy(JNIEnv *env, jclass cls, jstring jver, jstring jloader, jobjectArray jpkgs) {
    (void)cls;
    const char *ver = (*env)->GetStringUTFChars(env, jver, 0);
    const char *ldr = (*env)->GetStringUTFChars(env, jloader, 0);
    str_array *arr = jstring_array_to_str_array(env, jpkgs);

    char *report = mcpkg_get_policy(ver, ldr, arr);

    jstring out = (*env)->NewStringUTF(env, report ? report : "");
    if (report) free(report);
    str_array_free(arr);
    (*env)->ReleaseStringUTFChars(env, jver, ver);
    (*env)->ReleaseStringUTFChars(env, jloader, ldr);
    return out;
}

JNIEXPORT jint JNICALL
Java_com_mcpkg_jni_Mcpkg_activate(JNIEnv *env, jclass cls, jstring jver, jstring jloader) {
    (void)cls;
    const char *ver = (*env)->GetStringUTFChars(env, jver, 0);
    const char *ldr = (*env)->GetStringUTFChars(env, jloader, 0);
    int rc = mcpkg_activate(ver, ldr);  // your activate helper (symlink/copy)
    (*env)->ReleaseStringUTFChars(env, jver, ver);
    (*env)->ReleaseStringUTFChars(env, jloader, ldr);
    return rc;
}

JNIEXPORT jint JNICALL
Java_com_mcpkg_jni_Mcpkg_upgrade(JNIEnv *env, jclass cls, jstring jver, jstring jloader) {
    (void)cls;
    const char *ver = (*env)->GetStringUTFChars(env, jver, 0);
    const char *ldr = (*env)->GetStringUTFChars(env, jloader, 0);
    int rc = mcpkg_get_upgreade(ver, ldr);
    (*env)->ReleaseStringUTFChars(env, jver, ver);
    (*env)->ReleaseStringUTFChars(env, jloader, ldr);
    return rc;
}
