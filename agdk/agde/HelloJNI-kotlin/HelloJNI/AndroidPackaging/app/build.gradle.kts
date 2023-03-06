plugins {
    id("com.android.application")
    kotlin("android")
    kotlin("android.extensions")
}

android {
    compileSdkVersion(31)
    buildToolsVersion("30.0.3")
    ndkVersion = rootProject.extra.get("MSBUILD_NDK_VERSION").toString()
    defaultConfig {
        applicationId = "com.example.hellojni"
        minSdkVersion(rootProject.extra.get("MSBUILD_MIN_SDK_VERSION").toString()) 
        targetSdkVersion(31)
        versionCode = 1
        versionName = "1.0"
        testInstrumentationRunner = "androidx.test.runner.AndroidJUnitRunner"
    }
    buildTypes {
        getByName("release")  {
            isMinifyEnabled = false
            proguardFiles(getDefaultProguardFile("proguard-android.txt"), "proguard-rules.pro")
        }
    }
    sourceSets["main"].jniLibs.srcDir(rootProject.extra.get("MSBUILD_JNI_LIBS_SRC_DIR").toString())

    applicationVariants.all {
        outputs
            .map { it as com.android.build.gradle.internal.api.BaseVariantOutputImpl }
            .forEach { output ->
                output.outputFileName = rootProject.extra.get("MSBUILD_ANDROID_OUTPUT_APK_NAME").toString()
            }
    }
}

dependencies {
    implementation(fileTree(mapOf("dir" to "libs", "include" to listOf("*.jar"))))
    implementation("androidx.appcompat:appcompat:1.0.2")
    implementation("androidx.constraintlayout:constraintlayout:1.1.3")
    testImplementation("junit:junit:4.12")
    androidTestImplementation("androidx.test.ext:junit:1.1.1")
    androidTestImplementation("androidx.test.espresso:espresso-core:3.2.0")
}

buildDir=File(rootProject.extra.get("MSBUILD_ANDROID_GRADLE_BUILD_OUTPUT_DIR").toString())
if (!File(rootProject.extra.get("MSBUILD_JNI_LIBS_SRC_DIR")!!.toString()).isDirectory) {
    error(rootProject.extra.get("MSBUILD_JNI_LIBS_SRC_DIR")!!.toString())
}
