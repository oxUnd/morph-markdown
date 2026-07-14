pluginManagement {
	repositories {
		google()
		mavenCentral()
		gradlePluginPortal()
	}
}

dependencyResolutionManagement {
	repositoriesMode.set(RepositoriesMode.FAIL_ON_PROJECT_REPOS)
	repositories {
		google()
		mavenCentral()
	}
}

rootProject.name = "morph-markdown-demo"
include(":app")
include(":morph-markdown")
project(":morph-markdown").projectDir =
	file("../../sdk/android/morph-markdown")
