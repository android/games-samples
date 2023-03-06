// Top-level build file where you can add configuration options common to all sub-projects/modules.
apply(from = ".agde/agde.gradle.kts")

buildscript {
    repositories {
       google()
       mavenCentral()
    }
    dependencies {
        classpath("com.android.tools.build:gradle:7.3.1")
        classpath(kotlin("gradle-plugin", version = "1.6.20"))
    }
}

allprojects {
    repositories {
        google()
        mavenCentral()
    }
}

tasks.register("clean", Delete::class) {
    delete(rootProject.buildDir)
}

