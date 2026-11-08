plugins {
    alias(libs.plugins.android.application)
}

android {
    namespace = "com.openflight"
    compileSdk {
        version = release(37)
    }

    defaultConfig {
        applicationId = "com.openflight"
        minSdk = 26
        targetSdk = 37
        versionCode = 13
        versionName = "0.13"

        externalNativeBuild {
            cmake {
                cppFlags += "-std=c++17"
            }
        }
    }

    buildTypes {
        release {
            isMinifyEnabled = false
        }
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_11
        targetCompatibility = JavaVersion.VERSION_11
    }

    externalNativeBuild {
        cmake {
            path = file("src/main/cpp/CMakeLists.txt")
            version = "3.22.1"
        }
    }
}