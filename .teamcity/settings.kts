import jetbrains.buildServer.configs.kotlin.*
import jetbrains.buildServer.configs.kotlin.buildSteps.script
import jetbrains.buildServer.configs.kotlin.triggers.vcs
import jetbrains.buildServer.configs.kotlin.vcs.GitVcsRoot

/*
The settings script is an entry point for defining a TeamCity
project hierarchy. The script should contain a single call to the
project() function with a Project instance or an init function as
an argument.

VcsRoots, BuildTypes, Templates, and subprojects can be
registered inside the project using the vcsRoot(), buildType(),
template(), and subProject() methods respectively.

To debug settings scripts in command-line, run the

    mvnDebug org.jetbrains.teamcity:teamcity-configs-maven-plugin:generate

command and attach your debugger to the port 8000.

To debug in IntelliJ Idea, open the 'Maven Projects' tool window (View
-> Tool Windows -> Maven Projects), find the generate task node
(Plugins -> teamcity-configs -> teamcity-configs:generate), the
'Debug' option is available in the context menu for the task.
*/

version = "2023.05"

project {
    description = "Carbon's audio engine"

    vcsRoot(AudioScripts)

    buildType(AudioScriptsPyPi)
    buildType(DebugWindows)
    buildType(InternalMacOS)
    buildType(DebugMacOS)
    buildType(PublishToPerforce)
    buildType(TrinityDevMacOS)
    buildType(TrinityDevWindows)
    buildType(InternalWindows)
    buildType(ReleaseMacOS)
    buildType(ReleaseWindows)
}

object AudioScriptsPyPi : BuildType({
    templates(AbsoluteId("PythonPyPIDockerfile"))
    name = "audio-scripts PyPi"
    description = "Build and deploy the Python packages audioscripts-standlone and audioscripts-monolith"

    params {
        param("dockerfile", "Dockerfile")
    }

    vcs {
        root(AudioScripts)
    }

    steps {
        script {
            name = "Publish audioscripts-standalone"
            id = "RUNNER_277"
            scriptContent = """
                docker build \
                --build-arg PACKAGE_DIRECTORY=standalone \
                --build-arg BUILD_NUMBER=%env.BUILD_NUMBER% \
                --build-arg PYPI_URL=%env.PYPI_URL% \
                --build-arg PYPI_USER=%env.PYPI_USER% \
                --build-arg PYPI_PASSWORD=%env.PYPI_PASSWORD% \
                --file %dockerfile% .
            """.trimIndent()
        }
        script {
            name = "Publish audioscripts-monolith"
            id = "RUNNER_143"
            scriptContent = """
                docker build \
                --build-arg PACKAGE_DIRECTORY=monolith \
                --build-arg BUILD_NUMBER=%env.BUILD_NUMBER% \
                --build-arg PYPI_URL=%env.PYPI_URL% \
                --build-arg PYPI_USER=%env.PYPI_USER% \
                --build-arg PYPI_PASSWORD=%env.PYPI_PASSWORD% \
                --file %dockerfile% .
            """.trimIndent()
        }
        stepsOrder = arrayListOf("RUNNER_277", "RUNNER_143")
    }

    triggers {
        vcs {
            id = "TRIGGER_190"
            branchFilter = ""
        }
    }
})

object DebugMacOS : BuildType({
    templates(AbsoluteId("Carbon_GitHubPerforceCMakeProjectTemplate"))
    name = "Debug macOS"

    params {
        param("eve_branch_type", "snapshot")
        param("env.SENTRY_CLI_DEBUG_SYMBOL_TYPE", "dsym")
        param("env.CMAKE_CONFIG_TYPE", "Debug")
        param("eve_branch_path", "//eve-frontier/branches/%eve_branch_type%/%eve_branch_shortname%/")
        param("eve_branch_shortname", "VS")
    }

    vcs {
        root(DslContext.settingsRoot)
    }

    triggers {
        vcs {
            id = "TRIGGER_193"
            triggerRules = "+:root=${DslContext.settingsRoot.id}:."

        }
    }

    requirements {
        startsWith("teamcity.agent.jvm.os.name", "Mac OS X", "RQ_342")
    }
})

object DebugWindows : BuildType({
    templates(AbsoluteId("Carbon_GitHubPerforceCMakeProjectTemplate"))
    name = "Debug Windows"

    params {
        param("eve_branch_type", "snapshot")
        param("env.SENTRY_CLI_DEBUG_SYMBOL_TYPE", "pdb")
        param("env.CMAKE_CONFIG_TYPE", "Debug")
        param("eve_branch_path", "//eve-frontier/branches/%eve_branch_type%/%eve_branch_shortname%/")
        param("eve_branch_shortname", "VS")
    }

    vcs {
        root(DslContext.settingsRoot)
    }

    triggers {
        vcs {
            id = "TRIGGER_193"
            triggerRules = "+:root=${DslContext.settingsRoot.id}:."

        }
    }

    requirements {
        startsWith("teamcity.agent.jvm.os.name", "Windows Server 2019", "RQ_342")
    }
})

object InternalMacOS : BuildType({
    templates(AbsoluteId("Carbon_GitHubPerforceCMakeProjectTemplate"))
    name = "Internal macOS"

    params {
        param("eve_branch_type", "snapshot")
        param("env.SENTRY_CLI_DEBUG_SYMBOL_TYPE", "dsym")
        param("env.CMAKE_CONFIG_TYPE", "Internal")
        param("eve_branch_path", "//eve-frontier/branches/%eve_branch_type%/%eve_branch_shortname%/")
        param("eve_branch_shortname", "VS")
    }

    vcs {
        root(DslContext.settingsRoot)
    }

    triggers {
        vcs {
            id = "TRIGGER_193"
            triggerRules = "+:root=${DslContext.settingsRoot.id}:."

        }
    }

    requirements {
        startsWith("teamcity.agent.jvm.os.name", "Mac OS X", "RQ_342")
    }
})

object InternalWindows : BuildType({
    templates(AbsoluteId("Carbon_GitHubPerforceCMakeProjectTemplate"))
    name = "Internal Windows"

    params {
        param("eve_branch_type", "snapshot")
        param("env.SENTRY_CLI_DEBUG_SYMBOL_TYPE", "pdb")
        param("env.CMAKE_CONFIG_TYPE", "Internal")
        param("eve_branch_path", "//eve-frontier/branches/%eve_branch_type%/%eve_branch_shortname%/")
        param("eve_branch_shortname", "VS")
    }

    vcs {
        root(DslContext.settingsRoot)
    }

    triggers {
        vcs {
            id = "TRIGGER_193"
            triggerRules = "+:root=${DslContext.settingsRoot.id}:."

        }
    }

    requirements {
        startsWith("teamcity.agent.jvm.os.name", "Windows Server 2019", "RQ_342")
    }
})

object PublishToPerforce : BuildType({
    templates(AbsoluteId("Carbon_PublishToPerforceTemplate"))
    name = "Publish to Perforce"

    enablePersonalBuilds = false
    type = BuildTypeSettings.Type.DEPLOYMENT
    maxRunningBuilds = 1

    params {
        param("perforce_path_to_publish_into", "vendor/github.com/ccpgames/carbon-audio")
        param("project", "eve")
        param("eve_branch_shortname", "CARBON-TO-GITHUB")
    }

    dependencies {
        dependency(DebugMacOS) {
            snapshot {
                onDependencyFailure = FailureAction.FAIL_TO_START
            }

            artifacts {
                id = "ARTIFACT_DEPENDENCY_554"
                artifactRules = "**/*=>/%eve_branch_root%/%perforce_path_to_publish_into%/${DebugMacOS.depParamRefs["env.GIT_TAG_HASH"]}"
            }
        }
        dependency(DebugWindows) {
            snapshot {
                onDependencyFailure = FailureAction.FAIL_TO_START
            }

            artifacts {
                id = "ARTIFACT_DEPENDENCY_555"
                artifactRules = "**/*=>/%eve_branch_root%/%perforce_path_to_publish_into%/${DebugWindows.depParamRefs["env.GIT_TAG_HASH"]}"
            }
        }
        dependency(InternalMacOS) {
            snapshot {
                onDependencyFailure = FailureAction.FAIL_TO_START
            }

            artifacts {
                id = "ARTIFACT_DEPENDENCY_556"
                artifactRules = "**/*=>/%eve_branch_root%/%perforce_path_to_publish_into%/${InternalMacOS.depParamRefs["env.GIT_TAG_HASH"]}"
            }
        }
        dependency(InternalWindows) {
            snapshot {
                onDependencyFailure = FailureAction.FAIL_TO_START
            }

            artifacts {
                id = "ARTIFACT_DEPENDENCY_557"
                artifactRules = "**/*=>/%eve_branch_root%/%perforce_path_to_publish_into%/${InternalWindows.depParamRefs["env.GIT_TAG_HASH"]}"
            }
        }
        dependency(ReleaseMacOS) {
            snapshot {
                onDependencyFailure = FailureAction.FAIL_TO_START
            }

            artifacts {
                id = "ARTIFACT_DEPENDENCY_558"
                artifactRules = "**/*=>/%eve_branch_root%/%perforce_path_to_publish_into%/${ReleaseMacOS.depParamRefs["env.GIT_TAG_HASH"]}"
            }
        }
        dependency(ReleaseWindows) {
            snapshot {
                onDependencyFailure = FailureAction.FAIL_TO_START
            }

            artifacts {
                id = "ARTIFACT_DEPENDENCY_559"
                artifactRules = "**/*=>/%eve_branch_root%/%perforce_path_to_publish_into%/${ReleaseWindows.depParamRefs["env.GIT_TAG_HASH"]}"
            }
        }
        dependency(TrinityDevMacOS) {
            snapshot {
                onDependencyFailure = FailureAction.FAIL_TO_START
            }

            artifacts {
                id = "ARTIFACT_DEPENDENCY_560"
                artifactRules = "**/*=>/%eve_branch_root%/%perforce_path_to_publish_into%/${TrinityDevMacOS.depParamRefs["env.GIT_TAG_HASH"]}"
            }
        }
        dependency(TrinityDevWindows) {
            snapshot {
                onDependencyFailure = FailureAction.FAIL_TO_START
            }

            artifacts {
                id = "ARTIFACT_DEPENDENCY_561"
                artifactRules = "**/*=>/%eve_branch_root%/%perforce_path_to_publish_into%/${TrinityDevWindows.depParamRefs["env.GIT_TAG_HASH"]}"
            }
        }
    }
    
    disableSettings("RUNNER_851")
})

object ReleaseMacOS : BuildType({
    templates(AbsoluteId("Carbon_GitHubPerforceCMakeProjectTemplate"))
    name = "Release macOS"

    params {
        param("eve_branch_type", "snapshot")
        param("env.SENTRY_CLI_DEBUG_SYMBOL_TYPE", "dsym")
        param("env.CMAKE_CONFIG_TYPE", "Release")
        param("eve_branch_path", "//eve-frontier/branches/%eve_branch_type%/%eve_branch_shortname%/")
        param("eve_branch_shortname", "VS")
    }

    vcs {
        root(DslContext.settingsRoot)
    }

    triggers {
        vcs {
            id = "TRIGGER_193"
            triggerRules = "+:root=${DslContext.settingsRoot.id}:."

        }
    }

    requirements {
        startsWith("teamcity.agent.jvm.os.name", "Mac OS X", "RQ_342")
    }
})

object ReleaseWindows : BuildType({
    templates(AbsoluteId("Carbon_GitHubPerforceCMakeProjectTemplate"))
    name = "Release Windows"

    params {
        param("eve_branch_type", "snapshot")
        param("env.SENTRY_CLI_DEBUG_SYMBOL_TYPE", "pdb")
        param("env.CMAKE_CONFIG_TYPE", "Release")
        param("eve_branch_path", "//eve-frontier/branches/%eve_branch_type%/%eve_branch_shortname%/")
        param("eve_branch_shortname", "VS")
    }

    vcs {
        root(DslContext.settingsRoot)
    }

    triggers {
        vcs {
            id = "TRIGGER_193"
            triggerRules = "+:root=${DslContext.settingsRoot.id}:."

        }
    }

    requirements {
        startsWith("teamcity.agent.jvm.os.name", "Windows Server 2019", "RQ_342")
    }
})

object TrinityDevMacOS : BuildType({
    templates(AbsoluteId("Carbon_GitHubPerforceCMakeProjectTemplate"))
    name = "TrinityDev macOS"

    params {
        param("eve_branch_type", "snapshot")
        param("env.SENTRY_CLI_DEBUG_SYMBOL_TYPE", "dsym")
        param("env.CMAKE_CONFIG_TYPE", "TrinityDev")
        param("eve_branch_path", "//eve-frontier/branches/%eve_branch_type%/%eve_branch_shortname%/")
        param("eve_branch_shortname", "VS")
    }

    vcs {
        root(DslContext.settingsRoot)
    }

    triggers {
        vcs {
            id = "TRIGGER_193"
            triggerRules = "+:root=${DslContext.settingsRoot.id}:."

        }
    }

    requirements {
        startsWith("teamcity.agent.jvm.os.name", "Mac OS X", "RQ_342")
    }
})

object TrinityDevWindows : BuildType({
    templates(AbsoluteId("Carbon_GitHubPerforceCMakeProjectTemplate"))
    name = "TrinityDev Windows"

    params {
        param("env.SENTRY_CLI_DEBUG_SYMBOL_TYPE", "pdb")
        param("env.CMAKE_CONFIG_TYPE", "TrinityDev")
        param("eve_branch_path", "//eve/branches/%eve_branch_type%/%eve_branch_shortname%/")
        param("eve_branch_shortname", "CARBON-TO-GITHUB")
    }

    vcs {
        root(DslContext.settingsRoot)
    }

    triggers {
        vcs {
            id = "TRIGGER_193"
            triggerRules = "+:root=${DslContext.settingsRoot.id}:."

        }
    }

    requirements {
        startsWith("teamcity.agent.jvm.os.name", "Windows Server 2019", "RQ_342")
    }
})

object AudioScripts : GitVcsRoot({
    name = "audio-scripts"
    url = "https://github.com/ccpgames/audio-scripts"
    branch = "main"
    authMethod = password {
        userName = "ccp-zoetrope"
        password = "credentialsJSON:4cfb10f9-a149-4ce1-8201-325f0eb3a54f"
    }
    param("oauthProviderId", "PROJECT_EXT_71")
    param("tokenType", "undefined")
})
