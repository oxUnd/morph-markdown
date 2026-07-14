plugins {
	id("com.android.application")
	id("org.jetbrains.kotlin.android")
}

android {
	namespace = "com.morph.markdown.demo"
	compileSdk = 36

	defaultConfig {
		applicationId = "com.morph.markdown.demo"
		minSdk = 26
		targetSdk = 36
		versionCode = 1
		versionName = "0.1.0"

		ndk {
			abiFilters += listOf("arm64-v8a")
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
	implementation(project(":morph-markdown"))
	implementation("androidx.core:core-ktx:1.12.0")
	implementation("androidx.appcompat:appcompat:1.6.1")
	implementation("androidx.activity:activity-ktx:1.8.2")
}
