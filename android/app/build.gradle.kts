plugins { id("com.android.application") }

android {
    namespace = "com.eightcee.mk64"
    compileSdk = 35

    defaultConfig {
        applicationId = "com.eightcee.mk64"
        minSdk = 26
        targetSdk = 35
        versionCode = 1
        versionName = "0.1.0"
        ndk { abiFilters += listOf("arm64-v8a") }
        externalNativeBuild {
            cmake { cppFlags += listOf("-std=c++17") }
        }
    }

    externalNativeBuild {
        cmake { path = file("src/main/cpp/CMakeLists.txt") }
    }
}
