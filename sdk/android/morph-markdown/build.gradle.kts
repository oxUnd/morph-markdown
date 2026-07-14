plugins {
	id("com.android.library")
	id("org.jetbrains.kotlin.android")
}

android {
	namespace = "com.morph.markdown"
	compileSdk = 36

	defaultConfig {
		minSdk = 26

		ndk {
			abiFilters += listOf("arm64-v8a")
		}

		externalNativeBuild {
			cmake {
				arguments += listOf("-DANDROID_STL=c++_static")
			}
		}
	}

	externalNativeBuild {
		cmake {
			path = file("src/main/cpp/CMakeLists.txt")
		}
	}

	sourceSets {
		getByName("main") {
			assets.srcDir(projectDir.resolve("../../../vendor/mathjax-c/fonts"))
		}
	}

	compileOptions {
		sourceCompatibility = JavaVersion.VERSION_17
		targetCompatibility = JavaVersion.VERSION_17
	}

	kotlinOptions {
		jvmTarget = "17"
	}
}

dependencies {
	implementation("androidx.core:core-ktx:1.12.0")
	testImplementation("junit:junit:4.13.2")
}
