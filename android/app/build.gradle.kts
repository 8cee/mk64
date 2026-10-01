plugins { id("com.android.application"); id("org.jetbrains.kotlin.android")     kotlinOptions {
        jvmTarget = "17"
    }
}

android {
    namespace = "com.eightcee.mk64"
    compileSdk = 35
    ndkVersion = "27.2.12479018"

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }

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
