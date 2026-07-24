plugins {
	id("com.android.library")
}

val repoDir = projectDir.resolve("../../..").normalize()
val mathJaxFontsDir = repoDir.resolve(".third_party/mathjax-c/fonts")
val androidVendorPrefix = projectDir.resolve(".build/vendor-android/arm64-v8a").normalize()
check(mathJaxFontsDir.resolve("STIXTwoMath-Regular.ttf").isFile) {
	"Missing MathJax font asset: ${mathJaxFontsDir.resolve("STIXTwoMath-Regular.ttf")}. " +
		"Run scripts/prepare-third-party.sh from the repository root."
}

android {
	namespace = "com.morph.markdown"
	compileSdk = 36
	ndkVersion = "25.2.9519653"

	defaultConfig {
		minSdk = 26

		ndk {
			abiFilters += listOf("arm64-v8a")
		}

		externalNativeBuild {
			cmake {
				arguments += listOf(
					"-DANDROID_STL=c++_static",
					"-DMORPH_MATHJAX_ANDROID_PREFIX=${androidVendorPrefix.absolutePath}"
				)
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
			assets.directories.add(mathJaxFontsDir.absolutePath)
		}
	}

	compileOptions {
		sourceCompatibility = JavaVersion.VERSION_17
		targetCompatibility = JavaVersion.VERSION_17
	}
}

dependencies {
	implementation("androidx.core:core-ktx:1.12.0")
	testImplementation("junit:junit:4.13.2")
}
